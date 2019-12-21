// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "Common/Version.h"

#include <string>

#include "Common/scmrev.h"

namespace Common
{
#ifdef _DEBUG
#define BUILD_TYPE_STR "Debug "
#elif defined DEBUGFAST
#define BUILD_TYPE_STR "DebugFast "
#else
#define BUILD_TYPE_STR ""
#endif

//const std::string scm_rev_str = ""
//#if !SCM_IS_MASTER
//                                ""
//#endif
//
//#ifdef __INTEL_COMPILER
//    BUILD_TYPE_STR SCM_DESC_STR "-ICC";
//#else
//    BUILD_TYPE_STR SCM_DESC_STR;
//#endif

#define RNO_REV_STR "r1"
const std::string scm_rev_str = "Dolphin RNO - (" RNO_REV_STR ")";

#ifdef _WIN32
const std::string netplay_dolphin_ver = "Dol RNO -" RNO_REV_STR " - Win32";
#elif __APPLE__
const std::string netplay_dolphin_ver = "Dol RNO -" RNO_REV_STR " - macOS";
#else
const std::string netplay_dolphin_ver = "Dol RNO -" RNO_REV_STR " - GNU Linux";
#endif

const std::string scm_rev_git_str = SCM_REV_STR;
const std::string scm_desc_str = SCM_DESC_STR;
const std::string scm_branch_str = SCM_BRANCH_STR;
const std::string scm_distributor_str = SCM_DISTRIBUTOR_STR;

//#ifdef _WIN32
//const std::string netplay_dolphin_ver = SCM_DESC_STR " Windows";
//#elif __APPLE__
//const std::string netplay_dolphin_ver = SCM_DESC_STR " macOS";
//#else
//const std::string netplay_dolphin_ver = SCM_DESC_STR " GNU-Linux";
//#endif
}  // namespace Common
