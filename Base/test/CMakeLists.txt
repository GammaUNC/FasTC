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

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/Base/include )
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/Base/include )

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/GTest/include)

SET(TESTS
  Vector Matrix Pixel Image Color Bits
)

FOREACH(TEST ${TESTS})
  SET(TEST_NAME Test_Base_${TEST})
  SET(TEST_MODULE Test${TEST}.cpp)

  # HACK for MSVC 2012...
  IF(MSVC)
    ADD_DEFINITIONS(-D_VARIADIC_MAX=10)
  ENDIF()

  ADD_EXECUTABLE(${TEST_NAME} ${TEST_MODULE})

  # Vector tests need to use uninitialized variables
  IF((${TEST_NAME} STREQUAL "Test_Base_Vector")
      OR
     (${TEST_NAME} STREQUAL "Test_Base_Matrix"))
    IF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUXX)
      SET_TARGET_PROPERTIES(
        ${TEST_NAME}
        PROPERTIES
        COMPILE_FLAGS
        "-Wno-uninitialized")
    ENDIF()
  ENDIF()

  TARGET_LINK_LIBRARIES(${TEST_NAME} FasTCBase)
  TARGET_LINK_LIBRARIES(${TEST_NAME} gtest_main)
  ADD_TEST(${TEST_NAME} ${TEST_NAME})
ENDFOREACH()
