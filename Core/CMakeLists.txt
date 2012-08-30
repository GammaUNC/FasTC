
SET( SOURCES
	"src/TexComp.cpp"
	"src/CompressedImage.cpp"
)

SET( HEADERS
	"include/TexComp.h"
	"include/CompressedImage.h"
	"include/TexCompTypes.h"
)

# Make sure to add the appropriate stopwatch files...
SET( HEADERS	${HEADERS} "src/StopWatch.h" )
IF( MSVC )
	SET( SOURCES ${SOURCES} "src/StopWatchWin32.cpp" )
ELSE()
	SET( SOURCES ${SOURCES} "src/StopWatchUnix.cpp" )
ENDIF()

INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/BPTCEncoder/include )
INCLUDE_DIRECTORIES( ${TexC_BINARY_DIR}/BPTCEncoder/include )

INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/IO/include )
INCLUDE_DIRECTORIES( ${TexC_BINARY_DIR}/IO/include )

INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/Core/include )

FIND_PACKAGE( Boost COMPONENTS thread system ) 
IF( Boost_FOUND )
	INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

	SET( SOURCES ${SOURCES} "src/ThreadGroup.cpp")
	SET( HEADERS ${HEADERS} "src/ThreadGroup.h")

	LINK_DIRECTORIES( ${Boost_LIBRARY_DIR} )
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
