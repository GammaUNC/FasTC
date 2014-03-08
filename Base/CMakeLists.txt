# FasTC
# Copyright (c) 2013 University of North Carolina at Chapel Hill.
# All rights reserved.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for educational, research, and non-profit purposes, without
# fee, and without a written agreement is hereby granted, provided that the
# above copyright notice, this paragraph, and the following four paragraphs
# appear in all copies.
#
# Permission to incorporate this software into commercial products may be
# obtained by contacting the authors or the Office of Technology Development
# at the University of North Carolina at Chapel Hill <otd@unc.edu>.
#
# This software program and documentation are copyrighted by the University of
# North Carolina at Chapel Hill. The software program and documentation are
# supplied "as is," without any accompanying services from the University of
# North Carolina at Chapel Hill or the authors. The University of North
# Carolina at Chapel Hill and the authors do not warrant that the operation of
# the program will be uninterrupted or error-free. The end-user understands
# that the program was developed for research purposes and is advised not to
# rely exclusively on the program for any reason.
#
# IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE
# AUTHORS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL,
# OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF
# THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA
# AT CHAPEL HILL OR THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.
#
# THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY
# DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
# STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON
# AN "AS IS" BASIS, AND THE UNIVERSITY  OF NORTH CAROLINA AT CHAPEL HILL AND
# THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
# ENHANCEMENTS, OR MODIFICATIONS.
#
# Please send all BUG REPORTS to <pavel@cs.unc.edu>.
#
# The authors may be contacted via:
#
# Pavel Krajcevski
# Dept of Computer Science
# 201 S Columbia St
# Frederick P. Brooks, Jr. Computer Science Bldg
# Chapel Hill, NC 27599-3175
# USA
# 
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
  "config/FasTCBaseConfig.h.in"
  "include/FasTCBaseConfig.h"
)

SET( SOURCES
  "src/Image.cpp"
  "src/CompressionJob.cpp"
  "src/Pixel.cpp"
  "src/IPixel.cpp"
  "src/Color.cpp"
)

SET( HEADERS
  "include/Bits.h"
  "include/BitStream.h"
  "include/Color.h"
  "include/CompressionJob.h"
  "config/FasTCBaseConfig.h.in"
  "include/IPixel.h"
  "include/Image.h"
  "include/MatrixBase.h"
  "include/MatrixSquare.h"
  "include/Matrix3x3.h"
  "include/Pixel.h"
  "include/ScopedAllocator.h"
  "include/TexCompTypes.h"
  "include/VectorBase.h"
  "include/Vector2.h"
  "include/Vector3.h"
  "include/Vector4.h"
)

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/Base/include)
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/Base/include)

ADD_LIBRARY( FasTCBase
	${HEADERS}
	${SOURCES}
)

IF( NOT WIN32 AND NOT APPLE )
  	TARGET_LINK_LIBRARIES( FasTCBase rt )
ENDIF()
