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

#include "FasTC/ETCCompressor.h"

#include "rg_etc1.h"

namespace ETCC {

  void Decompress(const FasTC::DecompressionJob &cj) {

    uint32 blocksX = cj.Width() / 4;
    uint32 blocksY = cj.Height() / 4;

    for(uint32 j = 0; j < blocksY; j++) {
      for(uint32 i = 0; i < blocksX; i++) {
        uint32 pixels[16];
        uint32 blockIdx = j*blocksX + i;
        rg_etc1::unpack_etc1_block(cj.InBuf() + blockIdx * 8, pixels);
        for(uint32 y = 0; y < 4; y++)
        for(uint32 x = 0; x < 4; x++) {
          uint32 *out = reinterpret_cast<uint32 *>(cj.OutBuf());
          out[(j*4 + y)*cj.Width() + (i*4 + x)] = pixels[y*4 + x];
        }
      }
    }
  }

}  // namespace PVRTCC
