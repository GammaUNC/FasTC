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

SET( LIBRARY_HEADERS
  "include/FasTC/PVRTCCompressor.h"
)

SET( HEADERS
  ${LIBRARY_HEADERS}
  src/Block.h
  src/PVRTCImage.h
  src/Indexer.h
)

SET( SOURCES
  src/Compressor.cpp
  src/Decompressor.cpp
  src/Block.cpp
  src/PVRTCImage.cpp
)

OPTION(DEBUG_PVRTC_DECODER "Output intermediate images during PVRTC decoding." OFF)

CONFIGURE_FILE(
  "config/PVRTCDefines.h.in"
  "include/FasTC/PVRTCDefines.h"
)

IF( PVRTEXLIB_FOUND )
  INCLUDE_DIRECTORIES( ${PVRTEXLIB_INCLUDE_DIRS} )
  SET( SOURCES
    ${SOURCES}
    src/CompressorPVRLib.cpp
  )
ENDIF()

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/PVRTCEncoder/include)
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/PVRTCEncoder/include)
INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/Base/include)
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/Base/include)

ADD_LIBRARY( PVRTCEncoder
  ${HEADERS}
  ${SOURCES}
)

INSTALL(TARGETS PVRTCEncoder EXPORT FasTCTargets ARCHIVE DESTINATION lib COMPONENT lib)
INSTALL(
  FILES ${LIBRARY_HEADERS} "${FasTC_BINARY_DIR}/PVRTCEncoder/include/FasTC/PVRTCDefines.h"
  DESTINATION ${INCLUDE_INSTALL_DIR}/FasTC
  COMPONENT dev)

TARGET_LINK_LIBRARIES( PVRTCEncoder FasTCBase )
IF( DEBUG_PVRTC_DECODER )
  TARGET_LINK_LIBRARIES( PVRTCEncoder FasTCIO )
ENDIF( DEBUG_PVRTC_DECODER )

IF( PVRTEXLIB_FOUND )
  TARGET_LINK_LIBRARIES( PVRTCEncoder ${PVRTEXLIB_LIBRARIES} )
ENDIF( PVRTEXLIB_FOUND )
