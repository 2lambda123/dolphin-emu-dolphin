// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#ifdef __MINGW64__
// TDM-GCC has this, but for some reason doesn't enable it. We need it for vsnprintf_l.
#define _WIN32_WINNT 0x0600
#define MINGW_HAS_SECURE_API 1
#endif

#include <algorithm>
#include <array>
#include <assert.h>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <ctype.h>
#include <deque>
#include <errno.h>
#if !defined ANDROID && !defined _WIN32
#include <execinfo.h>
#endif
#include <fcntl.h>
#include <float.h>
#include <fstream>
#include <functional>
#ifndef _WIN32
#include <getopt.h>
#endif
#include <iomanip>
#include <iostream>
#include <limits>
#include <limits.h>
#include <list>
#include <locale.h>
#include <map>
#include <math.h>
#include <memory>
#include <memory.h>
#include <mutex>
#include <numeric>
#ifndef _WIN32
#include <pthread.h>
#endif
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <time.h>
#include <type_traits>
#if defined(__MINGW64__) || !defined(_WIN32)
#include <unistd.h>
#endif
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef _WIN32

#if _MSC_FULL_VER < 180030723 && !defined(__GNUC__)
#error Please update your build environment to VS2013 with Update 3 or later!
#endif

#include <Windows.h>

#endif

#include "Common/Common.h"
#include "Common/Thread.h"
