// Copyright 2018 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Config/ConfigControls/ConfigRadio.h"

#include <QMouseEvent>
#include <QSignalBlocker>

#include "Common/Config/Config.h"

#include "DolphinQt/Settings.h"

ConfigRadioInt::ConfigRadioInt(const QString& label, const Config::Info<int>& setting, int value,
                               std::shared_ptr<Config::Layer> target_layer)
    : ToolTipRadioButton(label), m_setting(setting), m_value(value), m_layer(target_layer)
{
  if (m_layer == nullptr)
  {
    setChecked(Config::Get(m_setting) == m_value);
    connect(this, &QRadioButton::toggled, this, &ConfigRadioInt::Update);

    connect(&Settings::Instance(), &Settings::ConfigChanged, this, [this] {
      QFont bf = font();
      bf.setBold(Config::GetActiveLayerForConfig(m_setting) != Config::LayerType::Base);
      setFont(bf);

      const QSignalBlocker blocker(this);
      setChecked(Config::Get(m_setting) == m_value);
    });
  }
  else
  {
    setChecked(m_layer->Get(m_setting) == m_value);
    connect(this, &QRadioButton::toggled, this, &ConfigRadioInt::Update);

    connect(&Settings::Instance(), &Settings::ConfigChanged, this, [this] {
      QFont bf = font();
      bf.setBold(m_layer->Exists(m_setting.GetLocation()));
      setFont(bf);

      const QSignalBlocker blocker(this);
      setChecked(m_layer->Get(m_setting) == m_value);
    });
  }
}

void ConfigRadioInt::Update()
{
  if (isChecked())
  {
    // This checks if a game ini was loaded for editing without a game running. Its hacky, but
    // avoids a more intensive method.
    if (m_layer != nullptr)
    {
      m_layer->Set(m_setting, m_value);
      Config::OnConfigChanged();
    }
    else
    {
      Config::SetBaseOrCurrent(m_setting, m_value);
    }

    emit OnSelected(m_value);
  }
  else
  {
    emit OnDeselected(m_value);
  }
}

void ConfigRadioInt::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::RightButton && m_layer != nullptr)
  {
    m_layer->DeleteKey(m_setting.GetLocation());
    Config::OnConfigChanged();
  }
  else
  {
    QRadioButton::mousePressEvent(event);
  }
}
