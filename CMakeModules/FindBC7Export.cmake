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

SET(AVPCLLIB_ROOT "" CACHE STRING "Location of the BC7 Export library from NVTT")

IF(NOT AVPCLLIB_ROOT STREQUAL "")
  IF(NOT EXISTS "${AVPCLLIB_ROOT}/src/CMakeLists.txt")
    CONFIGURE_FILE(
      "${CMAKE_CURRENT_LIST_DIR}/bc7_export/CMakeLists.txt"
      "${AVPCLLIB_ROOT}/src"
      COPYONLY)
  ENDIF()
  ADD_SUBDIRECTORY(${AVPCLLIB_ROOT}/src ${CMAKE_CURRENT_BINARY_DIR}/bc7_export)
  set(AVPCLLIB_INCLUDE_DIR ${AVPCLLIB_ROOT}/src )
  SET(FOUND_NVTT_BPTC_EXPORT TRUE)
ENDIF()

mark_as_advanced( FORCE AVPCLLIB_ROOT AVPCLLIB_INCLUDE_DIR )
