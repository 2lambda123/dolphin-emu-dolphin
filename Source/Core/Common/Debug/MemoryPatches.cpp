// Copyright 2018 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Common/Debug/MemoryPatches.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace Common::Debug
{
MemoryPatch::MemoryPatch(u32 address_, std::vector<u8> value)
    : address(address_), patch_value(std::move(value)), original_value()
{
}

MemoryPatch::MemoryPatch(u32 address_, u32 value)
    : MemoryPatch(address_, {static_cast<u8>(value >> 24), static_cast<u8>(value >> 16),
                             static_cast<u8>(value >> 8), static_cast<u8>(value)})
{
}

MemoryPatches::MemoryPatches() = default;
MemoryPatches::~MemoryPatches() = default;

void MemoryPatches::SetPatch(u32 address, u32 value)
{
  const std::size_t index = m_patches.size();
  m_patches.emplace_back(address, value);
  Patch(index);
}

void MemoryPatches::SetPatch(u32 address, std::vector<u8> value)
{
  const std::size_t index = m_patches.size();
  m_patches.emplace_back(address, std::move(value));
  Patch(index);
}

const MemoryPatch& MemoryPatches::GetPatch(std::size_t index) const
{
  return m_patches.at(index);
}

const std::vector<MemoryPatch>& MemoryPatches::GetPatches() const
{
  return m_patches;
}

void MemoryPatches::UnsetPatch(u32 address)
{
  const auto it = std::remove_if(m_patches.begin(), m_patches.end(),
                                 [address](const auto& patch) { return patch.address == address; });

  if (it == m_patches.end())
    return;

  const std::size_t size = m_patches.size();
  std::size_t index = size - std::distance(it, m_patches.end());
  while (index < size)
  {
    DisablePatch(index);
    ++index;
  }
  m_patches.erase(it, m_patches.end());
}

void MemoryPatches::EnablePatch(std::size_t index)
{
  if (m_patches[index].is_enabled == MemoryPatch::State::Enabled)
    return;
  m_patches[index].is_enabled = MemoryPatch::State::Enabled;
  Patch(index);
}

void MemoryPatches::DisablePatch(std::size_t index)
{
  if (m_patches[index].is_enabled == MemoryPatch::State::Disabled)
    return;
  m_patches[index].is_enabled = MemoryPatch::State::Disabled;
  Patch(index);
  m_patches[index].original_value.clear();
}

bool MemoryPatches::HasEnabledPatch(u32 address) const
{
  return std::any_of(m_patches.begin(), m_patches.end(), [address](const MemoryPatch& patch) {
    return patch.address == address && patch.is_enabled == MemoryPatch::State::Enabled;
  });
}

void MemoryPatches::RemovePatch(std::size_t index)
{
  DisablePatch(index);
  m_patches.erase(m_patches.begin() + index);
}

void MemoryPatches::ClearPatches()
{
  const std::size_t size = m_patches.size();
  for (std::size_t index = 0; index < size; ++index)
    DisablePatch(index);
  m_patches.clear();
}
}  // namespace Common::Debug
