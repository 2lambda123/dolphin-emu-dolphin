// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

// PatchEngine
// Supports simple memory patches, and has a partial Action Replay
// implementation
// in ActionReplay.cpp/h.

// TODO: Still even needed?  Zelda WW now works with improved DSP code.
// Zelda item hang fixes:
// [Tue Aug 21 2007] [18:30:40] <Knuckles->    0x802904b4 in US released
// [Tue Aug 21 2007] [18:30:53] <Knuckles->    0x80294d54 in EUR Demo version
// [Tue Aug 21 2007] [18:31:10] <Knuckles->    we just patch a blr on it
// (0x4E800020)
// [OnLoad]
// 0x80020394=dword,0x4e800020

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "Common/CommonPaths.h"
#include "Common/FileUtil.h"
#include "Common/OnionConfig.h"
#include "Common/StringUtil.h"

#include "Core/ActionReplay.h"
#include "Core/ConfigManager.h"
#include "Core/GeckoCode.h"
#include "Core/GeckoCodeConfig.h"
#include "Core/OnionCoreLoaders/GameConfigLoader.h"
#include "Core/PatchEngine.h"
#include "Core/PowerPC/PowerPC.h"

using namespace Common;

namespace PatchEngine
{
const char* PatchTypeStrings[] = {
    "byte", "word", "dword",
};

static std::vector<Patch> onFrame;
static std::map<u32, int> speedHacks;

void LoadPatchSection(OnionConfig::Layer* global_config, OnionConfig::Layer* local_config,
                      std::vector<Patch>& patches)
{
  // Load the name of all enabled patches
  std::vector<std::string> enabledLines;
  std::set<std::string> enabledNames;

  OnionConfig::Section* local_codes_enabled =
      local_config->GetOrCreateSection(OnionConfig::System::SYSTEM_MAIN, "OnFrame_Enabled");

  local_codes_enabled->GetLines(&enabledLines);
  for (const std::string& line : enabledLines)
  {
    if (line.size() != 0 && line[0] == '$')
    {
      std::string name = line.substr(1, line.size() - 1);
      enabledNames.insert(name);
    }
  }

  OnionConfig::Layer* configs[] = {global_config, local_config};
  for (auto config : configs)
  {
    OnionConfig::Section* codes =
        config->GetOrCreateSection(OnionConfig::System::SYSTEM_MAIN, "OnFrame");

    std::vector<std::string> lines;
    Patch currentPatch;
    codes->GetLines(&lines);

    for (std::string& line : lines)
    {
      if (line.size() == 0)
        continue;

      if (line[0] == '$')
      {
        // Take care of the previous code
        if (currentPatch.name.size())
        {
          patches.push_back(currentPatch);
        }
        currentPatch.entries.clear();

        // Set active and name
        currentPatch.name = line.substr(1, line.size() - 1);
        currentPatch.active = enabledNames.find(currentPatch.name) != enabledNames.end();

        currentPatch.user_defined = config == local_config;
      }
      else
      {
        std::string::size_type loc = line.find('=');

        if (loc != std::string::npos)
        {
          line[loc] = ':';
        }

        std::vector<std::string> items;
        SplitString(line, ':', items);

        if (items.size() >= 3)
        {
          PatchEntry pE;
          bool success = true;
          success &= TryParse(items[0], &pE.address);
          success &= TryParse(items[2], &pE.value);

          pE.type = PatchType(std::find(PatchTypeStrings, PatchTypeStrings + 3, items[1]) -
                              PatchTypeStrings);
          success &= (pE.type != (PatchType)3);
          if (success)
          {
            currentPatch.entries.push_back(pE);
          }
        }
      }
    }

    if (currentPatch.name.size() && currentPatch.entries.size())
    {
      patches.push_back(currentPatch);
    }
  }
}

static void LoadSpeedhacks()
{
  OnionConfig::Section* speedhacks =
      OnionConfig::GetOrCreateSection(OnionConfig::System::SYSTEM_MAIN, "Speedhacks");

  const auto& values = speedhacks->GetValues();
  for (auto value : values)
  {
    u32 address;
    u32 cycles;
    bool success = true;
    success &= TryParse(value.first, &address);
    success &= TryParse(value.second, &cycles);
    if (success)
    {
      speedHacks[address] = (int)cycles;
    }
  }
}

int GetSpeedhackCycles(const u32 addr)
{
  std::map<u32, int>::const_iterator iter = speedHacks.find(addr);
  if (iter == speedHacks.end())
    return 0;
  else
    return iter->second;
}

void LoadPatches()
{
  OnionConfig::Layer* global_config =
      OnionConfig::GetLayer(OnionConfig::LayerType::LAYER_GLOBALGAME);
  OnionConfig::Layer* local_config = OnionConfig::GetLayer(OnionConfig::LayerType::LAYER_LOCALGAME);

  LoadPatchSection(global_config, local_config, onFrame);
  ActionReplay::LoadAndApplyCodes(global_config, local_config);

  // lil silly
  std::vector<Gecko::GeckoCode> gcodes;
  Gecko::LoadCodes(global_config, local_config, gcodes);
  Gecko::SetActiveCodes(gcodes);

  LoadSpeedhacks();
}

static void ApplyPatches(const std::vector<Patch>& patches)
{
  for (const Patch& patch : patches)
  {
    if (patch.active)
    {
      for (const PatchEntry& entry : patch.entries)
      {
        u32 addr = entry.address;
        u32 value = entry.value;
        switch (entry.type)
        {
        case PATCH_8BIT:
          PowerPC::HostWrite_U8((u8)value, addr);
          break;
        case PATCH_16BIT:
          PowerPC::HostWrite_U16((u16)value, addr);
          break;
        case PATCH_32BIT:
          PowerPC::HostWrite_U32(value, addr);
          break;
        default:
          // unknown patchtype
          break;
        }
      }
    }
  }
}

void ApplyFramePatches()
{
  // TODO: Messing with MSR this way is really, really, evil; we should
  // probably be using some sort of Gecko OS-style hooking mechanism
  // so the emulated CPU is in a predictable state when we process cheats.
  u32 oldMSR = MSR;
  UReg_MSR newMSR = oldMSR;
  newMSR.IR = 1;
  newMSR.DR = 1;
  MSR = newMSR.Hex;
  ApplyPatches(onFrame);

  // Run the Gecko code handler
  Gecko::RunCodeHandler();
  ActionReplay::RunAllActive();
  MSR = oldMSR;
}

void Shutdown()
{
  onFrame.clear();
  speedHacks.clear();
  ActionReplay::ApplyCodes({});
  Gecko::SetActiveCodes({});
}

}  // namespace
