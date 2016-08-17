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

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/ASTCEncoder/include)
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/ASTCEncoder/include)
INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/ASTCEncoder/src)

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/Base/include )
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/Base/include )

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/GTest/include)

SET(TESTS
  IntegerEncoding
)

FOREACH(TEST ${TESTS})
  SET(TEST_NAME Test_ASTCEncoder_${TEST})
  SET(TEST_MODULE Test${TEST}.cpp)

  # HACK for MSVC 2012...
  IF(MSVC)
    ADD_DEFINITIONS(-D_VARIADIC_MAX=10)
  ENDIF()

  ADD_EXECUTABLE(${TEST_NAME} ${TEST_MODULE})

  TARGET_LINK_LIBRARIES(${TEST_NAME} FasTCBase)
  TARGET_LINK_LIBRARIES(${TEST_NAME} ASTCEncoder)
  TARGET_LINK_LIBRARIES(${TEST_NAME} gtest_main)
  ADD_TEST(${TEST_NAME} ${TEST_NAME})
ENDFOREACH()

IF( PNG_FOUND )

  # Setup the test that makes sure that we don't have any
  # regressions in the ASTC decoder.
  SET(TEST_NAME Test_ASTCEncoder_Decompress)

  # Copy the .pvr files that we will use for testing...
  SET(TEST_IMAGES 4x4 12x12 8x8 6x5 10x8 )
  FOREACH(IMAGE ${TEST_IMAGES})
    FILE(
      COPY ${FasTC_SOURCE_DIR}/ASTCEncoder/test/data/mandrill_${IMAGE}.astc
      DESTINATION ${CMAKE_BINARY_DIR}
      USE_SOURCE_PERMISSIONS
    )
  ENDFOREACH()

  FOREACH(IMAGE ${TEST_IMAGES})
    FILE(
      COPY ${FasTC_SOURCE_DIR}/ASTCEncoder/test/data/mandrill_decompressed_${IMAGE}.png
      DESTINATION ${CMAKE_BINARY_DIR}
      USE_SOURCE_PERMISSIONS
	)
  ENDFOREACH()

  INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/IO/include )
  INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/IO/include )
  INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/Core/include )
  INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/Core/include )

  # HACK for MSVC 2012...
  IF(MSVC)
    ADD_DEFINITIONS(-D_VARIADIC_MAX=10)
  ENDIF()

  # The cpp file to compile for the test
  ADD_EXECUTABLE(${TEST_NAME} TestASTCDecompression.cpp)

  # Libraries that we need...
  TARGET_LINK_LIBRARIES(${TEST_NAME} ASTCEncoder)
  TARGET_LINK_LIBRARIES(${TEST_NAME} gtest_main)
  TARGET_LINK_LIBRARIES(${TEST_NAME} FasTCBase)
  TARGET_LINK_LIBRARIES(${TEST_NAME} FasTCIO)
  TARGET_LINK_LIBRARIES(${TEST_NAME} FasTCCore)

  ADD_TEST(NAME ${TEST_NAME} WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND ${TEST_NAME})

ENDIF( PNG_FOUND )