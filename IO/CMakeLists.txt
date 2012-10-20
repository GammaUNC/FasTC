
SET( SOURCES
	"src/ImageWriter.cpp"
	"src/ImageLoader.cpp"
	"src/FileStream.cpp"
	"src/ImageFile.cpp"
)

SET( HEADERS
	"config/ImageWriter.h.in"
	"config/ImageLoader.h.in"
	"include/ImageFile.h"
	"include/FileStream.h"
)

FIND_PACKAGE( PNG )
IF( PNG_FOUND )
	INCLUDE_DIRECTORIES( ${PNG_INCLUDE_DIR} )

	SET( SOURCES ${SOURCES} "src/ImageLoaderPNG.cpp" )
	SET( HEADERS ${HEADERS} "src/ImageLoaderPNG.h" )
	SET( SOURCES ${SOURCES} "src/ImageWriterPNG.cpp" )
	SET( HEADERS ${HEADERS} "src/ImageWriterPNG.h" )
ENDIF()

CONFIGURE_FILE(
	"config/ImageLoader.h.in"
	"include/ImageLoader.h"
)

CONFIGURE_FILE(
	"config/ImageWriter.h.in"
	"include/ImageWriter.h"
)

INCLUDE_DIRECTORIES( ${TexC_BINARY_DIR}/IO/include )
INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/IO/include )
INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/Core/include )

ADD_LIBRARY(TexCompIO
	${SOURCES}
	${HEADERS}
)

IF( PNG_FOUND )
	TARGET_LINK_LIBRARIES( TexCompIO ${PNG_LIBRARY} )
  TARGET_LINK_LIBRARIES( TexCompIO ${ZLIB_LIBRARY} )
ENDIF()
