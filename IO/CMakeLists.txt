
SET( SOURCES
	"src/ImageFile.cpp"
)

SET( HEADERS
	"config/ImageLoader.h.in"
	"include/ImageFile.h"
)

FIND_PACKAGE( PNG )
IF( PNG_FOUND )
	INCLUDE_DIRECTORIES( ${PNG_INCLUDE_DIR} )
	TARGET_LINK_LIBRARIES( ${PNG_LIBRARY} )

	SET( SOURCES ${SOURCES} "src/ImageLoaderPNG.cpp" )
	SET( HEADERS ${HEADERS} "src/ImageLoaderPNG.h" )
ENDIF()

CONFIGURE_FILE(
	"config/ImageLoader.h.in"
	"include/ImageLoader.h"
)

INCLUDE_DIRECTORIES( ${TexC_BINARY_DIR}/IO/include )
INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/IO/include )

ADD_LIBRARY(TexCompIO
	${SOURCES}
	${HEADERS}
)
