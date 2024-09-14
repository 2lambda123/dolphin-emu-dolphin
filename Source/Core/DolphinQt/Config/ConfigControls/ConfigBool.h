// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DolphinQt/Config/ToolTipControls/ToolTipCheckBox.h"

namespace Config
{
template <typename T>
class Info;
class Layer;
}  // namespace Config

class ConfigBool : public ToolTipCheckBox
{
  Q_OBJECT
public:
  ConfigBool(const QString& label, const Config::Info<bool>& setting, bool reverse = false);
  ConfigBool(const QString& label, const Config::Info<bool>& setting,
             std::shared_ptr<Config::Layer> target_layer, bool reverse = false);

private:
  void Update();
  void mousePressEvent(QMouseEvent* event) override;
  const Config::Info<bool>& m_setting;
  std::shared_ptr<Config::Layer> m_layer;
  bool m_reverse;
};
