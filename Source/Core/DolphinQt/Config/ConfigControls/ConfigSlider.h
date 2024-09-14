// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DolphinQt/Config/ToolTipControls/ToolTipSlider.h"

namespace Config
{
template <typename T>
class Info;
class Layer;
}  // namespace Config

class ConfigSlider : public ToolTipSlider
{
  Q_OBJECT
public:
  ConfigSlider(int minimum, int maximum, const Config::Info<int>& setting, int tick = 0);
  ConfigSlider(int minimum, int maximum, const Config::Info<int>& setting,
               std::shared_ptr<Config::Layer> target_layer, int tick = 0);

  void Update(int value);

private:
  void mousePressEvent(QMouseEvent* event) override;

  std::shared_ptr<Config::Layer> m_layer;
  const Config::Info<int>& m_setting;
};
