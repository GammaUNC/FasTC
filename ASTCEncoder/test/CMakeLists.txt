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


# Setup the test that makes sure that we don't have any
# regressions in the ASTC decoder.
SET(TEST_NAME Test_ASTCEncoder_Decompress)

# Copy the .pvr files that we will use for testing...
SET(TEST_IMAGES 4x4 12x12 8x8 6x5 10x8 )
FOREACH(IMAGE ${TEST_IMAGES})
  FILE(COPY
    ${FasTC_SOURCE_DIR}/ASTCEncoder/test/data/mandrill_${IMAGE}.astc
    DESTINATION ${CMAKE_BINARY_DIR}
    USE_SOURCE_PERMISSIONS
    )
ENDFOREACH()

FOREACH(IMAGE ${TEST_IMAGES})
  FILE(COPY
    ${FasTC_SOURCE_DIR}/ASTCEncoder/test/data/mandrill_decompressed_${IMAGE}.png
    DESTINATION ${CMAKE_BINARY_DIR}
    USE_SOURCE_PERMISSIONS
    )
ENDFOREACH()

INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/IO/include )
INCLUDE_DIRECTORIES( ${FasTC_BINARY_DIR}/IO/include )
INCLUDE_DIRECTORIES( ${FasTC_SOURCE_DIR}/Core/include )

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
