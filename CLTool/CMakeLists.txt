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

INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/Base/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/Base/include )

INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/Core/include )
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/IO/include )

ADD_EXECUTABLE(
  tc
  "src/tc.cpp"
)

ADD_EXECUTABLE(
  compare
  "src/compare.cpp"
)

ADD_EXECUTABLE(
  decomp
  "src/decomp.cpp"
)

# Add flag for link time code generation. This was used to build the libpng
# libraries, so we should probably also include it for this project as well...
IF( MSVC )
  SET_TARGET_PROPERTIES(tc PROPERTIES LINK_FLAGS "/LTCG")
  SET_TARGET_PROPERTIES(compare PROPERTIES LINK_FLAGS "/LTCG")
  SET_TARGET_PROPERTIES(decomp PROPERTIES LINK_FLAGS "/LTCG")
ENDIF()

TARGET_LINK_LIBRARIES( tc FasTCBase )
TARGET_LINK_LIBRARIES( tc FasTCIO )
TARGET_LINK_LIBRARIES( tc FasTCCore )

TARGET_LINK_LIBRARIES( compare FasTCBase )
TARGET_LINK_LIBRARIES( compare FasTCIO )

TARGET_LINK_LIBRARIES( decomp FasTCBase )
TARGET_LINK_LIBRARIES( decomp FasTCIO )

INSTALL(TARGETS tc compare decomp EXPORT FasTCTargets
  RUNTIME DESTINATION bin COMPONENT bin)
