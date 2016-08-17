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

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_LIST_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_LIST_DIR}/arvo)

SET( HEADERS
  avpcl.h
  bits.h
  endpts.h
  ImfArray.h
  rgba.h
  shapes_three.h
  shapes_two.h
  targa.h
  tile.h
  utils.h
  arvo/ArvoMath.h
  arvo/Char.h
  arvo/Complex.h
  arvo/form.h
  arvo/Matrix.h
  arvo/Perm.h
  arvo/Rand.h
  arvo/SI_units.h
  arvo/SphTri.h
  arvo/SVD.h
  arvo/Token.h
  arvo/Vec2.h
  arvo/Vec3.h
  arvo/Vec4.h
  arvo/Vector.h
)

SET( SOURCES
  avpcl.cpp
  avpcl_mode0.cpp
  avpcl_mode1.cpp
  avpcl_mode2.cpp
  avpcl_mode3.cpp
  avpcl_mode4.cpp
  avpcl_mode5.cpp
  avpcl_mode6.cpp
  avpcl_mode7.cpp
  targa.cpp
  utils.cpp
  arvo/ArvoMath.cpp
  arvo/Char.cpp
  arvo/Complex.cpp
  arvo/Matrix.cpp
  arvo/Perm.cpp
  arvo/Rand.cpp
  arvo/SphTri.cpp
  arvo/SVD.cpp
  arvo/Token.cpp
  arvo/Vec2.cpp
  arvo/Vec3.cpp
  arvo/Vec4.cpp
  arvo/Vector.cpp
)

ADD_LIBRARY( avpcl
  ${HEADERS}
  ${SOURCES}
)
