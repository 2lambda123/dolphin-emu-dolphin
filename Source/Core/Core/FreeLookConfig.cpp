// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "Core/FreeLookConfig.h"
#include "Core/Config/FreeLookSettings.h"
#include "Core/ConfigManager.h"
#include "Core/Core.h"

namespace FreeLook
{
static Config s_config;
static Config s_active_config;
static bool s_has_registered_callback = false;

Config& GetConfig()
{
  return s_config;
}

const Config& GetActiveConfig()
{
  return s_active_config;
}

void UpdateActiveConfig()
{
  s_active_config = s_config;
}

Config::Config()
{
  camera_config.control_type = ControlType::SixAxis;
  camera_config_2d.layers = "";
  enabled = false;
  enabled_2d = false;
}

void Config::Refresh()
{
  if (!s_has_registered_callback)
  {
    ::Config::AddConfigChangedCallback([] { Core::RunAsCPUThread([] { s_config.Refresh(); }); });
    s_has_registered_callback = true;
  }

  camera_config.control_type = ::Config::Get(::Config::FL1_CONTROL_TYPE);
  camera_config_2d.layers = ::Config::Get(::Config::FL1_2DLAYERS);
  enabled = ::Config::Get(::Config::FREE_LOOK_ENABLED);
  enabled_2d = ::Config::Get(::Config::FREE_LOOK_2D_ENABLED);
}
}  // namespace FreeLook
