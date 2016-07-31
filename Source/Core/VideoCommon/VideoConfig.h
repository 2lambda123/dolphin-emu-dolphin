// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

// IMPORTANT: UI etc should modify g_Config. Graphics code should read g_ActiveConfig.
// The reason for this is to get rid of race conditions etc when the configuration
// changes in the middle of a frame. This is done by copying g_Config to g_ActiveConfig
// at the start of every frame. Noone should ever change members of g_ActiveConfig
// directly.

#pragma once

#include <string>
#include <vector>

#include "Common/CommonTypes.h"
#include "VideoCommon/VideoCommon.h"

// Log in two categories, and save three other options in the same byte
#define CONF_LOG 1
#define CONF_PRIMLOG 2
#define CONF_SAVETARGETS 8
#define CONF_SAVESHADERS 16

enum AspectMode
{
  ASPECT_AUTO = 0,
  ASPECT_ANALOG_WIDE = 1,
  ASPECT_ANALOG = 2,
  ASPECT_STRETCH = 3,
};

enum EFBScale
{
  SCALE_FORCE_INTEGRAL = -1,
  SCALE_AUTO,
  SCALE_AUTO_INTEGRAL,
  SCALE_1X,
  SCALE_1_5X,
  SCALE_2X,
  SCALE_2_5X,
};

enum StereoMode
{
  STEREO_OFF = 0,
  STEREO_SBS,
  STEREO_TAB,
  STEREO_ANAGLYPH,
  STEREO_OSVR,
  STEREO_3DVISION,
  STEREO_OCULUS,
  STEREO_VR920,
};

enum TGameCamera
{
  CAMERA_YAWPITCHROLL = 0,
  CAMERA_YAWPITCH,
  CAMERA_YAW,
  CAMERA_NONE
};

// NEVER inherit from this class.
struct VideoConfig final
{
  VideoConfig();
  void Load(const std::string& ini_file);
  void LoadVR(const std::string& ini_file);
  void GameIniLoad();
  void GameIniSave();
  void GameIniReset();
  void VerifyValidity();
  void Save(const std::string& ini_file);
  void SaveVR(const std::string& ini_file);
  void UpdateProjectionHack();
  bool IsVSync();
  bool VRSettingsModified();

  // General
  bool bVSync;
  bool bFullscreen;
  bool bExclusiveMode;
  bool bRunning;
  bool bWidescreenHack;
  int iAspectRatio;
  bool bCrop;  // Aspect ratio controls.
  bool bUseXFB;
  bool bUseRealXFB;

  // Enhancements
  int iMultisamples;
  bool bSSAA;
  int iEFBScale;
  bool bForceFiltering;
  int iMaxAnisotropy;
  std::string sPostProcessingShader;

  // Information
  bool bShowFPS;
  bool bShowNetPlayPing;
  bool bShowNetPlayMessages;
  bool bOverlayStats;
  bool bOverlayProjStats;
  bool bTexFmtOverlayEnable;
  bool bTexFmtOverlayCenter;
  bool bLogRenderTimeToFile;

  // Render
  bool bWireFrame;
  bool bDisableFog;

  // Utility
  bool bDumpTextures;
  bool bHiresTextures;
  bool bConvertHiresTextures;
  bool bCacheHiresTextures;
  bool bDumpEFBTarget;
  bool bUseFFV1;
  bool bFreeLook;
  bool bBorderlessFullscreen;

  // Hacks
  bool bEFBAccessEnable;
  bool bPerfQueriesEnable;
  bool bBBoxEnable;
  bool bForceProgressive;

  bool bEFBCopyEnable;
  bool bEFBCopyClearDisable;
  bool bEFBEmulateFormatChanges;
  bool bSkipEFBCopyToRam;
  bool bCopyEFBScaled;
  int iSafeTextureCache_ColorSamples;
  int iPhackvalue[3];
  std::string sPhackvalue[2];
  float fAspectRatioHackW, fAspectRatioHackH;
  bool bEnablePixelLighting;
  bool bFastDepthCalc;
  int iLog;           // CONF_ bits
  int iSaveTargetId;  // TODO: Should be dropped

  // Stereoscopy
  int iStereoMode;
  int iStereoDepth;
  int iStereoConvergence;
  int iStereoConvergencePercentage;
  bool bStereoSwapEyes;
  bool bStereoEFBMonoDepth;
  int iStereoDepthPercentage;

  // VR global
  float fScale;
  float fLeanBackAngle;
  bool bStabilizeRoll;
  bool bStabilizePitch;
  bool bStabilizeYaw;
  bool bStabilizeX;
  bool bStabilizeY;
  bool bStabilizeZ;
  bool bKeyhole;
  float fKeyholeWidth;
  bool bKeyholeSnap;
  float fKeyholeSnapSize;
  bool bPullUp20fps;
  bool bPullUp30fps;
  bool bPullUp60fps;
  bool bPullUpAuto;
  bool bSynchronousTimewarp;
  bool bOpcodeWarningDisable;
  bool bReplayVertexData;
  bool bReplayOtherData;
  bool bPullUp20fpsTimewarp;
  bool bPullUp30fpsTimewarp;
  bool bPullUp60fpsTimewarp;
  bool bPullUpAutoTimewarp;
  bool bOpcodeReplay;
  bool bAsynchronousTimewarp;
  bool bEnableVR;
  bool bLowPersistence;
  bool bDynamicPrediction;
  bool bNoMirrorToWindow;
  bool bOrientationTracking;
  bool bMagYawCorrection;
  bool bPositionTracking;
  bool bChromatic;
  bool bTimewarp;
  bool bVignette;
  bool bNoRestore;
  bool bFlipVertical;
  bool bSRGB;
  bool bOverdrive;
  bool bHqDistortion;
  bool bDisableNearClipping;
  bool bAutoPairViveControllers;
  bool bShowHands;
  bool bShowFeet;
  bool bShowController;
  bool bShowLaserPointer;
  bool bShowAimRectangle;
  bool bShowHudBox;
  bool bShow2DBox;
  bool bShowSensorBar;
  bool bShowGameCamera;
  bool bShowGameFrustum;
  bool bShowTrackingCamera;
  bool bShowTrackingVolume;
  bool bShowBaseStation;

  bool bMotionSicknessAlways;
  bool bMotionSicknessFreelook;
  bool bMotionSickness2D;
  bool bMotionSicknessLeftStick;
  bool bMotionSicknessRightStick;
  bool bMotionSicknessDPad;
  bool bMotionSicknessIR;
  int iMotionSicknessMethod;
  int iMotionSicknessSkybox;
  float fMotionSicknessFOV;

  int iVRPlayer;
  float fTimeWarpTweak;
  u32 iExtraTimewarpedFrames;
  u32 iExtraVideoLoops;
  u32 iExtraVideoLoopsDivider;

  std::string sLeftTexture;
  std::string sRightTexture;
  std::string sGCLeftTexture;
  std::string sGCRightTexture;

  // VR per game
  float fUnitsPerMetre;
  float fFreeLookSensitivity;
  float fHudThickness;
  float fHudDistance;
  float fHud3DCloser;
  float fCameraForward;
  float fCameraPitch;
  float fAimDistance;
  float fMinFOV;
  float fN64FOV;
  float fScreenHeight;
  float fScreenThickness;
  float fScreenDistance;
  float fScreenRight;
  float fScreenUp;
  float fScreenPitch;
  float fTelescopeMaxFOV;
  float fReadPitch;
  u32 iCameraMinPoly;
  bool bDisable3D;
  bool bHudFullscreen;
  bool bHudOnTop;
  bool bDontClearScreen;
  bool bCanReadCameraAngles;
  bool bDetectSkybox;
  int iTelescopeEye;
  int iMetroidPrime;
  // VR layer debugging
  int iSelectedLayer;
  int iFlashState;

  // D3D only config, mostly to be merged into the above
  int iAdapter;

  // VideoSW Debugging
  int drawStart;
  int drawEnd;
  bool bZComploc;
  bool bZFreeze;
  bool bDumpObjects;
  bool bDumpTevStages;
  bool bDumpTevTextureFetches;

  // Static config per API
  // TODO: Move this out of VideoConfig
  struct
  {
    APIType api_type;

    std::vector<std::string> Adapters;  // for D3D
    std::vector<int> AAModes;
    std::vector<std::string> PPShaders;        // post-processing shaders
    std::vector<std::string> AnaglyphShaders;  // anaglyph shaders

    // TODO: merge AdapterName and Adapters array
    std::string AdapterName;  // for OpenGL

    bool bSupportsExclusiveFullscreen;
    bool bSupportsDualSourceBlend;
    bool bSupportsPrimitiveRestart;
    bool bSupportsOversizedViewports;
    bool bSupportsGeometryShaders;
    bool bSupports3DVision;
    bool bSupportsEarlyZ;         // needed by PixelShaderGen, so must stay in VideoCommon
    bool bSupportsBindingLayout;  // Needed by ShaderGen, so must stay in VideoCommon
    bool bSupportsBBox;
    bool bSupportsGSInstancing;  // Needed by GeometryShaderGen, so must stay in VideoCommon
    bool bSupportsPostProcessing;
    bool bSupportsPaletteConversion;
    bool bSupportsClipControl;  // Needed by VertexShaderGen, so must stay in VideoCommon
    bool bSupportsSSAA;
  } backend_info;

  // Utility
  bool RealXFBEnabled() const { return bUseXFB && bUseRealXFB; }
  bool VirtualXFBEnabled() const { return bUseXFB && !bUseRealXFB; }
  bool EFBCopiesToTextureEnabled() const { return bEFBCopyEnable && bSkipEFBCopyToRam; }
  bool EFBCopiesToRamEnabled() const { return bEFBCopyEnable && !bSkipEFBCopyToRam; }
  bool ExclusiveFullscreenEnabled() const
  {
    return backend_info.bSupportsExclusiveFullscreen && !bBorderlessFullscreen;
  }
};

extern VideoConfig g_Config;
extern VideoConfig g_ActiveConfig;

// Called every frame.
void UpdateActiveConfig();
