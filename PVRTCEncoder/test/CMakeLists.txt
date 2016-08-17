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

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/PVRTCEncoder/include)
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/PVRTCEncoder/include)
INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/PVRTCEncoder/src)

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/Base/include )
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/Base/include )

IF( DEBUG_PVRTC_DECODER )
  INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/Core/include )
  INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/Core/include )
  INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/IO/include )
  INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/IO/include )
ENDIF( DEBUG_PVRTC_DECODER )

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/GTest/include)

SET(TESTS
  Block Image Decompressor
)

FOREACH(TEST ${TESTS})
  SET(TEST_NAME Test_PVRTCEncoder_${TEST})
  SET(TEST_MODULE ${TEST}Test.cpp)

  # HACK for MSVC 2012...
  IF(MSVC)
    ADD_DEFINITIONS(-D_VARIADIC_MAX=10)
  ENDIF()

  ADD_EXECUTABLE(${TEST_NAME} ${TEST_MODULE})
  TARGET_LINK_LIBRARIES(${TEST_NAME} PVRTCEncoder)
  TARGET_LINK_LIBRARIES(${TEST_NAME} gtest_main)
  ADD_TEST(${TEST_NAME} ${TEST_NAME})
ENDFOREACH()

# Test the decompressor against the included PVR Texture library....
IF(PVRTEXLIB_FOUND)

  SET(TEST_NAME Test_PVRTCEncoder_DecompVersusPVRLib)

  # Copy the .pvr files that we will use for testing...
  SET(TEST_IMAGES
    4bpp-gradient 4bpp-white 4bpp-gray 4bpp-transparent 4bpp-trans-gradient
    2bpp-gradient 2bpp-white 2bpp-gray 2bpp-transparent 2bpp-trans-gradient
  )
  FOREACH(IMAGE ${TEST_IMAGES})
    FILE(COPY
      ${FasTC_SOURCE_DIR}/PVRTCEncoder/test/data/${IMAGE}.pvr
      DESTINATION ${CMAKE_BINARY_DIR}
      USE_SOURCE_PERMISSIONS
    )
  ENDFOREACH()

  # Make sure to include the PVR library headers...
  INCLUDE_DIRECTORIES( ${PVRTEXLIB_INCLUDE_DIRS} )
  INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/Core/include )

  # HACK for MSVC 2012...
  IF(MSVC)
    ADD_DEFINITIONS(-D_VARIADIC_MAX=10)
  ENDIF()

  # The cpp file to compile for the test
  ADD_EXECUTABLE(${TEST_NAME} DecompTestPVR.cpp)

  # Libraries that we need...
  TARGET_LINK_LIBRARIES(${TEST_NAME} PVRTCEncoder)
  TARGET_LINK_LIBRARIES(${TEST_NAME} gtest_main)
  TARGET_LINK_LIBRARIES(${TEST_NAME} ${PVRTEXLIB_LIBRARIES} )
  TARGET_LINK_LIBRARIES(${TEST_NAME} FasTCBase)
  TARGET_LINK_LIBRARIES(${TEST_NAME} FasTCIO)
  TARGET_LINK_LIBRARIES(${TEST_NAME} FasTCCore)

  IF(MSVC)
    ADD_TEST(${TEST_NAME}
      ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}/Debug/${TEST_NAME}
    )
  ELSE()
    ADD_TEST(${TEST_NAME}
      ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}
    )
  ENDIF()

ENDIF(PVRTEXLIB_FOUND)
