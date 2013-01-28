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

SET( SOURCES
	"src/TexComp.cpp"
	"src/CompressedImage.cpp"
	"src/Image.cpp"
	"src/BlockStats.cpp"
)

SET( HEADERS
	"include/TexComp.h"
	"include/CompressedImage.h"
	"include/TexCompTypes.h"
	"include/Image.h"
	"include/BlockStats.h"
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

INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/BPTCEncoder/include )
INCLUDE_DIRECTORIES( ${TexC_BINARY_DIR}/BPTCEncoder/include )

INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/IO/include )
INCLUDE_DIRECTORIES( ${TexC_BINARY_DIR}/IO/include )

INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/Core/include )

SET( THREAD_API )
SET( THREAD_APIS_AVAILABLE "None" )

###### Find Boost...

IF( MSVC )
		SET(ENV{BOOSTLIBDIR} "${MSVC_LIB_DIR}")
		SET(ENV{BOOSTINCLUDEDIR} "${MSVC_INSTALL_PATH}/include")
ENDIF( MSVC )

SET( Boost_USE_STATIC_LIBS ON )
FIND_PACKAGE( Boost COMPONENTS thread system date_time chrono ) 
IF( Boost_FOUND )

    SET( THREAD_APIS_AVAILABLE "Boost" ${THREAD_APIS_AVAILABLE} )
    IF( "${THREAD_API}" STREQUAL "" )
    	SET( THREAD_API "Boost" )
    ENDIF()

ENDIF()

###### Find PThreads....

FIND_PACKAGE( Threads )
IF( CMAKE_USE_PTHREADS_INIT )
    
    SET( THREAD_APIS_AVAILABLE "PThread" ${THREAD_APIS_AVAILABLE} )
    IF( "${THREAD_API}" STREQUAL "" )
        SET( THREAD_API "PThread" )
    ENDIF()
    
ENDIF()

IF( "${THREAD_API}" STREQUAL "" )
    SET( THREAD_API "None" )
ENDIF()

SET( THREAD_API_DESC "API used to implement threading operations")
SET( THREAD_API ${THREAD_API} CACHE STRING ${THREAD_API_DESC} FORCE )
SET_PROPERTY( CACHE THREAD_API PROPERTY STRINGS ${THREAD_APIS_AVAILABLE} )

IF( THREAD_API MATCHES "Boost")
	INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
	SET( SOURCES ${SOURCES} "src/ThreadBoost.cpp" )
	LINK_DIRECTORIES( ${Boost_LIBRARY_DIRS} )
ENDIF()

IF( THREAD_API MATCHES "PThread" )
	SET( SOURCES ${SOURCES} "src/ThreadPThread.cpp" )
ENDIF()

IF( THREAD_API MATCHES "None")
  MESSAGE( FATAL_ERROR "Could not find suitable threading library." )
ELSE()
	SET( SOURCES ${SOURCES} "src/Thread.cpp" )
	SET( SOURCES ${SOURCES} "src/ThreadGroup.cpp" )
	SET( SOURCES ${SOURCES} "src/WorkerQueue.cpp" )

	SET( HEADERS ${HEADERS} "src/Thread.h" )
	SET( HEADERS ${HEADERS} "src/ThreadGroup.h" )
	SET( HEADERS ${HEADERS} "src/WorkerQueue.h" )
ENDIF()

ADD_LIBRARY( TexCompCore
	${HEADERS}
	${SOURCES}
)

TARGET_LINK_LIBRARIES( TexCompCore TexCompIO )
TARGET_LINK_LIBRARIES( TexCompCore BPTCEncoder )

IF( THREAD_API MATCHES "Boost" )
	TARGET_LINK_LIBRARIES( TexCompCore ${Boost_LIBRARIES} )
ENDIF()

IF( THREAD_API MATCHES "PThread" )
    TARGET_LINK_LIBRARIES( TexCompCore ${CMAKE_THREAD_LIBS_INIT} )
ENDIF()

IF( NOT WIN32 AND NOT APPLE )
  	TARGET_LINK_LIBRARIES( TexCompCore rt )
ENDIF()
