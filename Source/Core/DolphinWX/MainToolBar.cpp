// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "DolphinWX/MainToolBar.h"

#include <array>
#include <utility>

#include "Core/Core.h"
#include "Core/HW/CPU.h"
#include "DolphinWX/Globals.h"
#include "DolphinWX/WxUtils.h"

wxDEFINE_EVENT(DOLPHIN_EVT_RELOAD_TOOLBAR_BITMAPS, wxCommandEvent);

MainToolBar::MainToolBar(ToolBarType type, wxWindow* parent, wxWindowID id, const wxPoint& pos,
                         const wxSize& size, long style, const wxString& name)
    : wxToolBar{parent, id, pos, size, style, name}, m_type{type}
{
  wxToolBar::SetToolBitmapSize(FromDIP(wxSize{32, 32}));
  InitializeBitmaps();
  AddToolBarButtons();
  wxToolBar::Realize();

  BindEvents();
}

void MainToolBar::BindEvents()
{
  Bind(DOLPHIN_EVT_RELOAD_TOOLBAR_BITMAPS, &MainToolBar::OnReloadBitmaps, this);

  BindMainButtonEvents();

  if (m_type == ToolBarType::Debug)
    BindDebuggerButtonEvents();
}

void MainToolBar::BindMainButtonEvents()
{
  Bind(wxEVT_UPDATE_UI, &MainToolBar::OnUpdateIfCoreNotRunning, this, wxID_OPEN);
  Bind(wxEVT_UPDATE_UI, &MainToolBar::OnUpdateIfCoreNotRunning, this, wxID_REFRESH);
  Bind(wxEVT_UPDATE_UI, &MainToolBar::OnUpdateIfCoreRunningOrPaused, this, IDM_STOP);
  Bind(wxEVT_UPDATE_UI, &MainToolBar::OnUpdateIfCoreRunningOrPaused, this, IDM_TOGGLE_FULLSCREEN);
  Bind(wxEVT_UPDATE_UI, &MainToolBar::OnUpdateIfCoreRunningOrPaused, this, IDM_SCREENSHOT);
}

void MainToolBar::BindDebuggerButtonEvents()
{
  Bind(wxEVT_UPDATE_UI, &MainToolBar::OnUpdateIfCPUCanStep, this, IDM_STEP);
  Bind(wxEVT_UPDATE_UI, &MainToolBar::OnUpdateIfCPUCanStep, this, IDM_STEPOVER);
  Bind(wxEVT_UPDATE_UI, &MainToolBar::OnUpdateIfCPUCanStep, this, IDM_STEPOUT);
  Bind(wxEVT_UPDATE_UI, &MainToolBar::OnUpdateIfCPUCanStep, this, IDM_SKIP);
  Bind(wxEVT_UPDATE_UI, &MainToolBar::OnUpdateIfCorePaused, this, IDM_SETPC);
}

void MainToolBar::OnUpdateIfCoreNotRunning(wxUpdateUIEvent& event)
{
  event.Enable(!Core::IsRunning());
}

void MainToolBar::OnUpdateIfCorePaused(wxUpdateUIEvent& event)
{
  event.Enable(Core::GetState() == Core::CORE_PAUSE);
}

void MainToolBar::OnUpdateIfCoreRunningOrPaused(wxUpdateUIEvent& event)
{
  const auto state = Core::GetState();

  event.Enable(state == Core::CORE_RUN || state == Core::CORE_PAUSE);
}

void MainToolBar::OnUpdateIfCPUCanStep(wxUpdateUIEvent& event)
{
  event.Enable(Core::IsRunning() && CPU::IsStepping());
}

void MainToolBar::OnReloadBitmaps(wxCommandEvent& WXUNUSED(event))
{
  Freeze();

  m_icon_bitmaps.clear();
  InitializeBitmaps();
  ApplyThemeBitmaps();

  if (m_type == ToolBarType::Debug)
    ApplyDebuggerBitmaps();

  Thaw();
}

void MainToolBar::Refresh(bool erase_background, const wxRect* rect)
{
  wxToolBar::Refresh(erase_background, rect);

  RefreshPlayButton();
}

void MainToolBar::InitializeBitmaps()
{
  InitializeThemeBitmaps();

  if (m_type == ToolBarType::Debug)
    InitializeDebuggerBitmaps();
}

void MainToolBar::InitializeThemeBitmaps()
{
  m_icon_bitmaps.insert({{TOOLBAR_FILEOPEN, CreateBitmap("open")},
                         {TOOLBAR_REFRESH, CreateBitmap("refresh")},
                         {TOOLBAR_PLAY, CreateBitmap("play")},
                         {TOOLBAR_STOP, CreateBitmap("stop")},
                         {TOOLBAR_PAUSE, CreateBitmap("pause")},
                         {TOOLBAR_SCREENSHOT, CreateBitmap("screenshot")},
                         {TOOLBAR_FULLSCREEN, CreateBitmap("fullscreen")},
                         {TOOLBAR_CONFIGMAIN, CreateBitmap("config")},
                         {TOOLBAR_CONFIGGFX, CreateBitmap("graphics")},
                         {TOOLBAR_CONTROLLER, CreateBitmap("classic")}});
}

void MainToolBar::InitializeDebuggerBitmaps()
{
  m_icon_bitmaps.insert(
      {{TOOLBAR_DEBUG_STEP, CreateDebuggerBitmap("toolbar_debugger_step")},
       {TOOLBAR_DEBUG_STEPOVER, CreateDebuggerBitmap("toolbar_debugger_step_over")},
       {TOOLBAR_DEBUG_STEPOUT, CreateDebuggerBitmap("toolbar_debugger_step_out")},
       {TOOLBAR_DEBUG_SKIP, CreateDebuggerBitmap("toolbar_debugger_skip")},
       {TOOLBAR_DEBUG_GOTOPC, CreateDebuggerBitmap("toolbar_debugger_goto_pc")},
       {TOOLBAR_DEBUG_SETPC, CreateDebuggerBitmap("toolbar_debugger_set_pc")}});
}

wxBitmap MainToolBar::CreateBitmap(const std::string& name) const
{
  return WxUtils::LoadScaledThemeBitmap(name, this, GetToolBitmapSize());
}

wxBitmap MainToolBar::CreateDebuggerBitmap(const std::string& name) const
{
  constexpr auto scale_flags = WxUtils::LSI_SCALE_DOWN | WxUtils::LSI_ALIGN_CENTER;

  return WxUtils::LoadScaledResourceBitmap(name, this, GetToolBitmapSize(), wxDefaultSize,
                                           scale_flags);
}

void MainToolBar::ApplyThemeBitmaps()
{
  constexpr std::array<std::pair<int, ToolBarBitmapID>, 8> bitmap_entries{
      {{wxID_OPEN, TOOLBAR_FILEOPEN},
       {wxID_REFRESH, TOOLBAR_REFRESH},
       {IDM_STOP, TOOLBAR_STOP},
       {IDM_TOGGLE_FULLSCREEN, TOOLBAR_FULLSCREEN},
       {IDM_SCREENSHOT, TOOLBAR_SCREENSHOT},
       {wxID_PREFERENCES, TOOLBAR_CONFIGMAIN},
       {IDM_CONFIG_GFX_BACKEND, TOOLBAR_CONFIGGFX},
       {IDM_CONFIG_CONTROLLERS, TOOLBAR_CONTROLLER}}};

  for (const auto& entry : bitmap_entries)
    ApplyBitmap(entry.first, entry.second);

  // Separate, as the play button is dual-state and doesn't have a fixed bitmap.
  RefreshPlayButton();
}

void MainToolBar::ApplyDebuggerBitmaps()
{
  constexpr std::array<std::pair<int, ToolBarBitmapID>, 6> bitmap_entries{
      {{IDM_STEP, TOOLBAR_DEBUG_STEP},
       {IDM_STEPOVER, TOOLBAR_DEBUG_STEPOVER},
       {IDM_STEPOUT, TOOLBAR_DEBUG_STEPOUT},
       {IDM_SKIP, TOOLBAR_DEBUG_SKIP},
       {IDM_GOTOPC, TOOLBAR_DEBUG_GOTOPC},
       {IDM_SETPC, TOOLBAR_DEBUG_SETPC}}};

  for (const auto& entry : bitmap_entries)
    ApplyBitmap(entry.first, entry.second);
}

void MainToolBar::ApplyBitmap(int tool_id, ToolBarBitmapID bitmap_id)
{
  const auto& bitmap = m_icon_bitmaps[bitmap_id];

  SetToolDisabledBitmap(tool_id, WxUtils::CreateDisabledButtonBitmap(bitmap));
  SetToolNormalBitmap(tool_id, bitmap);
}

void MainToolBar::AddToolBarButtons()
{
  if (m_type == ToolBarType::Debug)
  {
    AddDebuggerToolBarButtons();
    AddSeparator();
  }

  AddMainToolBarButtons();
}

void MainToolBar::AddMainToolBarButtons()
{
  AddToolBarButton(wxID_OPEN, _("Open"), TOOLBAR_FILEOPEN, _("Open file..."));
  AddToolBarButton(wxID_REFRESH, _("Refresh"), TOOLBAR_REFRESH, _("Refresh game list"));
  AddSeparator();
  AddToolBarButton(IDM_PLAY, _("Play"), TOOLBAR_PLAY, _("Play"));
  AddToolBarButton(IDM_STOP, _("Stop"), TOOLBAR_STOP, _("Stop"));
  AddToolBarButton(IDM_TOGGLE_FULLSCREEN, _("FullScr"), TOOLBAR_FULLSCREEN, _("Toggle fullscreen"));
  AddToolBarButton(IDM_SCREENSHOT, _("ScrShot"), TOOLBAR_SCREENSHOT, _("Take screenshot"));
  AddSeparator();
  AddToolBarButton(wxID_PREFERENCES, _("Config"), TOOLBAR_CONFIGMAIN, _("Configure..."));
  AddToolBarButton(IDM_CONFIG_GFX_BACKEND, _("Graphics"), TOOLBAR_CONFIGGFX,
                   _("Graphics settings"));
  AddToolBarButton(IDM_CONFIG_CONTROLLERS, _("Controllers"), TOOLBAR_CONTROLLER,
                   _("Controller settings"));
}

void MainToolBar::AddDebuggerToolBarButtons()
{
  AddToolBarButton(IDM_STEP, _("Step"), TOOLBAR_DEBUG_STEP, _("Step into the next instruction"));
  AddToolBarButton(IDM_STEPOVER, _("Step Over"), TOOLBAR_DEBUG_STEPOVER,
                   _("Step over the next instruction"));
  AddToolBarButton(IDM_STEPOUT, _("Step Out"), TOOLBAR_DEBUG_STEPOUT,
                   _("Step out of the current function"));
  AddToolBarButton(IDM_SKIP, _("Skip"), TOOLBAR_DEBUG_SKIP,
                   _("Skips the next instruction completely"));
  AddSeparator();
  AddToolBarButton(IDM_GOTOPC, _("Show PC"), TOOLBAR_DEBUG_GOTOPC,
                   _("Go to the current instruction"));
  AddToolBarButton(IDM_SETPC, _("Set PC"), TOOLBAR_DEBUG_SETPC, _("Set the current instruction"));
}

void MainToolBar::AddToolBarButton(int tool_id, const wxString& label, ToolBarBitmapID bitmap_id,
                                   const wxString& short_help)
{
  WxUtils::AddToolbarButton(this, tool_id, label, m_icon_bitmaps[bitmap_id], short_help);
}

void MainToolBar::RefreshPlayButton()
{
  ToolBarBitmapID bitmap_id;
  wxString label;

  if (Core::GetState() == Core::CORE_RUN)
  {
    bitmap_id = TOOLBAR_PAUSE;
    label = _("Pause");
  }
  else
  {
    bitmap_id = TOOLBAR_PLAY;
    label = _("Play");
  }

  FindById(IDM_PLAY)->SetLabel(label);
  SetToolShortHelp(IDM_PLAY, label);
  ApplyBitmap(IDM_PLAY, bitmap_id);
}
