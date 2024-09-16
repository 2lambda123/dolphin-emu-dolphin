// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Config/ConfigControls/ConfigBool.h"

#include <QEvent>
#include <QFont>
#include <QMouseEvent>
#include <QSignalBlocker>

#include "Common/Config/Config.h"

#include "DolphinQt/Settings.h"

ConfigBool::ConfigBool(const QString& label, const Config::Info<bool>& setting, bool reverse)
    : ConfigBool(label, setting, nullptr, reverse)
{
}

ConfigBool::ConfigBool(const QString& label, const Config::Info<bool>& setting,
                       std::shared_ptr<Config::Layer> target_layer, bool reverse)
    : ToolTipCheckBox(label), m_setting(setting), m_layer(target_layer), m_reverse(reverse)
{
  if (m_layer == nullptr)
  {
    setChecked(Config::Get(m_setting) ^ reverse);

    connect(this, &QCheckBox::toggled, this, &ConfigBool::Update);
    connect(&Settings::Instance(), &Settings::ConfigChanged, this, [this] {
      QFont bf = font();
      bf.setBold(Config::GetActiveLayerForConfig(m_setting) != Config::LayerType::Base);
      setFont(bf);

      const QSignalBlocker blocker(this);
      setChecked(Config::Get(m_setting) ^ m_reverse);
    });
  }
  else
  {
    setChecked(m_layer->Get(m_setting) ^ reverse);

    QFont bf = font();
    bf.setBold(m_layer->Exists(m_setting.GetLocation()));
    setFont(bf);

    connect(this, &QCheckBox::toggled, this, &ConfigBool::Update);
    connect(&Settings::Instance(), &Settings::ConfigChanged, this, [this] {
      QFont bf = font();
      bf.setBold(m_layer->Exists(m_setting.GetLocation()));
      setFont(bf);

      const QSignalBlocker blocker(this);
      setChecked(m_layer->Get(m_setting) ^ m_reverse);
    });
  }
}

void ConfigBool::Update()
{
  const bool value = static_cast<bool>(isChecked() ^ m_reverse);

  // This checks if a game ini was loaded for editing without a game running. Its hacky, but avoids
  // a more intensive method.
  if (m_layer != nullptr)
  {
    m_layer->Set(m_setting, value);
    Config::OnConfigChanged();
    return;
  }

  Config::SetBaseOrCurrent(m_setting, value);
}

void ConfigBool::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::RightButton && m_layer != nullptr)
  {
    m_layer->DeleteKey(m_setting.GetLocation());
    Config::OnConfigChanged();
  }
  else
  {
    QCheckBox::mousePressEvent(event);
  }
}
