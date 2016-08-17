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
#include <algorithm>
#include <cstring>

namespace ETCC {

  void Compress_RG(const FasTC::CompressionJob &cj) {

    rg_etc1::etc1_pack_params params;
    params.m_quality = rg_etc1::cLowQuality;
    rg_etc1::pack_etc1_block_init();

    uint32 kBlockSz = GetBlockSize(FasTC::eCompressionFormat_ETC1);
    const uint32 startBlock = cj.CoordsToBlockIdx(cj.XStart(), cj.YStart());
    uint8 *outBuf = cj.OutBuf() + startBlock * kBlockSz;

    const uint32 endY = std::min(cj.YEnd(), cj.Height() - 4);
    uint32 startX = cj.XStart();
    for(uint32 j = cj.YStart(); j <= endY; j += 4) {
      const uint32 endX = j == cj.YEnd()? cj.XEnd() : cj.Width();
      for(uint32 i = startX; i < endX; i += 4) {

        uint32 pixels[16];
        const uint32 *inPixels = reinterpret_cast<const uint32 *>(cj.InBuf());
        memcpy(pixels, inPixels + j*cj.Width() + i, 4 * sizeof(uint32));
        memcpy(pixels + 4, inPixels + (j+1)*cj.Width() + i, 4 * sizeof(uint32));
        memcpy(pixels + 8, inPixels + (j+2)*cj.Width() + i, 4 * sizeof(uint32));
        memcpy(pixels + 12, inPixels + (j+3)*cj.Width() + i, 4 * sizeof(uint32));

        pack_etc1_block(outBuf, pixels, params);
        outBuf += kBlockSz;
      }
      startX = 0;
    }
  }
}  // namespace PVRTCC
