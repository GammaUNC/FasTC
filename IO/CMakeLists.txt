# Copyright 2016 The University of North Carolina at Chapel Hill
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Please send all BUG REPORTS to <pavel@cs.unc.edu>.
# <http://gamma.cs.unc.edu/FasTC/>

SET( SOURCES
	"src/ImageWriter.cpp"
	"src/ImageLoader.cpp"
	"src/ImageFile.cpp"
)

SET( LIBRARY_HEADERS
	"include/FasTC/ImageFile.h"
	"include/FasTC/ImageFileFormat.h"
	"include/FasTC/FileStream.h"
)

SET( HEADERS
	"config/ImageWriter.h.in"
	"config/ImageLoader.h.in"
    ${LIBRARY_HEADERS}
)

IF( WIN32 )
	SET( SOURCES ${SOURCES} "src/FileStreamWin32.cpp" )
ELSE()
	SET( SOURCES ${SOURCES} "src/FileStreamUnix.cpp" )
	
	# Assume compiler is GCC
	SET( LINK_FLAGS -lrt ${LINK_FLAGS} )
ENDIF()

IF( PNG_FOUND )
	INCLUDE_DIRECTORIES( ${PNG_INCLUDE_DIR} )

	SET( SOURCES ${SOURCES} "src/ImageLoaderPNG.cpp" )
	SET( HEADERS ${HEADERS} "src/ImageLoaderPNG.h" )
	SET( SOURCES ${SOURCES} "src/ImageWriterPNG.cpp" )
	SET( HEADERS ${HEADERS} "src/ImageWriterPNG.h" )
ENDIF()

IF( PVRTEXLIB_FOUND )
  INCLUDE_DIRECTORIES( ${PVRTEXLIB_INCLUDE_DIRS} )
  SET( SOURCES ${SOURCES} "src/ImageLoaderPVR.cpp" )
  SET( HEADERS ${HEADERS} "src/ImageLoaderPVR.h" )
ENDIF( PVRTEXLIB_FOUND )

# Add third-party TGA library...
SET( SOURCES ${SOURCES} "third_party/tga/targa.c" )
SET( HEADERS ${HEADERS} "third_party/tga/targa.h" )
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/IO/third_party/tga )

# Add TGA loaders
SET( SOURCES ${SOURCES} "src/ImageLoaderTGA.cpp" )
SET( HEADERS ${HEADERS} "src/ImageLoaderTGA.h" )

# Add KTX loaders
SET( SOURCES ${SOURCES} "src/ImageLoaderKTX.cpp" )
SET( HEADERS ${HEADERS} "src/ImageLoaderKTX.h" )
SET( SOURCES ${SOURCES} "src/ImageWriterKTX.cpp" )
SET( HEADERS ${HEADERS} "src/ImageWriterKTX.h" )

# Add ASTC loader
# This supports the Mali ASTC evaluation codec.
SET( SOURCES ${SOURCES} "src/ImageLoaderASTC.cpp" )
SET( HEADERS ${HEADERS} "src/ImageLoaderASTC.h" )

FIND_PACKAGE( OpenGL )
IF(OPENGL_FOUND)
  INCLUDE_DIRECTORIES( ${OPENGL_INCLUDE_DIRS} )
ENDIF(OPENGL_FOUND)

CONFIGURE_FILE(
	"config/ImageLoader.h.in"
	"include/FasTC/ImageLoader.h"
)

CONFIGURE_FILE(
	"config/ImageWriter.h.in"
	"include/FasTC/ImageWriter.h"
)

INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/Base/include)
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/Base/include)
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/Core/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/Core/include )
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/IO/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/IO/include )

ADD_LIBRARY(FasTCIO ${SOURCES} ${HEADERS})

INSTALL(TARGETS FasTCIO EXPORT FasTCTargets ARCHIVE DESTINATION lib COMPONENT lib)
SET(EXPORT_HEADERS
  ${LIBRARY_HEADERS}
  "${FasTC_BINARY_DIR}/IO/include/FasTC/ImageWriter.h"
  "${FasTC_BINARY_DIR}/IO/include/FasTC/ImageLoader.h"
)
INSTALL(
  FILES ${EXPORT_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/FasTC COMPONENT dev)

TARGET_LINK_LIBRARIES( FasTCIO FasTCBase )
TARGET_LINK_LIBRARIES( FasTCIO FasTCCore )

IF( PNG_FOUND )
  TARGET_LINK_LIBRARIES( FasTCIO ${PNG_LIBRARY} )
  TARGET_LINK_LIBRARIES( FasTCIO ${ZLIB_LIBRARY} )
ENDIF()

IF( PVRTEXLIB_FOUND )
  TARGET_LINK_LIBRARIES( FasTCIO ${PVRTEXLIB_LIBRARIES} )
ENDIF()
