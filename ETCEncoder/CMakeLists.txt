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

SET( LIBRARY_HEADERS
  "include/FasTC/ETCCompressor.h"
)

SET( HEADERS
  ${LIBRARY_HEADERS}
  src/rg_etc1.h
)

SET( SOURCES
  src/Compressor.cpp
  src/Decompressor.cpp
  src/rg_etc1.cpp
)

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/Base/include)
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/Base/include)
INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/ETCEncoder/include)
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/ETCEncoder/include)

ADD_LIBRARY( ETCEncoder
  ${HEADERS}
  ${SOURCES}
)

INSTALL(TARGETS ETCEncoder EXPORT FasTCTargets ARCHIVE DESTINATION lib COMPONENT lib)
INSTALL(
  FILES ${LIBRARY_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/FasTC
  COMPONENT dev)

TARGET_LINK_LIBRARIES( ETCEncoder FasTCBase )
