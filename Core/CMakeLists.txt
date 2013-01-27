
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

	SET( HEADERS ${HEADERS} "include/Thread.h" )
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
