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

#ifndef ASTCENCODER_SRC_UTILS_H_
#define ASTCENCODER_SRC_UTILS_H_

#include "ASTCCompressor.h"

#include "TexCompTypes.h"
#include "CompressionFormat.h"
#include "Pixel.h"

namespace ASTCC {

  static inline uint32 GetBlockHeight(FasTC::ECompressionFormat fmt) {
    switch(fmt) {
      case FasTC::eCompressionFormat_ASTC4x4: return 4;
      case FasTC::eCompressionFormat_ASTC5x4: return 4;
      case FasTC::eCompressionFormat_ASTC5x5: return 5;
      case FasTC::eCompressionFormat_ASTC6x5: return 5;
      case FasTC::eCompressionFormat_ASTC6x6: return 6;
      case FasTC::eCompressionFormat_ASTC8x5: return 5;
      case FasTC::eCompressionFormat_ASTC8x6: return 6;
      case FasTC::eCompressionFormat_ASTC8x8: return 8;
      case FasTC::eCompressionFormat_ASTC10x5: return 5;
      case FasTC::eCompressionFormat_ASTC10x6: return 6;
      case FasTC::eCompressionFormat_ASTC10x8: return 8;
      case FasTC::eCompressionFormat_ASTC10x10: return 10;
      case FasTC::eCompressionFormat_ASTC12x10: return 10;
      case FasTC::eCompressionFormat_ASTC12x12: return 12;
      default: assert(false); return -1;
    }
    assert(false);
    return -1;
  };

  static inline uint32 GetBlockWidth(FasTC::ECompressionFormat fmt) {
    switch(fmt) {
      case FasTC::eCompressionFormat_ASTC4x4: return 4;
      case FasTC::eCompressionFormat_ASTC5x4: return 5;
      case FasTC::eCompressionFormat_ASTC5x5: return 5;
      case FasTC::eCompressionFormat_ASTC6x5: return 6;
      case FasTC::eCompressionFormat_ASTC6x6: return 6;
      case FasTC::eCompressionFormat_ASTC8x5: return 8;
      case FasTC::eCompressionFormat_ASTC8x6: return 8;
      case FasTC::eCompressionFormat_ASTC8x8: return 8;
      case FasTC::eCompressionFormat_ASTC10x5: return 10;
      case FasTC::eCompressionFormat_ASTC10x6: return 10;
      case FasTC::eCompressionFormat_ASTC10x8: return 10;
      case FasTC::eCompressionFormat_ASTC10x10: return 10;
      case FasTC::eCompressionFormat_ASTC12x10: return 12;
      case FasTC::eCompressionFormat_ASTC12x12: return 12;
      default: assert(false); return -1;
    }
    assert(false);
    return -1;
  };

  // Count the number of bits set in a number.
  static inline uint32 Popcnt(uint32 n) {
    uint32 c;
    for(c = 0; n; c++) {
      n &= n-1;
    }
    return c;
  }

  // Transfers a bit as described in C.2.14
  static inline void BitTransferSigned(int32 &a, int32 &b) {
    b >>= 1;
    b |= a & 0x80;
    a >>= 1;
    a &= 0x3F;
    if(a & 0x20)
      a -= 0x40;
  }

  // Adds more precision to the blue channel as described
  // in C.2.14
  static inline FasTC::Pixel BlueContract(int32 a, int32 r, int32 g, int32 b) {
    return FasTC::Pixel(a, (r + b) >> 1, (g + b) >> 1, b);
  }

  // Partition selection functions as specified in
  // C.2.21
  static inline uint32 hash52(uint32 p) {
    p ^= p >> 15;  p -= p << 17;  p += p << 7; p += p << 4;
    p ^= p >>  5;  p += p << 16;  p ^= p >> 7; p ^= p >> 3;
    p ^= p <<  6;  p ^= p >> 17;
    return p;
  }

  static int32 SelectPartition(int32 seed, int32 x, int32 y, int32 z,
                               int32 partitionCount, int32 smallBlock) {
    if(1 == partitionCount)
      return 0;

    if(smallBlock) {
      x <<= 1;
      y <<= 1;
      z <<= 1;
    }

    seed += (partitionCount-1) * 1024;

    uint32 rnum = hash52(seed);
    uint8 seed1  =  rnum        & 0xF;
    uint8 seed2  = (rnum >>  4) & 0xF;
    uint8 seed3  = (rnum >>  8) & 0xF;
    uint8 seed4  = (rnum >> 12) & 0xF;
    uint8 seed5  = (rnum >> 16) & 0xF;
    uint8 seed6  = (rnum >> 20) & 0xF;
    uint8 seed7  = (rnum >> 24) & 0xF;
    uint8 seed8  = (rnum >> 28) & 0xF;
    uint8 seed9  = (rnum >> 18) & 0xF;
    uint8 seed10 = (rnum >> 22) & 0xF;
    uint8 seed11 = (rnum >> 26) & 0xF;
    uint8 seed12 = ((rnum >> 30) | (rnum << 2)) & 0xF;

    seed1 *= seed1;     seed2 *= seed2;
    seed3 *= seed3;     seed4 *= seed4;
    seed5 *= seed5;     seed6 *= seed6;
    seed7 *= seed7;     seed8 *= seed8;
    seed9 *= seed9;     seed10 *= seed10;
    seed11 *= seed11;   seed12 *= seed12;

    int32 sh1, sh2, sh3;
    if(seed & 1) {
      sh1 = (seed & 2)? 4 : 5;
      sh2 = (partitionCount == 3)? 6 : 5;
    } else {
      sh1 = (partitionCount == 3)? 6 : 5;
      sh2 = (seed & 2)? 4 : 5;
    }
    sh3 = (seed & 0x10) ? sh1 : sh2;

    seed1 >>= sh1; seed2  >>= sh2; seed3  >>= sh1; seed4  >>= sh2;
    seed5 >>= sh1; seed6  >>= sh2; seed7  >>= sh1; seed8  >>= sh2;
    seed9 >>= sh3; seed10 >>= sh3; seed11 >>= sh3; seed12 >>= sh3;

    int32 a = seed1*x + seed2*y + seed11*z + (rnum >> 14);
    int32 b = seed3*x + seed4*y + seed12*z + (rnum >> 10);
    int32 c = seed5*x + seed6*y + seed9 *z + (rnum >>  6);
    int32 d = seed7*x + seed8*y + seed10*z + (rnum >>  2);

    a &= 0x3F; b &= 0x3F; c &= 0x3F; d &= 0x3F;

    if( partitionCount < 4 ) d = 0;
    if( partitionCount < 3 ) c = 0;

    if( a >= b && a >= c && a >= d ) return 0;
    else if( b >= c && b >= d ) return 1;
    else if( c >= d ) return 2;
    return 3;
  }

  static inline int32 Select2DPartition(int32 seed, int32 x, int32 y,
                                        int32 partitionCount, int32 smallBlock) {
    return SelectPartition(seed, x, y, 0, partitionCount, smallBlock);
  }

  static inline int32 SelectSmall2DPartition(int32 seed, int32 x, int32 y,
                                             int32 partitionCount) {
    return Select2DPartition(seed, x, y, partitionCount, 1);
  }

  static inline int32 SelectLarge2DPartition(int32 seed, int32 x, int32 y,
                                             int32 partitionCount) {
    return Select2DPartition(seed, x, y, partitionCount, 0);
  }
}  // namespace ASTCC

#endif  // ASTCENCODER_SRC_UTILS_H_
