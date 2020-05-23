// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <optional>

#include "Common/Matrix.h"
#include "VideoCommon/VideoConfig.h"

class PointerWrap;

class CameraController
{
public:
  CameraController() = default;
  virtual ~CameraController() = default;

  CameraController(const CameraController&) = delete;
  CameraController& operator=(const CameraController&) = delete;

  CameraController(CameraController&&) = delete;
  CameraController& operator=(CameraController&&) = delete;

  virtual Common::Matrix44 GetView() = 0;

  virtual void MoveVertical(float amt) = 0;
  virtual void MoveHorizontal(float amt) = 0;

  virtual void Zoom(float amt) = 0;

  virtual void Rotate(const Common::Vec3& amt) = 0;

  virtual void Reset() = 0;

  virtual void DoState(PointerWrap& p) = 0;
};

class FreeLookCamera
{
public:
  void SetControlType(FreelookControlType type);
  Common::Matrix44 GetView();

  void MoveVertical(float amt);
  void MoveHorizontal(float amt);

  void Zoom(float amt);

  void Rotate(const Common::Vec3& amt);

  void Reset();

  void DoState(PointerWrap& p);

  bool IsDirty() const;
  void SetClean();

private:
  bool m_dirty = false;
  std::optional<FreelookControlType> m_current_type;
  std::unique_ptr<CameraController> m_camera_controller;
};

extern FreeLookCamera g_freelook_camera;
