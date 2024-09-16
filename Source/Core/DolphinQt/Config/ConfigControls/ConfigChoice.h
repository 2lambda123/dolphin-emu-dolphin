// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "DolphinQt/Config/ToolTipControls/ToolTipComboBox.h"

#include "Common/Config/Config.h"

class ConfigChoice : public ToolTipComboBox
{
  Q_OBJECT
public:
  ConfigChoice(const QStringList& options, const Config::Info<int>& setting,
               std::shared_ptr<Config::Layer> layer = nullptr);

private:
  void Update(int choice);
  void mousePressEvent(QMouseEvent* event) override;

  Config::Info<int> m_setting;
  std::shared_ptr<Config::Layer> m_layer = nullptr;
};

class ConfigStringChoice : public ToolTipComboBox
{
  Q_OBJECT
public:
  ConfigStringChoice(const std::vector<std::string>& options,
                     const Config::Info<std::string>& setting);
  ConfigStringChoice(const std::vector<std::pair<QString, QString>>& options,
                     const Config::Info<std::string>& setting);

private:
  void Connect();
  void Update(int index);
  void Load();

  Config::Info<std::string> m_setting;
  bool m_text_is_data = false;
};
