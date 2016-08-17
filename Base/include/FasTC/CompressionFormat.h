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

#ifndef _BASE_INCLUDE_COMPRESSIONFORMAT_H_
#define _BASE_INCLUDE_COMPRESSIONFORMAT_H_

#include "FasTC/TexCompTypes.h"

namespace FasTC {

  // The different supported compression formats
  enum ECompressionFormat {
    eCompressionFormat_DXT1,
    eCompressionFormat_DXT5,
    eCompressionFormat_ETC1,
    eCompressionFormat_BPTC,

    eCompressionFormat_PVRTC2,
    eCompressionFormat_PVRTC4,
    COMPRESSION_FORMAT_PVRTC_BEGIN = eCompressionFormat_PVRTC2,
    COMPRESSION_FORMAT_PVRTC_END = eCompressionFormat_PVRTC4,

    eCompressionFormat_ASTC4x4,
    eCompressionFormat_ASTC5x4,
    eCompressionFormat_ASTC5x5,
    eCompressionFormat_ASTC6x5,
    eCompressionFormat_ASTC6x6,
    eCompressionFormat_ASTC8x5,
    eCompressionFormat_ASTC8x6,
    eCompressionFormat_ASTC8x8,
    eCompressionFormat_ASTC10x5,
    eCompressionFormat_ASTC10x6,
    eCompressionFormat_ASTC10x8,
    eCompressionFormat_ASTC10x10,
    eCompressionFormat_ASTC12x10,
    eCompressionFormat_ASTC12x12,
    COMPRESSION_FORMAT_ASTC_BEGIN = eCompressionFormat_ASTC4x4,
    COMPRESSION_FORMAT_ASTC_END = eCompressionFormat_ASTC12x12,

    kNumCompressionFormats
  };

  // Returns the dimensions of the blocks for the given format.
  inline static void GetBlockDimensions(ECompressionFormat fmt, uint32 (&outSz)[2]) {
    switch(fmt) {
      default:
      case eCompressionFormat_DXT1:
      case eCompressionFormat_DXT5:
      case eCompressionFormat_BPTC:
      case eCompressionFormat_PVRTC4:
      case eCompressionFormat_ETC1:
      case eCompressionFormat_ASTC4x4:
        outSz[0] = 4;
        outSz[1] = 4;
        break;

      case eCompressionFormat_PVRTC2:
        outSz[0] = 8;
        outSz[1] = 4;
        break;

      case eCompressionFormat_ASTC5x4:
        outSz[0] = 5;
        outSz[1] = 4;
        break;

      case eCompressionFormat_ASTC5x5:
        outSz[0] = 5;
        outSz[1] = 5;
        break;

      case eCompressionFormat_ASTC6x5:
        outSz[0] = 6;
        outSz[1] = 5;
        break;

      case eCompressionFormat_ASTC6x6:
        outSz[0] = 6;
        outSz[1] = 6;
        break;

      case eCompressionFormat_ASTC8x5:
        outSz[0] = 8;
        outSz[1] = 5;
        break;

      case eCompressionFormat_ASTC8x6:
        outSz[0] = 8;
        outSz[1] = 6;
        break;

      case eCompressionFormat_ASTC8x8:
        outSz[0] = 8;
        outSz[1] = 8;
        break;

      case eCompressionFormat_ASTC10x5:
        outSz[0] = 10;
        outSz[1] = 5;
        break;

      case eCompressionFormat_ASTC10x6:
        outSz[0] = 10;
        outSz[1] = 6;
        break;

      case eCompressionFormat_ASTC10x8:
        outSz[0] = 10;
        outSz[1] = 8;
        break;

      case eCompressionFormat_ASTC10x10:
        outSz[0] = 10;
        outSz[1] = 10;
        break;

      case eCompressionFormat_ASTC12x10:
        outSz[0] = 12;
        outSz[1] = 10;
        break;

      case eCompressionFormat_ASTC12x12:
        outSz[0] = 12;
        outSz[1] = 12;
        break;
    }
  }

  // Returns the size of the compressed block in bytes for the given format.
  inline static uint32 GetBlockSize(ECompressionFormat fmt) {
    switch(fmt) {
      default:
      case eCompressionFormat_DXT1:
      case eCompressionFormat_PVRTC4:
      case eCompressionFormat_PVRTC2:
      case eCompressionFormat_ETC1:
        return 8;

      case eCompressionFormat_DXT5:

      case eCompressionFormat_BPTC:

      case eCompressionFormat_ASTC4x4:
      case eCompressionFormat_ASTC5x4:
      case eCompressionFormat_ASTC5x5:
      case eCompressionFormat_ASTC6x5:
      case eCompressionFormat_ASTC6x6:
      case eCompressionFormat_ASTC8x5:
      case eCompressionFormat_ASTC8x6:
      case eCompressionFormat_ASTC8x8:
      case eCompressionFormat_ASTC10x5:
      case eCompressionFormat_ASTC10x6:
      case eCompressionFormat_ASTC10x8:
      case eCompressionFormat_ASTC10x10:
      case eCompressionFormat_ASTC12x10:
      case eCompressionFormat_ASTC12x12:
        return 16;
    }
  }
}  // namespace FasTC

#endif // _BASE_INCLUDE_COMPRESSIONFORMAT_H_
