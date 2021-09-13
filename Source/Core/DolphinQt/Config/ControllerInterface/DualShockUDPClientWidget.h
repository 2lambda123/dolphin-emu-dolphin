// Copyright 2019 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QStringList>
#include <QWidget>

class QCheckBox;
class QListWidget;
class QPushButton;

class DualShockUDPClientWidget final : public QWidget
{
  Q_OBJECT
public:
  explicit DualShockUDPClientWidget();

private:
  void CreateWidgets();
  void ConnectWidgets();

  void RefreshServerList();

  void OnServerAdded();
  void OnServerSelectionChanged();
  void OnServerRemoved();
  void OnServersEnabledToggled();

  void OnCalibration();
  void OnCalibrationEnded();

  QCheckBox* m_servers_enabled;
  QListWidget* m_server_list;
  QPushButton* m_calibrate;
  QPushButton* m_add_server;
  QPushButton* m_remove_server;
};
