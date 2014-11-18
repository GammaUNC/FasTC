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

SET( LIBRARY_HEADERS
  "include/FasTC/PVRTCCompressor.h"
)

SET( HEADERS
  ${LIBRARY_HEADERS}
  src/Block.h
  src/PVRTCImage.h
)

SET( SOURCES
  src/Compressor.cpp
  src/Decompressor.cpp
  src/Block.cpp
  src/PVRTCImage.cpp
)

OPTION(DEBUG_PVRTC_DECODER "Output intermediate images during PVRTC decoding." OFF)

CONFIGURE_FILE(
  "config/PVRTCDefines.h.in"
  "include/FasTC/PVRTCDefines.h"
)

IF( PVRTEXLIB_FOUND )
  INCLUDE_DIRECTORIES( ${PVRTEXLIB_INCLUDE_DIRS} )
  SET( SOURCES
    ${SOURCES}
    src/CompressorPVRLib.cpp
  )
ENDIF()

INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/PVRTCEncoder/include)
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/PVRTCEncoder/include)
INCLUDE_DIRECTORIES(${FasTC_SOURCE_DIR}/Base/include)
INCLUDE_DIRECTORIES(${FasTC_BINARY_DIR}/Base/include)

ADD_LIBRARY( PVRTCEncoder
  ${HEADERS}
  ${SOURCES}
)

INSTALL(TARGETS PVRTCEncoder EXPORT FasTCTargets ARCHIVE DESTINATION lib COMPONENT lib)
INSTALL(
  FILES ${LIBRARY_HEADERS} "${FasTC_BINARY_DIR}/PVRTCEncoder/include/FasTC/PVRTCDefines.h"
  DESTINATION ${INCLUDE_INSTALL_DIR}/FasTC
  COMPONENT dev)

TARGET_LINK_LIBRARIES( PVRTCEncoder FasTCBase )
IF( DEBUG_PVRTC_DECODER )
  TARGET_LINK_LIBRARIES( PVRTCEncoder FasTCIO )
ENDIF( DEBUG_PVRTC_DECODER )

IF( PVRTEXLIB_FOUND )
  TARGET_LINK_LIBRARIES( PVRTCEncoder ${PVRTEXLIB_LIBRARIES} )
ENDIF( PVRTEXLIB_FOUND )
