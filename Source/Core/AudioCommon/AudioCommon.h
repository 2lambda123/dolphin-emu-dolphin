// Copyright 2009 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "AudioCommon/Enums.h"

class Mixer;

namespace AudioCommon
{
Mixer* GetMixer();
void Init();
void Shutdown();
std::string GetDefaultSoundBackend();
std::vector<std::string> GetSoundBackends();
DPL2Quality GetDefaultDPL2Quality();
bool SupportsDPL2Decoder(std::string_view backend);
bool SupportsLatencyControl(std::string_view backend);
bool SupportsVolumeChanges(std::string_view backend);
void UpdateVolume();
void SetSoundStreamRunning(bool running);
void SendAIBuffer(const short* samples, unsigned int num_samples);
void StartAudioDump();
void StopAudioDump();
void IncreaseVolume(unsigned short offset);
void DecreaseVolume(unsigned short offset);
void ToggleMuteVolume();
}  // namespace AudioCommon
