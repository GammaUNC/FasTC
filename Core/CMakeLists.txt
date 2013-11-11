# FasTC
# Copyright (c) 2013 University of North Carolina at Chapel Hill.
# All rights reserved.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for educational, research, and non-profit purposes, without
# fee, and without a written agreement is hereby granted, provided that the
# above copyright notice, this paragraph, and the following four paragraphs
# appear in all copies.
#
# Permission to incorporate this software into commercial products may be
# obtained by contacting the authors or the Office of Technology Development
# at the University of North Carolina at Chapel Hill <otd@unc.edu>.
#
# This software program and documentation are copyrighted by the University of
# North Carolina at Chapel Hill. The software program and documentation are
# supplied "as is," without any accompanying services from the University of
# North Carolina at Chapel Hill or the authors. The University of North
# Carolina at Chapel Hill and the authors do not warrant that the operation of
# the program will be uninterrupted or error-free. The end-user understands
# that the program was developed for research purposes and is advised not to
# rely exclusively on the program for any reason.
#
# IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE
# AUTHORS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL,
# OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF
# THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA
# AT CHAPEL HILL OR THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.
#
# THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY
# DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
# STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON
# AN "AS IS" BASIS, AND THE UNIVERSITY  OF NORTH CAROLINA AT CHAPEL HILL AND
# THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
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

SET( SOURCES
	"src/TexComp.cpp"
	"src/CompressedImage.cpp"
)

SET( HEADERS
	"include/TexComp.h"
	"include/CompressedImage.h"
    "src/CompressionFuncs.h"
)

# Make sure to add the appropriate stopwatch files...
SET( HEADERS	${HEADERS} "src/StopWatch.h" )

IF( WIN32 )
	SET( SOURCES ${SOURCES} "src/StopWatchWin32.cpp" )
ELSEIF( APPLE )
	SET( SOURCES ${SOURCES} "src/StopWatchOSX.cpp" )
ELSE()
	SET( SOURCES ${SOURCES} "src/StopWatchUnix.cpp" )
	
	# Assume compiler is GCC
	SET( LINK_FLAGS -lrt ${LINK_FLAGS} )
ENDIF()

INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/Base/include )
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR} )

INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/ETCEncoder/include )
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/DXTEncoder/include )

INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/PVRTCEncoder/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/PVRTCEncoder/include)

INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/BPTCEncoder/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/BPTCEncoder/include )

INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/IO/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/IO/include )

INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/Core/include )

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

SET( HEADERS ${HEADERS} "src/Thread.h" )
SET( HEADERS ${HEADERS} "include/ThreadSafeStreambuf.h" )
SET( HEADERS ${HEADERS} "src/ThreadGroup.h" )
SET( HEADERS ${HEADERS} "src/WorkerQueue.h" )

SET( SOURCES ${SOURCES} "src/ThreadSafeStreambuf.cpp" )
SET( SOURCES ${SOURCES} "src/Thread.cpp" )
SET( SOURCES ${SOURCES} "src/ThreadGroup.cpp" )
SET( SOURCES ${SOURCES} "src/WorkerQueue.cpp" )

ADD_LIBRARY( FasTCCore
	${HEADERS}
	${SOURCES}
)

TARGET_LINK_LIBRARIES( FasTCCore FasTCBase )
TARGET_LINK_LIBRARIES( FasTCCore FasTCIO )
TARGET_LINK_LIBRARIES( FasTCCore ETCEncoder )
TARGET_LINK_LIBRARIES( FasTCCore DXTEncoder )
TARGET_LINK_LIBRARIES( FasTCCore BPTCEncoder )
TARGET_LINK_LIBRARIES( FasTCCore PVRTCEncoder )

IF( CMAKE_USE_PTHREADS_INIT )
    TARGET_LINK_LIBRARIES( FasTCCore ${CMAKE_THREAD_LIBS_INIT} )
ENDIF()

IF( NOT WIN32 AND NOT APPLE )
  	TARGET_LINK_LIBRARIES( FasTCCore rt )
ENDIF()
