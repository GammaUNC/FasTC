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

#include "FasTC/DXTCompressor.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>

#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"

namespace DXTC
{
  // Function prototypes
  void ExtractBlock(const uint32* inPtr, uint32 width, uint8* colorBlock);

  // Extract a 4 by 4 block of pixels from inPtr and store it in colorBlock. The width parameter
  // specifies the size of the image in pixels.
  void ExtractBlock(const uint32* inPtr, uint32 width, uint8* colorBlock)
  {
    for (int j = 0; j < 4; j++)
    {
      memcpy(&colorBlock[j * 4 * 4], inPtr, 4 * 4);
      inPtr += width;
    }
  }

  // Compress an image using DXT1 compression. Use the inBuf parameter to point to an image in
  // 4-byte RGBA format. The width and height parameters specify the size of the image in pixels.
  // The buffer pointed to by outBuf should be large enough to store the compressed image. This
  // implementation has an 8:1 compression ratio.
  void CompressImageDXT1(const FasTC::CompressionJob &cj) {
    uint8 block[64];

    const uint32 kBlockSz = GetBlockSize(FasTC::eCompressionFormat_DXT1);
    const uint32 startBlock = cj.CoordsToBlockIdx(cj.XStart(), cj.YStart());
    uint8 *outBuf = cj.OutBuf() + startBlock * kBlockSz;

    const uint32 *inPixels = reinterpret_cast<const uint32 *>(cj.InBuf());
    uint32 endY = std::min(cj.YEnd(), cj.Height() - 4);
    uint32 startX = cj.XStart();
    for(uint32 j = cj.YStart(); j <= endY; j += 4) {
      const uint32 endX = j == cj.YEnd()? cj.XEnd() : cj.Width();
      for(uint32 i = startX; i < endX; i += 4) {

        const uint32 kOffset = j*cj.Width() + i;
        ExtractBlock(inPixels + kOffset, cj.Width(), block);
        stb_compress_dxt_block(outBuf, block, 0, STB_DXT_DITHER);
        outBuf += 8;
      }
      startX = 0;
    }
  }

  // Compress an image using DXT5 compression. Use the inBuf parameter to point to an image in
  // 4-byte RGBA format. The width and height parameters specify the size of the image in pixels.
  // The buffer pointed to by outBuf should be large enough to store the compressed image. This
  // implementation has an 4:1 compression ratio.
  void CompressImageDXT5(const FasTC::CompressionJob &cj) {
    uint8 block[64];

    const uint32 kBlockSz = GetBlockSize(FasTC::eCompressionFormat_DXT5);
    const uint32 startBlock = cj.CoordsToBlockIdx(cj.XStart(), cj.YStart());
    uint8 *outBuf = cj.OutBuf() + startBlock * kBlockSz;

    const uint32 *inPixels = reinterpret_cast<const uint32 *>(cj.InBuf());
    uint32 endY = std::min(cj.YEnd(), cj.Height() - 4);
    uint32 startX = cj.XStart();
    for(uint32 j = cj.YStart(); j <= endY; j += 4) {
      const uint32 endX = j == cj.YEnd()? cj.XEnd() : cj.Width();
      for(uint32 i = startX; i < endX; i += 4) {

        const uint32 kOffset = j*cj.Width() + i;
        ExtractBlock(inPixels + kOffset, cj.Width(), block);
        stb_compress_dxt_block(outBuf, block, 1, STB_DXT_DITHER);
        outBuf += 16;
      }
      startX = 0;
    }
  }
}

