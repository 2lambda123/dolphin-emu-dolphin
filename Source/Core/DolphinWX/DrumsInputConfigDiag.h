// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "DolphinWX/InputConfigDiag.h"

class DrumsInputConfigDialog : public InputConfigDialog
{
public:
  DrumsInputConfigDialog(wxWindow* const parent, InputConfig& config, const wxString& name,
                         const int port_num = 0);
};
