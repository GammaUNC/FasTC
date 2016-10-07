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

#include "FasTC/Pixel.h"

namespace {
  void DecompressDXT1Block(const uint8 *block, uint32 *outBuf, bool check_order) {
    // When we call FasTC::Pixel::FromBits, we expect the bits
    // to be read out of memory in LSB (byte) order first. Hence,
    // we can't read the blocks directly as uint16 values out of
    // the DXT buffer and we have to swap the bytes before hand.
    uint16 colorA = block[1] | block[0] << 8;
    uint16 colorB = block[3] | block[2] << 8;

    uint32 mod = reinterpret_cast<const uint32 *>(block + 4)[0];

    uint8 kFiveSixFive[4] = { 0, 5, 6, 5 };
    FasTC::Pixel a, b, c, d;
    a.FromBits(reinterpret_cast<const uint8 *>(&colorA), kFiveSixFive);
    b.FromBits(reinterpret_cast<const uint8 *>(&colorB), kFiveSixFive);

    uint8 kFullDepth[4] = { 8, 8, 8, 8 };
    a.ChangeBitDepth(kFullDepth);
    b.ChangeBitDepth(kFullDepth);

    // However, for the purposes of properly decoding DXT, we can read them as ints...
    const uint16 *block_ptr = reinterpret_cast<const uint16 *>(block);
    if (!check_order || block_ptr[0] > block_ptr[1]) {
      c = (a * 2 + b) / 3;
      d = (a + b * 2) / 3;
    }
    else {
      c = (a + b) / 2;
      // d already initialized to zero...
    }

    FasTC::Pixel *colors[4] = { &a, &b, &c, &d };

    uint32 *outPixels = reinterpret_cast<uint32 *>(outBuf);
    for (uint32 i = 0; i < 16; i++) {
      outPixels[i] &= 0xFF000000;
      outPixels[i] |= colors[(mod >> (i * 2)) & 3]->Pack() & 0x00FFFFFF;
    }
  }

  void DecompressDXT5Block(const uint8 *block, uint32 *outBuf) {
    int alpha0 = block[0];
    int alpha1 = block[1];

    int palette[8];
    palette[0] = alpha0;
    palette[1] = alpha1;

    if (alpha0 > alpha1) {
      for (int i = 2; i < 8; ++i) {
        palette[i] = ((8 - i) * alpha0 + (i - 1) * alpha1) / 7;
      }
    } else {
      for (int i = 2; i < 6; ++i) {
        palette[i] = ((6 - i) * alpha0 + (i - 1) * alpha1) / 5;
      }
      palette[6] = 0;
      palette[7] = 255;
    }

    uint64 mod = *reinterpret_cast<const uint64 *>(block) >> 16;
    uint32 *outPixels = reinterpret_cast<uint32 *>(outBuf);
    for (uint32 i = 0; i < 16; i++) {
      outPixels[i] &= 0x00FFFFFF;
      outPixels[i] |= palette[(mod >> (i * 3)) & 7] << 24;
    }
  }
}  // namespace

namespace DXTC
{
  void DecompressDXT1(const FasTC::DecompressionJob &dcj) {
    uint32 blockW = (dcj.Width() + 3) >> 2;
    uint32 blockH = (dcj.Height() + 3) >> 2;

    const uint32 blockSz = GetBlockSize(FasTC::eCompressionFormat_DXT1);

    uint32 *outPixels = reinterpret_cast<uint32 *>(dcj.OutBuf());

    uint32 outBlock[16];
    memset(outBlock, 0xFF, sizeof(outBlock));

    for(uint32 j = 0; j < blockH; j++) {
      for(uint32 i = 0; i < blockW; i++) {

        uint32 offset = (j * blockW + i) * blockSz;
        DecompressDXT1Block(dcj.InBuf() + offset, outBlock, true);

        uint32 decompWidth = std::min(4U, dcj.Width() - i * 4);
        uint32 decompHeight = std::min(4U, dcj.Height() - j * 4);

        for(uint32 y = 0; y < decompHeight; y++)
        for(uint32 x = 0; x < decompWidth; x++) {
          offset = (j*4 + y)*dcj.Width() + ((i*4)+x);
          outPixels[offset] = outBlock[y*4 + x];
        }
      }
    }
  }

  void DecompressDXT5(const FasTC::DecompressionJob &dcj) {
    uint32 blockW = (dcj.Width() + 3) >> 2;
    uint32 blockH = (dcj.Height() + 3) >> 2;

    const uint32 blockSz = GetBlockSize(FasTC::eCompressionFormat_DXT5);

    uint32 *outPixels = reinterpret_cast<uint32 *>(dcj.OutBuf());

    uint32 outBlock[16];
    memset(outBlock, 0xFF, sizeof(outBlock));

    for (uint32 j = 0; j < blockH; j++) {
      for (uint32 i = 0; i < blockW; i++) {

        uint32 offset = (j * blockW + i) * blockSz;
        DecompressDXT5Block(dcj.InBuf() + offset, outBlock);
        DecompressDXT1Block(dcj.InBuf() + offset + blockSz / 2, outBlock, false);

        uint32 decompWidth = std::min(4U, dcj.Width() - i * 4);
        uint32 decompHeight = std::min(4U, dcj.Height() - j * 4);

        for (uint32 y = 0; y < decompHeight; y++)
        for (uint32 x = 0; x < decompWidth; x++) {
          offset = (j * 4 + y)*dcj.Width() + ((i * 4) + x);
          outPixels[offset] = outBlock[y * 4 + x];
        }
      }
    }
  }
}
