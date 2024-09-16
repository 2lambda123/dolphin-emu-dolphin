// Copyright 2019 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Config/ConfigControls/ConfigInteger.h"

#include <QMouseEvent>
#include <QSignalBlocker>

#include "Common/Config/Config.h"

#include "DolphinQt/Settings.h"

ConfigInteger::ConfigInteger(int minimum, int maximum, const Config::Info<int>& setting, int step)
    : ConfigInteger(minimum, maximum, setting, nullptr, step)
{
}

ConfigInteger::ConfigInteger(int minimum, int maximum, const Config::Info<int>& setting,
                             std::shared_ptr<Config::Layer> layer, int step)
    : ToolTipSpinBox(), m_setting(setting), m_layer(layer)
{
  if (m_layer == nullptr)
  {
    setMinimum(minimum);
    setMaximum(maximum);
    setSingleStep(step);

    setValue(Config::Get(setting));

    connect(this, &ConfigInteger::valueChanged, this, &ConfigInteger::Update);
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
    setSingleStep(step);

    setValue(m_layer->Get(setting));

    connect(this, &ConfigInteger::valueChanged, this, &ConfigInteger::Update);
    connect(&Settings::Instance(), &Settings::ConfigChanged, this, [this] {
      QFont bf = font();
      bf.setBold(m_layer->Exists(m_setting.GetLocation()));
      setFont(bf);

      const QSignalBlocker blocker(this);
      setValue(m_layer->Get(m_setting));
    });
  }
}

void ConfigInteger::Update(int value)
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

void ConfigInteger::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::RightButton && m_layer != nullptr)
  {
    m_layer->DeleteKey(m_setting.GetLocation());
    Config::OnConfigChanged();
  }
  else
  {
    QSpinBox::mousePressEvent(event);
  }
}
