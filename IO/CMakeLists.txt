
SET( SOURCES
	ImageFile.cpp
)

SET( HEADERS
	ImageLoader.h
	ImageFile.h
)

CONFIGURE_FILE(
	"ImageLoader.h.in"
	"ImageLoader.h"
)

INCLUDE_DIRECTORIES( ${TexC_BINARY_DIR}/IO )

ADD_LIBRARY(TexCompIO
	${SOURCES}
	${HEADERS}
)
