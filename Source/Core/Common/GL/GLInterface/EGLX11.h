// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <X11/Xlib.h>

#include "Common/GL/GLInterface/EGL.h"
#include "Common/GL/GLX11Window.h"

class GLContextEGLX11 final : public GLContextEGL
{
public:
  ~GLContextEGLX11() override;

  void UpdateDimensions(int window_width, int window_height) override;

protected:
  EGLDisplay OpenEGLDisplay() override;
  EGLNativeWindowType GetEGLNativeWindow(EGLConfig config) override;

  std::unique_ptr<GLX11Window> m_render_window;
};
