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

SET( SOURCES
  "src/TexComp.cpp"
  "src/CompressedImage.cpp"
)

SET( LIBRARY_HEADERS
  "include/FasTC/CompressedImage.h"
  "include/FasTC/ReferenceCounter.h"
  "include/FasTC/StopWatch.h"
  "include/FasTC/TexComp.h"
  "include/FasTC/ThreadSafeStreambuf.h"
)

SET( HEADERS
  ${LIBRARY_HEADERS}
  "src/CompressionFuncs.h"
)

# Make sure to add the appropriate stopwatch files...
IF( WIN32 )
	SET( SOURCES ${SOURCES} "src/StopWatchWin32.cpp" )
ELSEIF( APPLE )
	SET( SOURCES ${SOURCES} "src/StopWatchOSX.cpp" )
ELSE()
	SET( SOURCES ${SOURCES} "src/StopWatchUnix.cpp" )
	
	# Assume compiler is GCC
	SET( LINK_FLAGS -lrt ${LINK_FLAGS} )
ENDIF()

###### Find Threads....
IF( MSVC )
	SET( SOURCES ${SOURCES} "src/ThreadWin32.cpp" )
ELSE()
	FIND_PACKAGE( Threads )
	IF( CMAKE_USE_PTHREADS_INIT )
		SET( SOURCES ${SOURCES} "src/ThreadPThread.cpp" )
	ELSE()
		MESSAGE( FATAL_ERROR "Could not find suitable threading library." )
	ENDIF()
ENDIF()

# Add internal sources
SET( HEADERS ${HEADERS} "src/Thread.h" )
SET( HEADERS ${HEADERS} "src/ThreadGroup.h" )
SET( HEADERS ${HEADERS} "src/WorkerQueue.h" )

SET( SOURCES ${SOURCES} "src/ThreadSafeStreambuf.cpp" )
SET( SOURCES ${SOURCES} "src/Thread.cpp" )
SET( SOURCES ${SOURCES} "src/ThreadGroup.cpp" )
SET( SOURCES ${SOURCES} "src/WorkerQueue.cpp" )

# Dependencies...
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/Base/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/Base/include )
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/Core/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/Core/include )
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/IO/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/IO/include )

INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/ASTCEncoder/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/ASTCEncoder/include )
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/ETCEncoder/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/ETCEncoder/include )
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/DXTEncoder/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/DXTEncoder/include )
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/PVRTCEncoder/include)
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/PVRTCEncoder/include)
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/BPTCEncoder/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/BPTCEncoder/include )

ADD_LIBRARY( FasTCCore
	${HEADERS}
	${SOURCES}
)

INSTALL(
  TARGETS FasTCCore
  EXPORT FasTCTargets
  ARCHIVE DESTINATION lib COMPONENT lib
)

INSTALL(
  FILES ${LIBRARY_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/FasTC COMPONENT dev)

TARGET_LINK_LIBRARIES( FasTCCore FasTCBase )
TARGET_LINK_LIBRARIES( FasTCCore FasTCIO )
TARGET_LINK_LIBRARIES( FasTCCore ETCEncoder )
TARGET_LINK_LIBRARIES( FasTCCore DXTEncoder )
TARGET_LINK_LIBRARIES( FasTCCore BPTCEncoder )
TARGET_LINK_LIBRARIES( FasTCCore PVRTCEncoder )
TARGET_LINK_LIBRARIES( FasTCCore ASTCEncoder )

IF( CMAKE_USE_PTHREADS_INIT )
    TARGET_LINK_LIBRARIES( FasTCCore ${CMAKE_THREAD_LIBS_INIT} )
ENDIF()

IF( NOT WIN32 AND NOT APPLE )
  	TARGET_LINK_LIBRARIES( FasTCCore rt )
ENDIF()
