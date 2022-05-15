// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Config/Mapping/MappingNumeric.h"

#include <limits>

#include "DolphinQt/Config/Mapping/MappingWidget.h"

#include "InputCommon/ControllerEmu/ControllerEmu.h"
#include "InputCommon/ControllerInterface/ControllerInterface.h"

MappingDouble::MappingDouble(QWidget* parent, ControllerEmu::NumericSetting<double>* setting,
                             bool is_in_mapping_widget)
    : QDoubleSpinBox(parent), m_setting(*setting)
{
  setDecimals(2);

  if (const auto ui_description = m_setting.GetUIDescription())
    setToolTip(tr(ui_description));

  connect(this, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
          [this, parent, is_in_mapping_widget](double value) {
            m_setting.SetValue(value);
            ConfigChanged();
            if (is_in_mapping_widget)
              static_cast<MappingWidget*>(parent)->SaveSettings();
            m_setting.Callback();
          });
  if (is_in_mapping_widget)
  {
    connect(static_cast<MappingWidget*>(parent), &MappingWidget::ConfigChanged, this,
            &MappingDouble::ConfigChanged);
    connect(static_cast<MappingWidget*>(parent), &MappingWidget::Update, this,
            &MappingDouble::Update);
  }
  else
  {
    connect(static_cast<MappingWidget*>(parent->parentWidget()), &MappingWidget::ConfigChanged,
            this, &MappingDouble::ConfigChanged);
    connect(static_cast<MappingWidget*>(parent->parentWidget()), &MappingWidget::Update, this,
            &MappingDouble::Update);
  }
}

// Overriding QDoubleSpinBox's fixup to set the default value when input is cleared.
void MappingDouble::fixup(QString& input) const
{
  input = QString::number(m_setting.GetDefaultValue());
}

void MappingDouble::ConfigChanged()
{
  QString suffix;

  if (const auto ui_suffix = m_setting.GetUISuffix())
    suffix += QLatin1Char{' '} + tr(ui_suffix);

  if (m_setting.IsSimpleValue())
  {
    setRange(m_setting.GetMinValue(), m_setting.GetMaxValue());
    setButtonSymbols(ButtonSymbols::UpDownArrows);
  }
  else
  {
    constexpr auto inf = std::numeric_limits<double>::infinity();
    setRange(-inf, inf);
    setButtonSymbols(ButtonSymbols::NoButtons);
    suffix += QString::fromUtf8(" 🎮");
  }

  setSuffix(suffix);

  setValue(m_setting.GetValue());
}

void MappingDouble::Update()
{
  if (m_setting.IsSimpleValue() || hasFocus())
    return;

  const QSignalBlocker blocker(this);
  setValue(m_setting.GetValue());
}

MappingBool::MappingBool(MappingWidget* parent, ControllerEmu::NumericSetting<bool>* setting,
                         bool is_in_mapping_widget)
    : QCheckBox(parent), m_setting(*setting)
{
  if (const auto ui_description = m_setting.GetUIDescription())
    setToolTip(tr(ui_description));

  connect(this, &QCheckBox::stateChanged, this, [this, parent](int value) {
    m_setting.SetValue(value != 0);
    ConfigChanged();
    parent->SaveSettings();
    m_setting.Callback();
  });

  if (is_in_mapping_widget)
  {
    connect(static_cast<MappingWidget*>(parent), &MappingWidget::ConfigChanged, this,
            &MappingBool::ConfigChanged);
    connect(static_cast<MappingWidget*>(parent), &MappingWidget::Update, this,
            &MappingBool::Update);
  }
  else
  {
    connect(static_cast<MappingWidget*>(parent->parentWidget()), &MappingWidget::ConfigChanged,
            this, &MappingBool::ConfigChanged);
    connect(static_cast<MappingWidget*>(parent->parentWidget()), &MappingWidget::Update, this,
            &MappingBool::Update);
  }

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
}

void MappingBool::ConfigChanged()
{
  if (m_setting.IsSimpleValue())
    setText({});
  else
    setText(QString::fromUtf8("🎮"));

  setChecked(m_setting.GetValue());
}

void MappingBool::Update()
{
  if (m_setting.IsSimpleValue())
    return;

  const QSignalBlocker blocker(this);
  setChecked(m_setting.GetValue());
}
