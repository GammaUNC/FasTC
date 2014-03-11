/* FasTC
 * Copyright (c) 2014 University of North Carolina at Chapel Hill.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for educational, research, and non-profit purposes, without
 * fee, and without a written agreement is hereby granted, provided that the
 * above copyright notice, this paragraph, and the following four paragraphs
 * appear in all copies.
 *
 * Permission to incorporate this software into commercial products may be
 * obtained by contacting the authors or the Office of Technology Development
 * at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of
 * North Carolina at Chapel Hill. The software program and documentation are
 * supplied "as is," without any accompanying services from the University of
 * North Carolina at Chapel Hill or the authors. The University of North
 * Carolina at Chapel Hill and the authors do not warrant that the operation of
 * the program will be uninterrupted or error-free. The end-user understands
 * that the program was developed for research purposes and is advised not to
 * rely exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE
 * AUTHORS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF
 * THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA
 * AT CHAPEL HILL OR THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY
 * DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON
 * AN "AS IS" BASIS, AND THE UNIVERSITY  OF NORTH CAROLINA AT CHAPEL HILL AND
 * THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
 * ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Please send all BUG REPORTS to <pavel@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Pavel Krajcevski
 * Dept of Computer Science
 * 201 S Columbia St
 * Frederick P. Brooks, Jr. Computer Science Bldg
 * Chapel Hill, NC 27599-3175
 * USA
 * 
 * <http://gamma.cs.unc.edu/FasTC/>
 */

#ifndef _IO_SRC_GL_DEFINES_H_
#define _IO_SRC_GL_DEFINES_H_

#ifdef OPENGL_FOUND
# ifdef __APPLE__
#  include <OpenGL/gl.h>
# else
#  include <GL/gl.h>
# endif
#endif

#ifndef GL_VERSION_1_1

#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_INT                            0x1404
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406
#define GL_DOUBLE                         0x140A
#define GL_RGBA8                          0x8058

#endif  // GL_VERSION_1_1

#ifndef GL_VERTSION_4_2

#define GL_COMPRESSED_RGBA_BPTC_UNORM     0x8E8C

#endif  // GL_VERTSION_4_2

////////////////////////////////////////////////////////////////////////////////
//
// PVRTC definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef COMPRESSED_RGB_PVRTC_4BPPV1_IMG
#define COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#endif

#ifndef COMPRESSED_RGB_PVRTC_2BPPV1_IMG
#define COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#endif

#ifndef COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
#define COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#endif

#ifndef COMPRESSED_RGBA_PVRTC_2BPPV1_IMG
#define COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
#endif

////////////////////////////////////////////////////////////////////////////////
//
// ASTC definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef COMPRESSED_RGBA_ASTC_4x4_KHR
#define COMPRESSED_RGBA_ASTC_4x4_KHR 0x93B0
#endif

#ifndef COMPRESSED_RGBA_ASTC_5x4_KHR
#define COMPRESSED_RGBA_ASTC_5x4_KHR 0x93B1
#endif

#ifndef COMPRESSED_RGBA_ASTC_5x5_KHR
#define COMPRESSED_RGBA_ASTC_5x5_KHR 0x93B2
#endif

#ifndef COMPRESSED_RGBA_ASTC_6x5_KHR
#define COMPRESSED_RGBA_ASTC_6x5_KHR 0x93B3
#endif

#ifndef COMPRESSED_RGBA_ASTC_6x6_KHR
#define COMPRESSED_RGBA_ASTC_6x6_KHR 0x93B4
#endif

#ifndef COMPRESSED_RGBA_ASTC_8x5_KHR
#define COMPRESSED_RGBA_ASTC_8x5_KHR 0x93B5
#endif

#ifndef COMPRESSED_RGBA_ASTC_8x6_KHR
#define COMPRESSED_RGBA_ASTC_8x6_KHR 0x93B6
#endif

#ifndef COMPRESSED_RGBA_ASTC_8x8_KHR
#define COMPRESSED_RGBA_ASTC_8x8_KHR 0x93B7
#endif

#ifndef COMPRESSED_RGBA_ASTC_10x5_KHR
#define COMPRESSED_RGBA_ASTC_10x5_KHR 0x93B8
#endif

#ifndef COMPRESSED_RGBA_ASTC_10x6_KHR
#define COMPRESSED_RGBA_ASTC_10x6_KHR 0x93B9
#endif

#ifndef COMPRESSED_RGBA_ASTC_10x8_KHR
#define COMPRESSED_RGBA_ASTC_10x8_KHR 0x93BA
#endif

#ifndef COMPRESSED_RGBA_ASTC_10x10_KHR
#define COMPRESSED_RGBA_ASTC_10x10_KHR 0x93BB
#endif

#ifndef COMPRESSED_RGBA_ASTC_12x10_KHR
#define COMPRESSED_RGBA_ASTC_12x10_KHR 0x93BC
#endif

#ifndef COMPRESSED_RGBA_ASTC_12x12_KHR
#define COMPRESSED_RGBA_ASTC_12x12_KHR 0x93BD
#endif

#endif
  // _IO_SRC_GL_DEFINES_H_
