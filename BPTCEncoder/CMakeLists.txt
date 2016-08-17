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

IF(NOT "" STREQUAL "${AVPCLLIB_ROOT}")
  INCLUDE_DIRECTORIES(${AVPCLLIB_INCLUDE_DIR})
  SET(FOUND_NVTT_BPTC_EXPORT TRUE)
ENDIF()

INCLUDE(CheckCXXSourceRuns)

IF( NOT HAS_INLINE_ASSEMBLY AND NOT HAS_INLINE_ASSEMBLY_WITH_FLAGS )
  SET( NO_INLINE_ASSEMBLY true )
ENDIF()

# Check to see whether or not our compiler supports atomic operations
IF( ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR
	${CMAKE_C_COMPILER_ID} STREQUAL "Clang" )
  SET( COMPILER_CLANG True )
ELSEIF( ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR
		${CMAKE_C_COMPILER_ID} STREQUAL "GNU" )
  SET( COMPILER_GNU True )
ENDIF()

IF( COMPILER_CLANG OR COMPILER_GNU )

  CHECK_CXX_SOURCE_RUNS("
    int main() {
      int x = 0;
      __sync_fetch_and_add(&x, 1);
      return !x;
    }"
    HAS_GCC_ATOMICS
  )

ELSEIF( MSVC )

  CHECK_CXX_SOURCE_RUNS("
    #include <Windows.h>
    int main() {
      unsigned int val;
      unsigned int *x = (unsigned int *)_aligned_malloc(sizeof(int), 32);
      *x = 0;
      val = InterlockedIncrement(x);
      _aligned_free(x);
      return !val;
    }"
    HAS_MSVC_ATOMICS
  )

ENDIF()

IF( HAS_MSVC_ATOMICS OR HAS_GCC_ATOMICS )
  SET(HAS_ATOMICS true)
ENDIF()

CONFIGURE_FILE(
  "config/BPTCConfig.h.in"
  "include/FasTC/BPTCConfig.h"
)

SET(LIBRARY_HEADERS
  "include/FasTC/BPTCCompressor.h"
  "include/FasTC/Shapes.h"
)

SET( HEADERS
  config/BPTCConfig.h.in
  src/AnchorTables.h
  src/CompressionMode.h
  src/RGBAEndpoints.h
  src/ParallelStage.h
  ${LIBRARY_HEADERS}
)

SET( SOURCES
  src/Compressor.cpp
  src/Decompressor.cpp
  src/RGBAEndpoints.cpp
  src/ParallelStage.cpp
)

IF( HAS_SSE_41 ) 

  IF ( HAS_SSE_POPCNT )
	IF( MSVC )
	  ADD_DEFINITIONS( /arch:SSE4.2 )
	ELSE() #Assume GCC
	  ADD_DEFINITIONS( -msse4.2 )
	ENDIF()
  ELSE()
	IF( MSVC )
	  ADD_DEFINITIONS( /arch:SSE4.1 )
	ELSE() #Assume GCC
	  ADD_DEFINITIONS( -msse4.1 )
	ENDIF()
  ENDIF()

  SET( HEADERS
	${HEADERS}
	src/RGBAEndpointsSIMD.h
	src/CompressionModeSIMD.h
  )

  SET( SOURCES
	${SOURCES}
	src/CompressorSIMD.cpp
	src/RGBAEndpointsSIMD.cpp
  )
ENDIF( HAS_SSE_41 )

IF( HAS_INLINE_ASSEMBLY_WITH_FLAGS )
  IF( MSVC )
    # !FIXME!
  ELSE()
    ADD_DEFINITIONS( -fasm-blocks )
  ENDIF()
ENDIF()

IF(NOT "" STREQUAL "${AVPCLLIB_ROOT}")
  SET( SOURCES
    ${SOURCES}
    src/CompressNVTT.cpp
  )
ENDIF()

SET(BPTC_INCLUDES_DIR ${FasTC_BINARY_DIR}/BPTCEncoder/include/FasTC)

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/Base/include)
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/Base/include)
INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/BPTCEncoder/include)
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/BPTCEncoder/include)

ADD_LIBRARY( BPTCEncoder
  ${HEADERS}
  ${SOURCES}
)

INSTALL(TARGETS BPTCEncoder EXPORT FasTCTargets ARCHIVE DESTINATION lib COMPONENT lib)
INSTALL(
  FILES ${LIBRARY_HEADERS} "${BPTC_INCLUDES_DIR}/BPTCConfig.h"
  DESTINATION ${INCLUDE_INSTALL_DIR}/FasTC
  COMPONENT dev)

TARGET_LINK_LIBRARIES( BPTCEncoder FasTCBase )

IF(NOT "" STREQUAL "${AVPCLLIB_ROOT}")
  TARGET_LINK_LIBRARIES( BPTCEncoder avpcl )
ENDIF()
