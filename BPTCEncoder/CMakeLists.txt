# FasTC
# Copyright (c) 2012 University of North Carolina at Chapel Hill. All rights reserved.
#
# Permission to use, copy, modify, and distribute this software and its documentation for educational, 
# research, and non-profit purposes, without fee, and without a written agreement is hereby granted, 
# provided that the above copyright notice, this paragraph, and the following four paragraphs appear 
# in all copies.
#
# Permission to incorporate this software into commercial products may be obtained by contacting the 
# authors or the Office of Technology Development at the University of North Carolina at Chapel Hill <otd@unc.edu>.
#
# This software program and documentation are copyrighted by the University of North Carolina at Chapel Hill. 
# The software program and documentation are supplied "as is," without any accompanying services from the 
# University of North Carolina at Chapel Hill or the authors. The University of North Carolina at Chapel Hill 
# and the authors do not warrant that the operation of the program will be uninterrupted or error-free. The 
# end-user understands that the program was developed for research purposes and is advised not to rely 
# exclusively on the program for any reason.
#
# IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE AUTHORS BE LIABLE TO ANY PARTY FOR 
# DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE 
# USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE 
# AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, 
# BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
# STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY 
# OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
# ENHANCEMENTS, OR MODIFICATIONS.
#
# Please send all BUG REPORTS to <pavel@cs.unc.edu>.
#
# The authors may be contacted via:
#
# Pavel Krajcevski
# Dept of Computer Science
# 201 S Columbia St
# Frederick P. Brooks, Jr. Computer Science Bldg
# Chapel Hill, NC 27599-3175
# USA
# 
# <http://gamma.cs.unc.edu/FasTC/>

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/Base/include)

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/BPTCEncoder/include)
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/BPTCEncoder/include)

IF(NOT "" STREQUAL "${AVPCLLIB_ROOT}")
  INCLUDE_DIRECTORIES(${AVPCLLIB_INCLUDE_DIR})
  SET(FOUND_NVTT_BPTC_EXPORT TRUE)
ENDIF()

INCLUDE(CheckCXXSourceRuns)

IF( NOT HAS_INLINE_ASSEMBLY AND NOT HAS_INLINE_ASSEMBLY_WITH_FLAGS )
  SET( NO_INLINE_ASSEMBLY true )
ENDIF()

# Check to see whether or not our compiler supports atomic operations
IF( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
	"${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" )
  SET( COMPILER_CLANG True )
ELSEIF( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
		"${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" )
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
  "include/BPTCConfig.h"
)

SET( HEADERS
  config/BPTCConfig.h.in
  include/BPTCCompressor.h
  src/CompressionMode.h
  src/RGBAEndpoints.h
  src/ParallelStage.h
)

SET( SOURCES
  src/Compressor.cpp
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

ADD_LIBRARY( BPTCEncoder
  ${HEADERS}
  ${SOURCES}
)

TARGET_LINK_LIBRARIES( BPTCEncoder FasTCBase )

IF(NOT "" STREQUAL "${AVPCLLIB_ROOT}")
  TARGET_LINK_LIBRARIES( BPTCEncoder avpcl )
ENDIF()
