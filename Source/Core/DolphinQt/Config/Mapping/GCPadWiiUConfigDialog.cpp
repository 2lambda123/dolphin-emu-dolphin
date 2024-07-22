// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Config/Mapping/GCPadWiiUConfigDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

#include "Core/Config/MainSettings.h"
#include "Core/ConfigManager.h"
#include "DolphinQt/QtUtils/QueueOnObject.h"

#include "InputCommon/GCAdapter.h"

GCPadWiiUConfigDialog::GCPadWiiUConfigDialog(const int port, QWidget* parent)
    : QDialog(parent), m_port{port}
{
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

  CreateLayout();

  LoadSettings();
  ConnectWidgets();
}

GCPadWiiUConfigDialog::~GCPadWiiUConfigDialog()
{
  GCAdapter::SetAdapterCallback(nullptr);
}

void GCPadWiiUConfigDialog::CreateLayout()
{
  setWindowTitle(tr("GameCube Adapter for Wii U at Port %1").arg(m_port + 1));

  m_layout = new QVBoxLayout();
  m_status_label = new QLabel();
  m_rumble = new QCheckBox(tr("Enable Rumble"));
  m_simulate_bongos = new QCheckBox(tr("Simulate DK Bongos"));
  m_button_box = new QDialogButtonBox(QDialogButtonBox::Ok);

  UpdateAdapterStatus();

  auto callback = [this] { QueueOnObject(this, &GCPadWiiUConfigDialog::UpdateAdapterStatus); };
  GCAdapter::SetAdapterCallback(callback);

  m_layout->addWidget(m_status_label);
  m_layout->addWidget(m_rumble);
  m_layout->addWidget(m_simulate_bongos);
  m_layout->addWidget(m_button_box);

  setLayout(m_layout);
}

void GCPadWiiUConfigDialog::ConnectWidgets()
{
  connect(m_rumble, &QCheckBox::toggled, this, &GCPadWiiUConfigDialog::SaveSettings);
  connect(m_simulate_bongos, &QCheckBox::toggled, this, &GCPadWiiUConfigDialog::SaveSettings);
  connect(m_button_box, &QDialogButtonBox::accepted, this, &GCPadWiiUConfigDialog::accept);
}

void GCPadWiiUConfigDialog::UpdateAdapterStatus() const
{
  const char* error_message = nullptr;
  const bool detected = GCAdapter::IsDetected(&error_message);
  QString status_text;

  if (detected)
  {
    status_text = tr("Adapter Detected");
  }
  else if (error_message)
  {
    status_text = tr("Error Opening Adapter: %1").arg(QString::fromUtf8(error_message));
  }
  else
  {
    status_text = tr("No Adapter Detected");
  }

  m_status_label->setText(status_text);

  m_rumble->setEnabled(detected);
  m_simulate_bongos->setEnabled(detected);
}

void GCPadWiiUConfigDialog::LoadSettings() const
{
  m_rumble->setChecked(Get(Config::GetInfoForAdapterRumble(m_port)));
  m_simulate_bongos->setChecked(Get(Config::GetInfoForSimulateKonga(m_port)));
}

void GCPadWiiUConfigDialog::SaveSettings() const
{
  SetBaseOrCurrent(Config::GetInfoForAdapterRumble(m_port), m_rumble->isChecked());
  SetBaseOrCurrent(Config::GetInfoForSimulateKonga(m_port), m_simulate_bongos->isChecked());
}
