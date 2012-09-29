
SET( SOURCES
	"src/TexComp.cpp"
	"src/CompressedImage.cpp"
	"src/Image.cpp"
)

SET( HEADERS
	"include/TexComp.h"
	"include/CompressedImage.h"
	"include/TexCompTypes.h"
	"include/Image.h"
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

SET( THREAD_API "None" )
FIND_PACKAGE( Boost COMPONENTS thread system ) 
IF( Boost_FOUND )
	INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

	SET( THREAD_API "Boost" )
	SET( SOURCES ${SOURCES} "src/ThreadBoost.cpp" )

	LINK_DIRECTORIES( ${Boost_LIBRARY_DIR} )
ENDIF()

IF( NOT THREAD_API MATCHES "None")

	SET( SOURCES ${SOURCES} "src/ThreadGroup.cpp" )
	SET( SOURCES ${SOURCES} "src/WorkerQueue.cpp" )

	SET( HEADERS ${HEADERS} "src/Thread.h" )
	SET( HEADERS ${HEADERS} "src/ThreadGroup.h" )
	SET( HEADERS ${HEADERS} "src/WorkerQueue.h" )
ELSE()
  MESSAGE( FATAL_ERROR "Could not find suitable threading library." )
ENDIF()

ADD_LIBRARY( TexCompCore
	${HEADERS}
	${SOURCES}
)

TARGET_LINK_LIBRARIES( TexCompCore TexCompIO )
TARGET_LINK_LIBRARIES( TexCompCore BPTCEncoder )

IF( Boost_FOUND )
	TARGET_LINK_LIBRARIES( TexCompCore ${Boost_LIBRARIES} )
ENDIF()

IF( NOT WIN32 AND NOT APPLE )
  	TARGET_LINK_LIBRARIES( TexCompCore rt )
ENDIF()
