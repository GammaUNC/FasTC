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

CMAKE_MINIMUM_REQUIRED(VERSION 3.1 FATAL_ERROR)
PROJECT(FasTC CXX C)

SET(FasTC_MAJOR_VERSION 0)
SET(FasTC_MINOR_VERSION 0)
SET(FasTC_PATCH_VERSION 1)
SET(FasTC_VERSION ${FasTC_MAJOR_VERSION}.${FasTC_MINOR_VERSION}.${FasTC_PATCH_VERSION})

OPTION(TREAT_WARNINGS_AS_ERRORS "Treat compiler warnings as errors. We use the highest warnings levels for compilers." OFF)

IF(MSVC)
	SET(MSVC_INSTALL_PATH "${PROJECT_SOURCE_DIR}/Windows")
	SET(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${MSVC_INSTALL_PATH}")

	IF(${MSVC_VERSION} EQUAL 1600)
		SET(MSVC_VERSION_STRING vc100)
	ELSEIF(${MSVC_VERSION} EQUAL 1700)
		SET(MSVC_VERSION_STRING vc110)
	ELSEIF(${MSVC_VERSION} EQUAL 1800)
		SET(MSVC_VERSION_STRING vc120)
	ELSEIF(${MSVC_VERSION} EQUAL 1900)
		SET(MSVC_VERSION_STRING vc140)
	ELSEIF(${MSVC_VERSION} EQUAL 1500)
		SET(MSVC_VERSION_STRING vc90)
	ELSEIF(${MSVC_VERSION} EQUAL 1400)
		SET(MSVC_VERSION_STRING vc80)
	ENDIF()

	# !FIXME! Actually detect compiler architecture version....
	IF( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		SET(MSVC_ARCHITECTURE_STRING x64)
	ELSE()
		SET(MSVC_ARCHITECTURE_STRING x86)
	ENDIF()

	SET(MSVC_LIB_DIR "${MSVC_INSTALL_PATH}/lib/${MSVC_ARCHITECTURE_STRING}/${MSVC_VERSION_STRING}")
	SET(CMAKE_LIBRARY_PATH "${CMAKE_LIBRARY_PATH};${MSVC_LIB_DIR}")

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

ELSEIF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fms-extensions")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fms-extensions")

ENDIF(MSVC)

IF(TREAT_WARNINGS_AS_ERRORS)
  IF(MSVC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
  ELSEIF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  ENDIF(MSVC)
ENDIF(TREAT_WARNINGS_AS_ERRORS)

SET(CMAKE_MODULE_PATH "${FasTC_SOURCE_DIR}/CMakeModules" ${CMAKE_MODULE_PATH})
FIND_PACKAGE(PVRTexLib)
FIND_PACKAGE(BC7Export)
FIND_PACKAGE(PNG)

######################################################################
##
## Package definitions 
##
######################################################################

SET(INCLUDE_INSTALL_DIR include CACHE INTERNAL "")
SET(LIB_INSTALL_DIR lib CACHE INTERNAL "")
SET(BIN_INSTALL_DIR bin CACHE INTERNAL "")

SET(FASTC_DIRECTORIES
  Base Core IO BPTCEncoder PVRTCEncoder DXTEncoder ETCEncoder ASTCEncoder
)

FOREACH(DIR ${FASTC_DIRECTORIES})
  ADD_SUBDIRECTORY(${DIR})
ENDFOREACH()
ADD_SUBDIRECTORY(CLTool)

SET(FasTC_LIBRARIES FasTCBase FasTCIO FasTCCore BPTCEncoder PVRTCEncoder DXTEncoder ETCEncoder ASTCEncoder)
SET(FasTC_EXECUTABLES tc compare decomp)

######################################################################
##
## Config
##
######################################################################

INCLUDE(CMakePackageConfigHelpers)
CONFIGURE_PACKAGE_CONFIG_FILE(
  "${FasTC_SOURCE_DIR}/CMakeModules/FasTCConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/FasTCConfig.cmake"
  INSTALL_DESTINATION ${LIB_INSTALL_DIR}/cmake/FasTC
  PATH_VARS INCLUDE_INSTALL_DIR BIN_INSTALL_DIR LIB_INSTALL_DIR)

WRITE_BASIC_PACKAGE_VERSION_FILE(
  "${CMAKE_CURRENT_BINARY_DIR}/FasTCConfigVersion.cmake"
  VERSION ${FasTC_VERSION}
  COMPATIBILITY SameMajorVersion)

INSTALL(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/FasTCConfigVersion.cmake" "${CMAKE_CURRENT_BINARY_DIR}/FasTCConfig.cmake"
  DESTINATION ${LIB_INSTALL_DIR}/cmake/FasTC
  COMPONENT dev)

EXPORT(
  TARGETS ${FasTC_LIBRARIES} ${FasTC_EXECUTABLES}
  FILE "${CMAKE_CURRENT_BINARY_DIR}/FasTCTargets.cmake")
EXPORT(PACKAGE FasTC)

INSTALL(
  EXPORT FasTCTargets
  FILE FasTCTargets.cmake
  DESTINATION ${LIB_INSTALL_DIR}/cmake/FasTC)

######################################################################
##
## Testing
##
######################################################################

ENABLE_TESTING()

IF(MSVC)
  SET(gtest_force_shared_crt TRUE CACHE BOOL "Used to properly select MSVC runtime libraries" FORCE)
ENDIF(MSVC)

ADD_SUBDIRECTORY(GTest)
FOREACH(DIR ${FASTC_DIRECTORIES})
  SET(TESTDIR ${FasTC_SOURCE_DIR}/${DIR}/test)
  IF(IS_DIRECTORY ${TESTDIR})
    ADD_SUBDIRECTORY(${TESTDIR})
  ENDIF()
ENDFOREACH()
