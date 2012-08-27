
SET( SOURCES
	"src/TexComp.cpp"
)

SET( HEADERS
	"include/TexComp.h"
)

INCLUDE_DIRECTORIES( ${TexC_SOURCE_DIR}/Core/include )

ADD_LIBRARY( TexCompCore
	${HEADERS}
	${SOURCES}
)
