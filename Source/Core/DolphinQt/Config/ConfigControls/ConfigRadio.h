// Copyright 2018 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DolphinQt/Config/ToolTipControls/ToolTipRadioButton.h"

#include "Common/Config/Config.h"

class ConfigRadioInt : public ToolTipRadioButton
{
  Q_OBJECT
public:
  ConfigRadioInt(const QString& label, const Config::Info<int>& setting, int value,
                 std::shared_ptr<Config::Layer> target_layer = nullptr);

signals:
  // Since selecting a new radio button deselects the old one, ::toggled will generate two signals.
  // These are convenience functions so you can receive only one signal if desired.
  void OnSelected(int new_value);
  void OnDeselected(int old_value);

private:
  void Update();
  void mousePressEvent(QMouseEvent* event) override;
  Config::Info<int> m_setting;
  std::shared_ptr<Config::Layer> m_layer;
  int m_value;
};
