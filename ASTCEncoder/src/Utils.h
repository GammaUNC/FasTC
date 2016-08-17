// Copyright 2016 The University of North Carolina at Chapel Hill
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Please send all BUG REPORTS to <pavel@cs.unc.edu>.
// <http://gamma.cs.unc.edu/FasTC/>

#ifndef ASTCENCODER_SRC_UTILS_H_
#define ASTCENCODER_SRC_UTILS_H_

#include "FasTC/ASTCCompressor.h"

#include "FasTC/TexCompTypes.h"
#include "FasTC/CompressionFormat.h"
#include "FasTC/Pixel.h"

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
      default: assert(false); return static_cast<uint32>(-1);
    }
    assert(false);
    return static_cast<uint32>(-1);
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
      default: assert(false); return static_cast<uint32>(-1);
    }
    assert(false);
    return static_cast<uint32>(-1);
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
    return FasTC::Pixel(
            static_cast<int16>(a),
            static_cast<int16>((r + b) >> 1),
            static_cast<int16>((g + b) >> 1),
            static_cast<int16>(b));
  }

  // Partition selection functions as specified in
  // C.2.21
  static inline uint32 hash52(uint32 p) {
    p ^= p >> 15;  p -= p << 17;  p += p << 7; p += p << 4;
    p ^= p >>  5;  p += p << 16;  p ^= p >> 7; p ^= p >> 3;
    p ^= p <<  6;  p ^= p >> 17;
    return p;
  }

  static uint32 SelectPartition(int32 seed, int32 x, int32 y, int32 z,
                               int32 partitionCount, int32 smallBlock) {
    if(1 == partitionCount)
      return 0;

    if(smallBlock) {
      x <<= 1;
      y <<= 1;
      z <<= 1;
    }

    seed += (partitionCount-1) * 1024;

    uint32 rnum = hash52(static_cast<uint32>(seed));
    uint8 seed1  = static_cast<uint8>(rnum        & 0xF);
    uint8 seed2  = static_cast<uint8>((rnum >>  4) & 0xF);
    uint8 seed3  = static_cast<uint8>((rnum >>  8) & 0xF);
    uint8 seed4  = static_cast<uint8>((rnum >> 12) & 0xF);
    uint8 seed5  = static_cast<uint8>((rnum >> 16) & 0xF);
    uint8 seed6  = static_cast<uint8>((rnum >> 20) & 0xF);
    uint8 seed7  = static_cast<uint8>((rnum >> 24) & 0xF);
    uint8 seed8  = static_cast<uint8>((rnum >> 28) & 0xF);
    uint8 seed9  = static_cast<uint8>((rnum >> 18) & 0xF);
    uint8 seed10 = static_cast<uint8>((rnum >> 22) & 0xF);
    uint8 seed11 = static_cast<uint8>((rnum >> 26) & 0xF);
    uint8 seed12 = static_cast<uint8>(((rnum >> 30) | (rnum << 2)) & 0xF);

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

  static inline uint32 Select2DPartition(int32 seed, int32 x, int32 y,
                                        int32 partitionCount, int32 smallBlock) {
    return SelectPartition(seed, x, y, 0, partitionCount, smallBlock);
  }

  static inline uint32 SelectSmall2DPartition(int32 seed, int32 x, int32 y,
                                             int32 partitionCount) {
    return Select2DPartition(seed, x, y, partitionCount, 1);
  }

  static inline uint32 SelectLarge2DPartition(int32 seed, int32 x, int32 y,
                                             int32 partitionCount) {
    return Select2DPartition(seed, x, y, partitionCount, 0);
  }
}  // namespace ASTCC

#endif  // ASTCENCODER_SRC_UTILS_H_
