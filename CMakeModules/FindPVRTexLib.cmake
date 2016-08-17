# Copyright 2016 The University of North Carolina at Chapel Hill
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Please send all BUG REPORTS to <pavel@cs.unc.edu>.
# <http://gamma.cs.unc.edu/FasTC/>

# - Try to find libPVRTexLib
# Once done this will define
#  PVRTEXLIB_FOUND - System has PVRTexLib
#  PVRTEXLIB_INCLUDE_DIRS - The PVRTexLib include directories
#  PVRTEXLIB_LIBRARIES - The libraries needed to use PVRTexLib

IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET( PVRTEXLIB_ROOT "/Applications/Imagination/PowerVR/GraphicsSDK/PVRTexTool/Library" )
  find_path(
    PVRTEXLIB_INCLUDE_DIR PVRTexture.h
    PATHS ${PVRTEXLIB_ROOT}/Include
  )

  find_library(PVRTEXLIB_LIB PVRTexLib
    PATHS ${PVRTEXLIB_ROOT}/OSX_x86/Static
          ${PVRTEXLIB_ROOT}/OSX_x86/Dynamic
  )

  SET( USE_PTHREAD TRUE )

ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  SET( PVRTEXLIB_ROOT "/opt/Imagination/PowerVR/GraphicsSDK/PVRTexTool/Library" )
  find_path(
    PVRTEXLIB_INCLUDE_DIR PVRTexture.h
    PATHS ${PVRTEXLIB_ROOT}/Include
  )

  IF(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    find_library(PVRTEXLIB_LIB PVRTexLib
      PATHS ${PVRTEXLIB_ROOT}/Linux_x86_64/Static
            ${PVRTEXLIB_ROOT}/Linux_x86_64/Dynamic
    )
  ELSE()
    find_library(PVRTEXLIB_LIB PVRTexLib
      PATHS ${PVRTEXLIB_ROOT}/Linux_x86_32/Static
            ${PVRTEXLIB_ROOT}/Linux_x86_32/Dynamic
    )
  ENDIF()

  SET( USE_PTHREAD TRUE )

ELSEIF(MSVC)
  SET( PVRTEXLIB_ROOT "C:/Imagination/PowerVR/GraphicsSDK/PVRTexTool/Library" )
  find_path(
    PVRTEXLIB_INCLUDE_DIR PVRTexture.h
    PATHS ${PVRTEXLIB_ROOT}/Include
  )

  IF(${CMAKE_GENERATOR} MATCHES Win64)
    find_library(PVRTEXLIB_LIB PVRTexLib
      PATHS ${PVRTEXLIB_ROOT}/Windows_x86_64/Static
            ${PVRTEXLIB_ROOT}/Windows_x86_64/Dynamic
    )
  ELSE()
    find_library(PVRTEXLIB_LIB PVRTexLib
      PATHS ${PVRTEXLIB_ROOT}/Windows_x86_32/Static
            ${PVRTEXLIB_ROOT}/Windows_x86_32/Dynamic
    )
  ENDIF()
ENDIF()

IF( USE_PTHREAD )
  FIND_PACKAGE( Threads REQUIRED )
  set(PVRTEXLIB_LIBRARIES
    ${PVRTEXLIB_LIB}
    ${CMAKE_THREAD_LIBS_INIT}
  )
ELSE()
  set(PVRTEXLIB_LIBRARIES ${PVRTEXLIB_LIB})
ENDIF()
set(PVRTEXLIB_INCLUDE_DIRS ${PVRTEXLIB_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBXML2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PVRTexLib  DEFAULT_MSG
                                  PVRTEXLIB_LIB PVRTEXLIB_INCLUDE_DIR)

mark_as_advanced( PVRTEXLIB_ROOT PVRTEXLIB_INCLUDE_DIR PVRTEXLIB_LIB )
