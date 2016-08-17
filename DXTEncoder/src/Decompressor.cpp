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

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "FasTC/Pixel.h"

namespace DXTC
{
  void DecompressDXT1Block(const uint8 *block, uint32 *outBuf) {
    // When we call FasTC::Pixel::FromBits, we expect the bits
    // to be read out of memory in LSB (byte) order first. Hence,
    // we can't read the blocks directly as uint16 values out of
    // the DXT buffer and we have to swap the bytes before hand.
    uint16 colorA = block[0];
    colorA <<= 8;
    colorA |= block[1];

    uint16 colorB = block[2];
    colorB <<= 8;
    colorB |= block[3];

    uint32 mod = reinterpret_cast<const uint32 *>(block + 4)[0];

    uint8 kFiveSixFive[4] = { 0, 5, 6, 5 };
    FasTC::Pixel a, b, c, d;
    a.FromBits(reinterpret_cast<const uint8 *>(&colorA), kFiveSixFive);
    b.FromBits(reinterpret_cast<const uint8 *>(&colorB), kFiveSixFive);

    uint8 kFullDepth[4] = {8, 8, 8, 8};
    a.ChangeBitDepth(kFullDepth);
    b.ChangeBitDepth(kFullDepth);

    d = (a + b*2) / 3;
    c = (a*2 + b) / 3;

    FasTC::Pixel *colors[4] = { &a, &b, &c, &d };

    uint32 *outPixels = reinterpret_cast<uint32 *>(outBuf);
    for(uint32 i = 0; i < 16; i++) {
      outPixels[i] = colors[(mod >> (i*2)) & 3]->Pack();
    }
  }

  void DecompressDXT1(const FasTC::DecompressionJob &dcj)
  {
    assert(!(dcj.Height() & 3));
    assert(!(dcj.Width() & 3));

    uint32 blockW = dcj.Width() >> 2;
    uint32 blockH = dcj.Height() >> 2;

    const uint32 blockSz = GetBlockSize(FasTC::eCompressionFormat_DXT1);

    uint32 *outPixels = reinterpret_cast<uint32 *>(dcj.OutBuf());

    uint32 outBlock[16];
    for(uint32 j = 0; j < blockH; j++) {
      for(uint32 i = 0; i < blockW; i++) {

        uint32 offset = (j * blockW + i) * blockSz;
        DecompressDXT1Block(dcj.InBuf() + offset, outBlock);

        for(uint32 y = 0; y < 4; y++)
        for(uint32 x = 0; x < 4; x++) {
          offset = (j*4 + y)*dcj.Width() + ((i*4)+x);
          outPixels[offset] = outBlock[y*4 + x];
        }
      }
    }
  }
}
