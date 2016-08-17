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

INCLUDE(CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES("
  #include <cstdint>
  int main() {
    int8_t x8 = 0;
    int16_t x16 = 1;
    int32_t x32 = 2;
    int64_t x64 = 3;
    uint8_t ux8 = 0;
    uint16_t ux16 = 1;
    uint32_t ux32 = 2;
    uint64_t ux64 = 3;
    return (x8 | ux8 | x16 | ux16 | x32 | ux32 | x64 | ux64);
  }"
  FASTC_BASE_HAS_CPP11_TYPES
)

CONFIGURE_FILE(
  "config/BaseConfig.h.in"
  "include/FasTC/BaseConfig.h"
)

SET( SOURCES
  "src/Image.cpp"
  "src/CompressionJob.cpp"
  "src/Pixel.cpp"
  "src/IPixel.cpp"
  "src/Color.cpp"
)

SET( LIBRARY_HEADERS
  "include/FasTC/Image.h"  
  "include/FasTC/ImageFwd.h"
  "include/FasTC/Pixel.h"
  "include/FasTC/TexCompTypes.h"
  "include/FasTC/CompressionFormat.h"
  "include/FasTC/CompressionJob.h"
  "include/FasTC/IPixel.h"
  "include/FasTC/Color.h"
  "include/FasTC/Bits.h"
  "include/FasTC/BitStream.h"
  "include/FasTC/MatrixBase.h"
  "include/FasTC/MatrixSquare.h"
  "include/FasTC/Matrix3x3.h"
  "include/FasTC/Matrix4x4.h"
  "include/FasTC/ScopedAllocator.h"
  "include/FasTC/VectorBase.h"
  "include/FasTC/Vector2.h"
  "include/FasTC/Vector3.h"
  "include/FasTC/Vector4.h")

SET( HEADERS
  ${LIBRARY_HEADERS}
  "${FasTC_BINARY_DIR}/Base/include/FasTC/BaseConfig.h"
  "config/BaseConfig.h.in"
)

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/Base/include)
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/Base/include)

ADD_LIBRARY( FasTCBase
  ${HEADERS}
  ${SOURCES}
)

INSTALL(
  TARGETS FasTCBase
  EXPORT FasTCTargets
  ARCHIVE DESTINATION lib COMPONENT lib)

INSTALL(
  FILES ${LIBRARY_HEADERS} "${FasTC_BINARY_DIR}/Base/include/FasTC/BaseConfig.h"
  DESTINATION ${INCLUDE_INSTALL_DIR}/FasTC COMPONENT dev)

IF( NOT WIN32 AND NOT APPLE )
  TARGET_LINK_LIBRARIES( FasTCBase rt )
ENDIF()
