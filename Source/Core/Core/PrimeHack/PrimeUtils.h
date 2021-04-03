#pragma once

#include "Core/PrimeHack/PrimeMod.h"
#include "Core/PrimeHack/Transform.h"

#include <cmath>
#include <sstream>

#include "Core/PowerPC/PowerPC.h"
#include "Core/PrimeHack/HackConfig.h"
#include "InputCommon/GenericMouse.h"
#include "VideoCommon/RenderBase.h"
#include "VideoCommon/VideoCommon.h"

extern std::string info_str;

namespace prime {
u8 read8(u32 addr);
u16 read16(u32 addr);
u32 read32(u32 addr);
u32 readi(u32 addr);
u64 read64(u32 addr);
float readf32(u32 addr);
void write8(u8 var, u32 addr);
void write16(u16 var, u32 addr);
void write32(u32 var, u32 addr);
void write64(u64 var, u32 addr);
void writef32(float var, u32 addr);

constexpr float kTurnrateRatio = 0.00498665500569808449206349206349f;

int get_beam_switch(std::array<int, 4> const& beams);
std::tuple<int, int> get_visor_switch(std::array<std::tuple<int, int>, 4> const& visors, bool combat_visor);

void handle_cursor(u32 x_address, u32 y_address, Region region);
void handle_reticle(u32 x_address, u32 y_address, Region region, float fov);

bool mem_check(u32 address);
void write_invalidate(u32 address, u32 value);

void set_beam_owned(int index, bool owned);
void set_visor_owned(int index, bool owned);
void set_cursor_pos(float x, float y);

void DevInfo(const char* name, const char* format, ...);
void DevInfoMatrix(const char* name, const Transform& t);

std::string GetDevInfo();
void ClrDevInfo();

// Borrowed from DolphinQt MathUtil.h
template <typename T, typename F>
constexpr auto Lerp(const T& x, const T& y, const F& a) -> decltype(x + (y - x) * a)
{
  return x + (y - x) * a;
}

template <typename T, typename F>
constexpr auto AntiLerp(const T& x, const T& y, const F& a) -> decltype((a - x) / (y - x))
{
  return (a - x) / (y - x);
}

}  // namespace prime
