include(FindPkgConfig OPTIONAL)

macro(_internal_message msg)
	if(NOT ${_is_quiet})
		message("${msg}")
	endif()
endmacro()

macro(check_lib var lib)
	set(_is_required 0)
	set(_is_quiet 0)
	set(_arg_list ${ARGN})
	foreach(_arg ${ARGN})
		if(_arg STREQUAL "REQUIRED")
			list(REMOVE_ITEM _arg_list "REQUIRED")
			set(_is_required 1)
		endif()
		if(_arg STREQUAL "QUIET")
			list(REMOVE_ITEM _arg_list "QUIET")
			set(_is_quiet 1)
		endif()
	endforeach()

	if(PKG_CONFIG_FOUND AND NOT ${var}_FOUND)
		string(TOLOWER ${lib} lower_lib)
		pkg_search_module(${var} QUIET ${lower_lib})
	endif()

	if(${var}_FOUND)
		include_directories(${${var}_INCLUDE_DIRS})
		# Make sure include directories for headers found using find_path below
		# are re-added when reconfiguring
		include_directories(${${var}_INCLUDE})
		_internal_message("${lib} found")
	else()
		find_library(${var} ${lib})
		if(_arg_list)
			find_path(${var}_INCLUDE ${_arg_list})
		else()
			set(${var}_INCLUDE FALSE)
		endif()
		if(${var} AND ${var}_INCLUDE)
			include_directories(${${var}_INCLUDE})
			_internal_message("${lib} found")
			set(${var}_FOUND 1 CACHE INTERNAL "")
		else()
			if(_is_required)
				message(FATAL_ERROR "${lib} is required but not found")
			else()
				_internal_message("${lib} not found")
			endif()
		endif()
	endif()
endmacro()

macro(check_libav)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(LIBAV libavcodec>=53.35.0 libavformat>=53.21.0
			libswscale>=2.1.0 libavutil>=51.22.1)
	else()
		message("pkg-config is required to check for libav")
	endif()
	if(LIBAV_FOUND)
		message(STATUS "Checking to see if the found libav works")

		set(CMAKE_REQUIRED_INCLUDES ${LIBAV_INCLUDE_DIR})
		set(CMAKE_REQUIRED_LIBRARIES ${LIBAV_LIBRARY})
		check_cxx_source_compiles("
		extern \"C\" {
		#include <libavcodec/avcodec.h>
		#include <libavformat/avformat.h>
		#include <libswscale/swscale.h>
		#include <libavutil/mathematics.h>
		}

		int main()
		{
			return 0;
		}"
		LIBAV_WORKS)
		if(LIBAV_WORKS)
			message("libav found and compile test passed, enabling AVI frame dumps")
			add_definitions(-DHAVE_LIBAV)
			include_directories(${LIBAV_INCLUDE_DIRS})
		else()
			message("libav found but compile test failed, disabling AVI frame dumps")
			set(LIBAV_FOUND 0 CACHE INTERNAL "")
		endif()
	else()
		message("libav not found, disabling AVI frame dumps")
	endif()
endmacro()

