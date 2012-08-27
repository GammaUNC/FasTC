
SET( SOURCES
	"src/TexComp.cpp"
	"src/CompressedImage.cpp"
)

SET( HEADERS
	"include/TexComp.h"
	"include/CompressedImage.h"
)

INCLUDE_DIRECTORIES( ${TexC_BINARY_DIR}/IO/include )
INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/IO/include )
INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/Core/include )

ADD_LIBRARY( TexCompCore
	${HEADERS}
	${SOURCES}
)

TARGET_LINK_LIBRARIES( TexCompCore TexCompIO )
