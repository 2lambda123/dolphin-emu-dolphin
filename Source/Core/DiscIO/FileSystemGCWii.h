// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Common/CommonTypes.h"
#include "DiscIO/Filesystem.h"

namespace DiscIO
{
class IVolume;

class CFileInfoGCWii : public IFileInfo
{
public:
  // Does not take ownership of pointers
  CFileInfoGCWii(const u8* fst, bool wii, u32 index, u32 total_file_infos);

  u64 GetOffset() const override;
  u32 GetSize() const override;
  bool IsDirectory() const override;
  std::string GetName() const override;

private:
  enum class EntryProperty
  {
    NAME_OFFSET = 0,
    FILE_OFFSET = 1,
    FILE_SIZE = 2
  };

  u32 Get(EntryProperty entry_property) const;

  const u8* const m_fst;
  const bool m_wii;
  const u32 m_index;
  const u32 m_total_file_infos;
};

class CFileSystemGCWii : public IFileSystem
{
public:
  CFileSystemGCWii(const IVolume* _rVolume);
  ~CFileSystemGCWii() override;
  bool IsValid() const override { return m_Valid; }
  const std::vector<CFileInfoGCWii>& GetFileList() const override;
  const IFileInfo* FindFileInfo(const std::string& path) const override;
  const IFileInfo* FindFileInfo(u64 disc_offset) const override;
  std::string GetPath(u64 _Address) const override;
  std::string GetPathFromFSTOffset(size_t file_info_offset) const override;
  u64 ReadFile(const IFileInfo* file_info, u8* _pBuffer, u64 _MaxBufferSize,
               u64 _OffsetInFile) const override;
  bool ExportFile(const IFileInfo* file_info, const std::string& _rExportFilename) const override;
  bool ExportApploader(const std::string& _rExportFolder) const override;
  bool ExportDOL(const std::string& _rExportFolder) const override;
  u64 GetBootDOLOffset() const override;
  u32 GetBootDOLSize(u64 dol_offset) const override;

private:
  bool m_Valid;
  bool m_Wii;
  std::vector<CFileInfoGCWii> m_FileInfoVector;
  std::vector<u8> m_file_system_table;

  const IFileInfo* FindFileInfo(const std::string& path, size_t search_start_offset) const;
  u32 GetOffsetShift() const;
};

}  // namespace
