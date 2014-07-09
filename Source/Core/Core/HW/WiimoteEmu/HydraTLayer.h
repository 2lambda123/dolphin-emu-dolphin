// Copyright 2014 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

// Razer Hydra Wiimote Translation Layer

#pragma once

#include "Core/HW/WiimoteEmu/WiimoteEmu.h"
#include "Core/HW/WiimoteEmu/Attachment/Classic.h"

namespace HydraTLayer
{

void GetButtons(int index, bool sideways, bool has_extension, wm_core * butt, bool * cycle_extension);
void GetAcceleration(int index, bool sideways, bool has_extension, WiimoteEmu::AccelData * const data);
void GetIR(int index, float * x, float * y, float * z);
void GetNunchukAcceleration(int index, WiimoteEmu::AccelData * const data);
void GetNunchuk(int index, u8 *jx, u8 *jy, u8 *butt);
void GetClassic(int index, wm_classic_extension *ccdata);
void GetGameCube(int index, u16 *butt, u8 *stick_x, u8 *stick_y, u8 *substick_x, u8 *substick_y, u8 *ltrigger, u8 *rtrigger);

}
