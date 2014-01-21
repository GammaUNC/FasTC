# FasTC
# Copyright (c) 2012 University of North Carolina at Chapel Hill. All rights reserved.
#
# Permission to use, copy, modify, and distribute this software and its documentation for educational, 
# research, and non-profit purposes, without fee, and without a written agreement is hereby granted, 
# provided that the above copyright notice, this paragraph, and the following four paragraphs appear 
# in all copies.
#
# Permission to incorporate this software into commercial products may be obtained by contacting the 
# authors or the Office of Technology Development at the University of North Carolina at Chapel Hill <otd@unc.edu>.
#
# This software program and documentation are copyrighted by the University of North Carolina at Chapel Hill. 
# The software program and documentation are supplied "as is," without any accompanying services from the 
# University of North Carolina at Chapel Hill or the authors. The University of North Carolina at Chapel Hill 
# and the authors do not warrant that the operation of the program will be uninterrupted or error-free. The 
# end-user understands that the program was developed for research purposes and is advised not to rely 
# exclusively on the program for any reason.
#
# IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE AUTHORS BE LIABLE TO ANY PARTY FOR 
# DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE 
# USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE 
# AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, 
# BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
# STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY 
# OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
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
