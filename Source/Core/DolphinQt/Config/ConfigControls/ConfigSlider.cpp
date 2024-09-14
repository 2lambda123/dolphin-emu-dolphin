// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Config/ConfigControls/ConfigSlider.h"

#include <QMouseEvent>
#include <QSignalBlocker>

#include "Common/Config/Config.h"

#include "DolphinQt/Settings.h"

ConfigSlider::ConfigSlider(int minimum, int maximum, const Config::Info<int>& setting, int tick)
    : ConfigSlider(minimum, maximum, setting, nullptr, tick)
{
}

ConfigSlider::ConfigSlider(int minimum, int maximum, const Config::Info<int>& setting,
                           std::shared_ptr<Config::Layer> target_layer, int tick)
    : ToolTipSlider(Qt::Horizontal), m_setting(setting), m_layer(target_layer)

{
  if (m_layer == nullptr)
  {
    setMinimum(minimum);
    setMaximum(maximum);
    setTickInterval(tick);

    setValue(Config::Get(setting));

    connect(this, &ConfigSlider::valueChanged, this, &ConfigSlider::Update);

    connect(&Settings::Instance(), &Settings::ConfigChanged, this, [this] {
      QFont bf = font();
      bf.setBold(Config::GetActiveLayerForConfig(m_setting) != Config::LayerType::Base);
      setFont(bf);

      const QSignalBlocker blocker(this);
      setValue(Config::Get(m_setting));
    });
  }
  else
  {
    setMinimum(minimum);
    setMaximum(maximum);
    setTickInterval(tick);

    setValue(m_layer->Get(setting));

    connect(this, &ConfigSlider::valueChanged, this, &ConfigSlider::Update);

    connect(&Settings::Instance(), &Settings::ConfigChanged, this, [this] {
      QFont bf = font();
      bf.setBold(m_layer->Exists(m_setting.GetLocation()));
      setFont(bf);

      const QSignalBlocker blocker(this);
      setValue(m_layer->Get(m_setting));
    });
  }
}

void ConfigSlider::Update(int value)
{
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

void ConfigSlider::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::RightButton && m_layer != nullptr)
  {
    m_layer->DeleteKey(m_setting.GetLocation());
    Config::OnConfigChanged();
  }
  else
  {
    QSlider::mousePressEvent(event);
  }
}
