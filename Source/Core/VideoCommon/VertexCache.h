// Copyright 2022 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "VideoCommon/CPMemory.h"

namespace VertexCache
{
void ReadData(u32 addr, u32 count, u8* dest);
// WARNING: These functions do not adjust for endianity!
template <typename T, u32 N>
std::array<T, N> ReadData(u32 addr)
{
  std::array<T, N> res{};
  ReadData(addr, N * sizeof(T), reinterpret_cast<u8*>(res.data()));
  return res;
}
template <typename T, u32 N>
std::array<T, N> ReadData(CPArray array, u16 index)
{
  u32 addr = g_main_cp_state.array_bases[array] + index * g_main_cp_state.array_strides[array];
  return ReadData<T, N>(addr);
}
template <typename T>
T ReadData(CPArray array, u16 index)
{
  return ReadData<T, 1>(array, index)[0];
}
void Invalidate();
}  // namespace VertexCache
