// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include "Common/CommonTypes.h"
#include "Common/FileUtil.h"
#include "Common/Logging/Log.h"
#include "Common/MsgHandler.h"
#include "Common/StringUtil.h"
#include "DiscIO/FileSystemGCWii.h"
#include "DiscIO/Filesystem.h"
#include "DiscIO/Volume.h"

namespace DiscIO
{
CFileInfoGCWii::CFileInfoGCWii(bool wii, const u8* fst_entry, const u8* name_table_start)
    : m_wii(wii), m_fst_entry(fst_entry), m_name_table_start(name_table_start)
{
}

u32 CFileInfoGCWii::Get(EntryProperty entry_property) const
{
  return Common::swap32(m_fst_entry + sizeof(u32) * static_cast<int>(entry_property));
}

u32 CFileInfoGCWii::GetSize() const
{
  return Get(EntryProperty::FILE_SIZE);
}

u64 CFileInfoGCWii::GetOffset() const
{
  return static_cast<u64>(Get(EntryProperty::FILE_OFFSET)) << (m_wii && !IsDirectory() ? 2 : 0);
}

bool CFileInfoGCWii::IsDirectory() const
{
  return (Get(EntryProperty::NAME_OFFSET) & 0xFF000000) != 0;
}

std::string CFileInfoGCWii::GetName() const
{
  // TODO: Should we really always use SHIFT-JIS?
  // Some names in Pikmin (NTSC-U) don't make sense without it, but is it correct?
  const u8* name = m_name_table_start + (Get(EntryProperty::NAME_OFFSET) & 0xFFFFFF);
  return SHIFTJISToUTF8(reinterpret_cast<const char*>(name));
}

CFileSystemGCWii::CFileSystemGCWii(const IVolume* _rVolume)
    : IFileSystem(_rVolume), m_Initialized(false), m_Valid(false), m_Wii(false)
{
  m_Valid = DetectFileSystem();
}

CFileSystemGCWii::~CFileSystemGCWii()
{
  m_FileInfoVector.clear();
}

const std::vector<CFileInfoGCWii>& CFileSystemGCWii::GetFileList()
{
  if (!m_Initialized)
    InitFileSystem();

  return m_FileInfoVector;
}

const IFileInfo* CFileSystemGCWii::FindFileInfo(const std::string& path)
{
  if (!m_Initialized)
    InitFileSystem();

  if (m_FileInfoVector.empty())
    return nullptr;

  return FindFileInfo(path, 0);
}

const IFileInfo* CFileSystemGCWii::FindFileInfo(const std::string& path,
                                                size_t search_start_offset) const
{
  // Given a path like "directory1/directory2/fileA.bin", this function will
  // find directory1 and then call itself to search for "directory2/fileA.bin".

  if (path.empty() || path == "/")
    return &m_FileInfoVector[search_start_offset];

  // It's only possible to search in directories. Searching in a file is an error
  if (!m_FileInfoVector[search_start_offset].IsDirectory())
    return nullptr;

  size_t first_dir_separator = path.find('/');
  const std::string searching_for = path.substr(0, first_dir_separator);
  const std::string rest_of_path =
      (first_dir_separator != std::string::npos) ? path.substr(first_dir_separator + 1) : "";

  size_t search_end_offset = m_FileInfoVector[search_start_offset].GetSize();
  search_start_offset++;
  while (search_start_offset < search_end_offset)
  {
    const CFileInfoGCWii& file_info = m_FileInfoVector[search_start_offset];

    if (file_info.GetName() == searching_for)
    {
      // A match is found. The rest of the path is passed on to finish the search.
      const IFileInfo* result = FindFileInfo(rest_of_path, search_start_offset);

      // If the search wasn't successful, the loop continues, just in case there's a second
      // file info that matches searching_for (which probably won't happen in practice)
      if (result)
        return result;
    }

    if (file_info.IsDirectory())
    {
      // Skip a directory and everything that it contains

      if (file_info.GetSize() <= search_start_offset)
      {
        // The next offset (obtained by GetSize) is supposed to be larger than
        // the current offset. If an FST is malformed and breaks that rule,
        // there's a risk that next offset pointers form a loop.
        // To avoid infinite loops, this method returns.
        ERROR_LOG(DISCIO, "Invalid next offset in file system");
        return nullptr;
      }

      search_start_offset = file_info.GetSize();
    }
    else
    {
      // Skip a single file
      search_start_offset++;
    }
  }

  return nullptr;
}

const IFileInfo* CFileSystemGCWii::FindFileInfo(u64 disc_offset)
{
  if (!m_Initialized)
    InitFileSystem();

  for (auto& file_info : m_FileInfoVector)
  {
    if ((file_info.GetOffset() <= disc_offset) &&
        ((file_info.GetOffset() + file_info.GetSize()) > disc_offset))
    {
      return &file_info;
    }
  }

  return nullptr;
}

std::string CFileSystemGCWii::GetPath(u64 _Address)
{
  if (!m_Initialized)
    InitFileSystem();

  for (size_t i = 0; i < m_FileInfoVector.size(); ++i)
  {
    const CFileInfoGCWii& file_info = m_FileInfoVector[i];
    if ((file_info.GetOffset() <= _Address) &&
        ((file_info.GetOffset() + file_info.GetSize()) > _Address))
    {
      return GetPathFromFSTOffset(i);
    }
  }

  return "";
}

std::string CFileSystemGCWii::GetPathFromFSTOffset(size_t file_info_offset)
{
  if (!m_Initialized)
    InitFileSystem();

  // Root entry doesn't have a name
  if (file_info_offset == 0)
    return "";

  const CFileInfoGCWii& file_info = m_FileInfoVector[file_info_offset];
  if (file_info.IsDirectory())
  {
    // The offset of the parent directory is stored in the current directory.

    if (file_info.GetOffset() >= file_info_offset)
    {
      // The offset of the parent directory is supposed to be smaller than
      // the current offset. If an FST is malformed and breaks that rule,
      // there's a risk that parent directory pointers form a loop.
      // To avoid stack overflows, this method returns.
      ERROR_LOG(DISCIO, "Invalid parent offset in file system");
      return "";
    }
    return GetPathFromFSTOffset(file_info.GetOffset()) + file_info.GetName() + "/";
  }
  else
  {
    // The parent directory can be found by searching backwards
    // for a directory that contains this file.

    size_t parent_offset = file_info_offset - 1;
    while (!(m_FileInfoVector[parent_offset].IsDirectory() &&
             m_FileInfoVector[parent_offset].GetSize() > file_info_offset))
    {
      parent_offset--;
    }
    return GetPathFromFSTOffset(parent_offset) + file_info.GetName();
  }
}

u64 CFileSystemGCWii::ReadFile(const IFileInfo* file_info, u8* _pBuffer, u64 _MaxBufferSize,
                               u64 _OffsetInFile)
{
  if (!m_Initialized)
    InitFileSystem();

  if (!file_info || file_info->IsDirectory())
    return 0;

  if (_OffsetInFile >= file_info->GetSize())
    return 0;

  u64 read_length = std::min(_MaxBufferSize, file_info->GetSize() - _OffsetInFile);

  DEBUG_LOG(DISCIO, "Reading %" PRIx64 " bytes at %" PRIx64 " from file %s. Offset: %" PRIx64
                    " Size: %" PRIx64,
            read_length, _OffsetInFile, GetPath(file_info->GetOffset()).c_str(),
            file_info->GetOffset(), file_info->GetSize());

  m_rVolume->Read(file_info->GetOffset() + _OffsetInFile, read_length, _pBuffer, m_Wii);
  return read_length;
}

bool CFileSystemGCWii::ExportFile(const IFileInfo* file_info, const std::string& _rExportFilename)
{
  if (!m_Initialized)
    InitFileSystem();

  if (!file_info || file_info->IsDirectory())
    return false;

  u64 remainingSize = file_info->GetSize();
  u64 fileOffset = file_info->GetOffset();

  File::IOFile f(_rExportFilename, "wb");
  if (!f)
    return false;

  bool result = true;

  while (remainingSize)
  {
    // Limit read size to 128 MB
    size_t readSize = (size_t)std::min(remainingSize, (u64)0x08000000);

    std::vector<u8> buffer(readSize);

    result = m_rVolume->Read(fileOffset, readSize, &buffer[0], m_Wii);

    if (!result)
      break;

    f.WriteBytes(&buffer[0], readSize);

    remainingSize -= readSize;
    fileOffset += readSize;
  }

  return result;
}

bool CFileSystemGCWii::ExportApploader(const std::string& _rExportFolder) const
{
  u32 apploader_size;
  u32 trailer_size;
  const u32 header_size = 0x20;
  if (!m_rVolume->ReadSwapped(0x2440 + 0x14, &apploader_size, m_Wii) ||
      !m_rVolume->ReadSwapped(0x2440 + 0x18, &trailer_size, m_Wii))
    return false;
  apploader_size += trailer_size + header_size;
  DEBUG_LOG(DISCIO, "Apploader size -> %x", apploader_size);

  std::vector<u8> buffer(apploader_size);
  if (m_rVolume->Read(0x2440, apploader_size, buffer.data(), m_Wii))
  {
    std::string exportName(_rExportFolder + "/apploader.img");

    File::IOFile AppFile(exportName, "wb");
    if (AppFile)
    {
      AppFile.WriteBytes(buffer.data(), apploader_size);
      return true;
    }
  }

  return false;
}

u64 CFileSystemGCWii::GetBootDOLOffset() const
{
  u32 offset = 0;
  m_rVolume->ReadSwapped(0x420, &offset, m_Wii);
  return static_cast<u64>(offset) << GetOffsetShift();
}

u32 CFileSystemGCWii::GetBootDOLSize(u64 dol_offset) const
{
  // The dol_offset value is usually obtained by calling GetBootDOLOffset.
  // If GetBootDOLOffset fails by returning 0, GetBootDOLSize should also fail.
  if (dol_offset == 0)
    return 0;

  u32 dol_size = 0;
  u32 offset = 0;
  u32 size = 0;

  // Iterate through the 7 code segments
  for (u8 i = 0; i < 7; i++)
  {
    if (!m_rVolume->ReadSwapped(dol_offset + 0x00 + i * 4, &offset, m_Wii) ||
        !m_rVolume->ReadSwapped(dol_offset + 0x90 + i * 4, &size, m_Wii))
      return 0;
    dol_size = std::max(offset + size, dol_size);
  }

  // Iterate through the 11 data segments
  for (u8 i = 0; i < 11; i++)
  {
    if (!m_rVolume->ReadSwapped(dol_offset + 0x1c + i * 4, &offset, m_Wii) ||
        !m_rVolume->ReadSwapped(dol_offset + 0xac + i * 4, &size, m_Wii))
      return 0;
    dol_size = std::max(offset + size, dol_size);
  }

  return dol_size;
}

bool CFileSystemGCWii::ExportDOL(const std::string& _rExportFolder) const
{
  u64 DolOffset = GetBootDOLOffset();
  u32 DolSize = GetBootDOLSize(DolOffset);

  if (DolOffset == 0 || DolSize == 0)
    return false;

  std::vector<u8> buffer(DolSize);
  if (m_rVolume->Read(DolOffset, DolSize, &buffer[0], m_Wii))
  {
    std::string exportName(_rExportFolder + "/boot.dol");

    File::IOFile DolFile(exportName, "wb");
    if (DolFile)
    {
      DolFile.WriteBytes(&buffer[0], DolSize);
      return true;
    }
  }

  return false;
}

bool CFileSystemGCWii::DetectFileSystem()
{
  u32 magic_bytes;
  if (m_rVolume->ReadSwapped(0x18, &magic_bytes, false) && magic_bytes == 0x5D1C9EA3)
  {
    m_Wii = true;
    return true;
  }
  else if (m_rVolume->ReadSwapped(0x1c, &magic_bytes, false) && magic_bytes == 0xC2339F3D)
  {
    m_Wii = false;
    return true;
  }

  return false;
}

void CFileSystemGCWii::InitFileSystem()
{
  m_Initialized = true;

  u32 fst_offset_unshifted, fst_size_unshifted;
  if (!m_rVolume->ReadSwapped(0x424, &fst_offset_unshifted, m_Wii) ||
      !m_rVolume->ReadSwapped(0x428, &fst_size_unshifted, m_Wii))
    return;
  u64 fst_offset = static_cast<u64>(fst_offset_unshifted) << GetOffsetShift();
  u64 fst_size = static_cast<u64>(fst_size_unshifted) << GetOffsetShift();
  if (fst_size < 0xC)
    return;

  // 128 MiB is more than the total amount of RAM in a Wii.
  // No file system should use anywhere near that much.
  static const u32 ARBITRARY_FILE_SYSTEM_SIZE_LIMIT = 128 * 1024 * 1024;
  if (fst_size > ARBITRARY_FILE_SYSTEM_SIZE_LIMIT)
  {
    // Without this check, Dolphin can crash by trying to allocate too much
    // memory when loading a disc image with an incorrect FST size.

    ERROR_LOG(DISCIO, "File system is abnormally large! Aborting loading");
    return;
  }

  // Read the whole FST
  m_file_system_table.resize(fst_size);
  if (!m_rVolume->Read(fst_offset, fst_size, m_file_system_table.data(), m_Wii))
    return;

  // Create all file info objects
  u32 number_of_file_infos = Common::swap32(*((u32*)m_file_system_table.data() + 2));
  const u8* fst_start = m_file_system_table.data();
  const u8* name_table_start = fst_start + (number_of_file_infos * 0xC);
  const u8* name_table_end = fst_start + fst_size;
  if (name_table_end < name_table_start)
    return;
  for (u32 i = 0; i < number_of_file_infos; i++)
    m_FileInfoVector.emplace_back(m_Wii, fst_start + (i * 0xC), name_table_start);
}

u32 CFileSystemGCWii::GetOffsetShift() const
{
  return m_Wii ? 2 : 0;
}

}  // namespace
