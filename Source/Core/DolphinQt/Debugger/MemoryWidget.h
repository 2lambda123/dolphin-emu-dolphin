// Copyright 2018 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>

#include <QByteArray>
#include <QDockWidget>

#include "Common/CommonTypes.h"
#include "VideoCommon/VideoEvents.h"

class MemoryViewWidget;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QHideEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QShowEvent;
class QSplitter;

namespace Core
{
class CPUThreadGuard;
}

class MemoryWidget : public QDockWidget
{
  Q_OBJECT
public:
  explicit MemoryWidget(QWidget* parent = nullptr);
  ~MemoryWidget();

  void SetAddress(u32 address);
  void Update();
signals:
  void BreakpointsChanged();
  void ShowCode(u32 address);
  void RequestWatch(QString name, u32 address);

private:
  struct TargetAddress
  {
    u32 address = 0;
    bool is_good_address = false;
    bool is_good_offset = false;
  };

  void CreateWidgets();
  void ConnectWidgets();

  void closeEvent(QCloseEvent*) override;
  void hideEvent(QHideEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void RegisterAfterFrameEventCallback();
  void RemoveAfterFrameEventCallback();
  void AutoUpdateTable();
  void DisasmUpdate();

  void LoadSettings();
  void SaveSettings();

  void OnAddressSpaceChanged();
  void OnDisplayChanged();
  void OnBPLogChanged();
  void OnBPTypeChanged();

  void OnSearchAddress();
  void OnFindNextValue();
  void OnFindPreviousValue();

  void OnSetValue();
  void OnSetValueFromFile();

  void OnSearchNotes();
  void OnSelectNote();
  void UpdateNotes();

  void OnDumpMRAM();
  void OnDumpExRAM();
  void OnDumpARAM();
  void OnDumpFakeVMEM();

  void ValidateAndPreviewInputValue();
  QByteArray GetInputData() const;
  TargetAddress GetTargetAddress() const;
  void FindValue(bool next);

  MemoryViewWidget* m_memory_view;
  QSplitter* m_splitter;
  QComboBox* m_search_address;
  QComboBox* m_target_address;
  QLineEdit* m_search_offset;
  QLineEdit* m_data_edit;
  QCheckBox* m_base_check;
  QLabel* m_data_preview;
  QComboBox* m_display_combo;
  QComboBox* m_align_combo;
  QComboBox* m_row_length_combo;
  QCheckBox* m_dual_check;
  QPushButton* m_set_value;

  // Search
  QPushButton* m_find_next;
  QPushButton* m_find_previous;
  QComboBox* m_input_combo;
  QLabel* m_result_label;

  // Address Spaces
  QRadioButton* m_address_space_physical;
  QRadioButton* m_address_space_effective;
  QRadioButton* m_address_space_auxiliary;

  // Breakpoint options
  QRadioButton* m_bp_read_write;
  QRadioButton* m_bp_read_only;
  QRadioButton* m_bp_write_only;
  QCheckBox* m_bp_log_check;

  QGroupBox* m_note_group;
  QLineEdit* m_search_notes;
  QListWidget* m_note_list;
  QString m_note_filter;
  Common::EventHook m_VI_end_field_event;

  bool m_auto_update_enabled = true;
};
