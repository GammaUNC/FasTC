
SET( SOURCES
	ImageFile.cpp
)

SET( HEADERS
	ImageLoader.h
	ImageFile.h
)

ADD_LIBRARY(TexCompIO
	${SOURCES}
	${HEADERS}
)
