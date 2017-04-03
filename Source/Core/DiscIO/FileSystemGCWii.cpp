// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <cstring>
#include <map>
#include <memory>
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
static const u32 FST_ENTRY_SIZE = 4 * 3;  // An FST entry consists of three 32-bit integers

// Set everything manually.
CFileInfoGCWii::CFileInfoGCWii(const u8* fst, bool wii, u32 index, u32 total_file_infos)
    : m_fst(fst), m_wii(wii), m_index(index), m_total_file_infos(total_file_infos)
{
}

// For the root object only.
// m_fst and m_index must be correctly set before GetSize() is called!
CFileInfoGCWii::CFileInfoGCWii(const u8* fst, bool wii)
    : m_fst(fst), m_wii(wii), m_index(0), m_total_file_infos(GetSize())
{
}

// Copy data that is common to the whole file system.
CFileInfoGCWii::CFileInfoGCWii(const CFileInfoGCWii& file_info, u32 index)
    : CFileInfoGCWii(file_info.m_fst, file_info.m_wii, index, file_info.m_total_file_infos)
{
}

CFileInfoGCWii::~CFileInfoGCWii()
{
}

uintptr_t CFileInfoGCWii::GetAddress() const
{
  return reinterpret_cast<uintptr_t>(m_fst + FST_ENTRY_SIZE * m_index);
}

u32 CFileInfoGCWii::GetNextIndex() const
{
  return IsDirectory() ? GetSize() : m_index + 1;
}

IFileInfo& CFileInfoGCWii::operator++()
{
  m_index = GetNextIndex();
  return *this;
}

std::unique_ptr<IFileInfo> CFileInfoGCWii::clone() const
{
  return std::make_unique<CFileInfoGCWii>(*this);
}

IFileInfo::const_iterator CFileInfoGCWii::begin() const
{
  return const_iterator(std::make_unique<CFileInfoGCWii>(*this, m_index + 1));
}

IFileInfo::const_iterator CFileInfoGCWii::end() const
{
  return const_iterator(std::make_unique<CFileInfoGCWii>(*this, GetNextIndex()));
}

u32 CFileInfoGCWii::Get(EntryProperty entry_property) const
{
  return Common::swap32(m_fst + FST_ENTRY_SIZE * m_index +
                        sizeof(u32) * static_cast<int>(entry_property));
}

u32 CFileInfoGCWii::GetSize() const
{
  return Get(EntryProperty::FILE_SIZE);
}

u64 CFileInfoGCWii::GetOffset() const
{
  return static_cast<u64>(Get(EntryProperty::FILE_OFFSET)) << (m_wii ? 2 : 0);
}

bool CFileInfoGCWii::IsDirectory() const
{
  return (Get(EntryProperty::NAME_OFFSET) & 0xFF000000) != 0;
}

u32 CFileInfoGCWii::GetTotalChildren() const
{
  return Get(EntryProperty::FILE_SIZE) - (m_index + 1);
}

u64 CFileInfoGCWii::GetNameOffset() const
{
  return static_cast<u64>(FST_ENTRY_SIZE) * m_total_file_infos +
         (Get(EntryProperty::NAME_OFFSET) & 0xFFFFFF);
}

std::string CFileInfoGCWii::GetName() const
{
  // TODO: Should we really always use SHIFT-JIS?
  // Some names in Pikmin (NTSC-U) don't make sense without it, but is it correct?
  return SHIFTJISToUTF8(reinterpret_cast<const char*>(m_fst + GetNameOffset()));
}

std::string CFileInfoGCWii::GetPath() const
{
  // The root entry doesn't have a name
  if (m_index == 0)
    return "";

  if (IsDirectory())
  {
    u32 parent_directory_index = Get(EntryProperty::FILE_OFFSET);
    return CFileInfoGCWii(*this, parent_directory_index).GetPath() + GetName() + "/";
  }
  else
  {
    // The parent directory can be found by searching backwards
    // for a directory that contains this file. The search cannot fail,
    // because the root directory at index 0 contains all files.
    CFileInfoGCWii potential_parent(*this, m_index - 1);
    while (!(potential_parent.IsDirectory() &&
             potential_parent.Get(EntryProperty::FILE_SIZE) > m_index))
    {
      potential_parent = CFileInfoGCWii(*this, potential_parent.m_index - 1);
    }
    return potential_parent.GetPath() + GetName();
  }
}

bool CFileInfoGCWii::IsValid(u64 fst_size, const CFileInfoGCWii& parent_directory) const
{
  if (GetNameOffset() >= fst_size)
  {
    ERROR_LOG(DISCIO, "Impossibly large name offset in file system");
    return false;
  }

  if (IsDirectory())
  {
    if (Get(EntryProperty::FILE_OFFSET) != parent_directory.m_index)
    {
      ERROR_LOG(DISCIO, "Incorrect parent offset in file system");
      return false;
    }

    u32 size = Get(EntryProperty::FILE_SIZE);

    if (size <= m_index)
    {
      ERROR_LOG(DISCIO, "Impossibly small directory size in file system");
      return false;
    }

    if (size > parent_directory.Get(EntryProperty::FILE_SIZE))
    {
      ERROR_LOG(DISCIO, "Impossibly large directory size in file system");
      return false;
    }

    for (const IFileInfo& child : *this)
    {
      if (!reinterpret_cast<const CFileInfoGCWii&>(child).IsValid(fst_size, *this))
        return false;
    }
  }

  return true;
}

CFileSystemGCWii::CFileSystemGCWii(const IVolume* volume)
    : IFileSystem(volume), m_valid(false), m_wii(false), m_root(nullptr, false, 0, 0)
{
  // Check if this is a GameCube or Wii disc
  u32 magic_bytes;
  if (m_volume->ReadSwapped(0x18, &magic_bytes, false) && magic_bytes == 0x5D1C9EA3)
    m_wii = true;
  else if (m_volume->ReadSwapped(0x1c, &magic_bytes, false) && magic_bytes == 0xC2339F3D)
    m_wii = false;
  else
    return;

  u32 fst_offset_unshifted, fst_size_unshifted;
  if (!m_volume->ReadSwapped(0x424, &fst_offset_unshifted, m_wii) ||
      !m_volume->ReadSwapped(0x428, &fst_size_unshifted, m_wii))
    return;
  u64 fst_offset = static_cast<u64>(fst_offset_unshifted) << GetOffsetShift();
  u64 fst_size = static_cast<u64>(fst_size_unshifted) << GetOffsetShift();
  if (fst_size < FST_ENTRY_SIZE)
  {
    ERROR_LOG(DISCIO, "File system is too small");
    return;
  }

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
  if (!m_volume->Read(fst_offset, fst_size, m_file_system_table.data(), m_wii))
  {
    ERROR_LOG(DISCIO, "Couldn't read file system table");
    return;
  }

  // Create the root object
  m_root = CFileInfoGCWii(m_file_system_table.data(), m_wii);
  if (!m_root.IsDirectory())
  {
    ERROR_LOG(DISCIO, "File system root is not a directory");
    return;
  }

  if (FST_ENTRY_SIZE * m_root.GetSize() > fst_size)
  {
    ERROR_LOG(DISCIO, "File system has too many entries for its size");
    return;
  }

  // If the FST's final byte isn't 0, CFileInfoGCWii::GetName() can read past the end
  if (m_file_system_table[fst_size - 1] != 0)
  {
    ERROR_LOG(DISCIO, "File system does not end with a null byte");
    return;
  }

  m_valid = m_root.IsValid(fst_size, m_root);
}

CFileSystemGCWii::~CFileSystemGCWii()
{
}

const IFileInfo& CFileSystemGCWii::GetRoot() const
{
  return m_root;
}

std::unique_ptr<IFileInfo> CFileSystemGCWii::FindFileInfo(const std::string& path) const
{
  if (!IsValid())
    return nullptr;

  return FindFileInfo(path, m_root);
}

std::unique_ptr<IFileInfo> CFileSystemGCWii::FindFileInfo(const std::string& path,
                                                          const IFileInfo& file_info) const
{
  // Given a path like "directory1/directory2/fileA.bin", this function will
  // find directory1 and then call itself to search for "directory2/fileA.bin".

  if (path.empty() || path == "/")
    return file_info.clone();

  // It's only possible to search in directories. Searching in a file is an error
  if (!file_info.IsDirectory())
    return nullptr;

  size_t first_dir_separator = path.find('/');
  const std::string searching_for = path.substr(0, first_dir_separator);
  const std::string rest_of_path =
      (first_dir_separator != std::string::npos) ? path.substr(first_dir_separator + 1) : "";

  for (const IFileInfo& child : file_info)
  {
    if (child.GetName() == searching_for)
    {
      // A match is found. The rest of the path is passed on to finish the search.
      std::unique_ptr<IFileInfo> result = FindFileInfo(rest_of_path, child);

      // If the search wasn't successful, the loop continues, just in case there's a second
      // file info that matches searching_for (which probably won't happen in practice)
      if (result)
        return result;
    }
  }

  return nullptr;
}

std::unique_ptr<IFileInfo> CFileSystemGCWii::FindFileInfo(u64 disc_offset) const
{
  if (!IsValid())
    return nullptr;

  // Build a cache (unless there already is one)
  if (m_offset_file_info_cache.empty())
  {
    u32 fst_entries = m_root.GetSize();
    for (u32 i = 0; i < fst_entries; i++)
    {
      CFileInfoGCWii file_info(m_root, i);
      if (!file_info.IsDirectory())
        m_offset_file_info_cache.emplace(file_info.GetOffset() + file_info.GetSize(), i);
    }
  }

  // Get the first file that ends after disc_offset
  auto it = m_offset_file_info_cache.upper_bound(disc_offset);
  if (it == m_offset_file_info_cache.end())
    return nullptr;
  std::unique_ptr<IFileInfo> result(std::make_unique<CFileInfoGCWii>(m_root, it->second));

  // If the file's start isn't after disc_offset, success
  if (result->GetOffset() <= disc_offset)
    return result;

  return nullptr;
}

u64 CFileSystemGCWii::ReadFile(const IFileInfo* file_info, u8* buffer, u64 max_buffer_size,
                               u64 offset_in_file) const
{
  if (!file_info || file_info->IsDirectory())
    return 0;

  if (offset_in_file >= file_info->GetSize())
    return 0;

  u64 read_length = std::min(max_buffer_size, file_info->GetSize() - offset_in_file);

  DEBUG_LOG(DISCIO, "Reading %" PRIx64 " bytes at %" PRIx64 " from file %s. Offset: %" PRIx64
                    " Size: %" PRIx64,
            read_length, offset_in_file, file_info->GetPath().c_str(), file_info->GetOffset(),
            file_info->GetSize());

  m_volume->Read(file_info->GetOffset() + offset_in_file, read_length, buffer, m_wii);
  return read_length;
}

bool CFileSystemGCWii::ExportFile(const IFileInfo* file_info,
                                  const std::string& export_filename) const
{
  if (!file_info || file_info->IsDirectory())
    return false;

  u64 remaining_size = file_info->GetSize();
  u64 file_offset = file_info->GetOffset();

  File::IOFile f(export_filename, "wb");
  if (!f)
    return false;

  bool result = true;

  while (remaining_size)
  {
    // Limit read size to 128 MB
    size_t read_size = (size_t)std::min(remaining_size, (u64)0x08000000);

    std::vector<u8> buffer(read_size);

    result = m_volume->Read(file_offset, read_size, &buffer[0], m_wii);

    if (!result)
      break;

    f.WriteBytes(&buffer[0], read_size);

    remaining_size -= read_size;
    file_offset += read_size;
  }

  return result;
}

bool CFileSystemGCWii::ExportApploader(const std::string& export_folder) const
{
  u32 apploader_size;
  u32 trailer_size;
  const u32 header_size = 0x20;
  if (!m_volume->ReadSwapped(0x2440 + 0x14, &apploader_size, m_wii) ||
      !m_volume->ReadSwapped(0x2440 + 0x18, &trailer_size, m_wii))
    return false;
  apploader_size += trailer_size + header_size;
  DEBUG_LOG(DISCIO, "Apploader size -> %x", apploader_size);

  std::vector<u8> buffer(apploader_size);
  if (m_volume->Read(0x2440, apploader_size, buffer.data(), m_wii))
  {
    std::string export_name(export_folder + "/apploader.img");

    File::IOFile apploader_file(export_name, "wb");
    if (apploader_file)
    {
      apploader_file.WriteBytes(buffer.data(), apploader_size);
      return true;
    }
  }

  return false;
}

u64 CFileSystemGCWii::GetBootDOLOffset() const
{
  u32 offset = 0;
  m_volume->ReadSwapped(0x420, &offset, m_wii);
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
    if (!m_volume->ReadSwapped(dol_offset + 0x00 + i * 4, &offset, m_wii) ||
        !m_volume->ReadSwapped(dol_offset + 0x90 + i * 4, &size, m_wii))
      return 0;
    dol_size = std::max(offset + size, dol_size);
  }

  // Iterate through the 11 data segments
  for (u8 i = 0; i < 11; i++)
  {
    if (!m_volume->ReadSwapped(dol_offset + 0x1c + i * 4, &offset, m_wii) ||
        !m_volume->ReadSwapped(dol_offset + 0xac + i * 4, &size, m_wii))
      return 0;
    dol_size = std::max(offset + size, dol_size);
  }

  return dol_size;
}

bool CFileSystemGCWii::ExportDOL(const std::string& export_folder) const
{
  u64 dol_offset = GetBootDOLOffset();
  u32 dol_size = GetBootDOLSize(dol_offset);

  if (dol_offset == 0 || dol_size == 0)
    return false;

  std::vector<u8> buffer(dol_size);
  if (m_volume->Read(dol_offset, dol_size, &buffer[0], m_wii))
  {
    std::string export_name(export_folder + "/boot.dol");

    File::IOFile dol_file(export_name, "wb");
    if (dol_file)
    {
      dol_file.WriteBytes(&buffer[0], dol_size);
      return true;
    }
  }

  return false;
}

u32 CFileSystemGCWii::GetOffsetShift() const
{
  return m_wii ? 2 : 0;
}

}  // namespace
