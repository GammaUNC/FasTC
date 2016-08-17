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

#include "FasTC/BPTCCompressor.h"

#include "CompressionMode.h"
#undef DBL_MAX
#include "FasTC/BitStream.h"
#include "FasTC/TexCompTypes.h"

#include <iostream>
#include <sstream>
#include <cstring>

#include "avpcl.h"

namespace BPTCC {

  void GetBlock(uint32 x, uint32 y, uint32 width, const uint32 *pixels, Tile &t) {
    for(uint32 j = 0; j < 4; j++)
    for(uint32 i = 0; i < 4; i++) {
      uint32 pixel = pixels[(y+j)*width + (x+i)];
      t.data[j][i].X() = pixel & 0xFF;
      t.data[j][i].Y() = (pixel >> 8) & 0xFF;
      t.data[j][i].Z() = (pixel >> 16) & 0xFF;
      t.data[j][i].W() = (pixel >> 24) & 0xFF;
    }
  }

  class BlockLogger {
   public:
    BlockLogger(uint64 blockIdx, std::ostream &os)
      : m_BlockIdx(blockIdx), m_Stream(os) { }

    template<typename T>
    friend std::ostream &operator<<(const BlockLogger &bl, const T &v);

    uint64 m_BlockIdx;
    std::ostream &m_Stream;
  };

  template<typename T>
  std::ostream &operator<<(const BlockLogger &bl, const T &v) {
    std::stringstream ss;
    ss << bl.m_BlockIdx << ": " << v;
    return bl.m_Stream << ss.str();
  }

  template<typename T>
  static void PrintStat(const BlockLogger &lgr, const char *stat, const T &v) {
    std::stringstream ss;
    ss << stat << " -- " << v << std::endl;
    lgr << ss.str();
  }

  // Compress an image using BC7 compression. Use the inBuf parameter to point
  // to an image in 4-byte RGBA format. The width and height parameters specify
  // the size of the image in pixels. The buffer pointed to by outBuf should be
  // large enough to store the compressed image. This implementation has an 4:1
  // compression ratio.
  void CompressNVTT(const FasTC::CompressionJob &cj) {
    const uint32 *inPixels = reinterpret_cast<const uint32 *>(cj.InBuf());
    const uint32 kBlockSz = GetBlockSize(FasTC::eCompressionFormat_BPTC);
    uint8 *outBuf = cj.OutBuf() + cj.CoordsToBlockIdx(cj.XStart(), cj.YStart()) * kBlockSz;

    uint32 startX = cj.XStart();
    const uint32 endY = std::min(cj.YEnd(), cj.Height() - 4);
    for(uint32 j = cj.YStart(); j <= endY; j += 4) {
      const uint32 endX = j == cj.YEnd()? cj.XEnd() : cj.Width();
      for(uint32 i = startX; i < endX; i += 4) {

        Tile block(4, 4);
        GetBlock(i, j, cj.Width(), inPixels, block);
        AVPCL::compress(block, reinterpret_cast<char *>(outBuf), NULL);

        outBuf += kBlockSz;
      }
      startX = 0;
    }
  }

  typedef double (*ModeCompressFunc)(const Tile &, char* out);
  static ModeCompressFunc kModeFuncs[8] = {
    AVPCL::compress_mode0,
    AVPCL::compress_mode1,
    AVPCL::compress_mode2,
    AVPCL::compress_mode3,
    AVPCL::compress_mode4,
    AVPCL::compress_mode5,
    AVPCL::compress_mode6,
    AVPCL::compress_mode7
  };

  double CompressMode(uint32 mode, const Tile &t, char *out, BlockLogger &log) {
    std::stringstream ss;
    ss << "Mode_" << mode << "_error";
    double mse = kModeFuncs[mode](t, out);
    PrintStat(log, ss.str().c_str(), mse);

    FasTC::BitStreamReadOnly strm(reinterpret_cast<uint8 *>(out));
    while(!strm.ReadBit());

    const CompressionMode::Attributes *attrs =
      CompressionMode::GetAttributesForMode(mode);
    const uint32 nSubsets = attrs->numSubsets;

    ss.str("");
    ss << "Mode_" << mode << "_shape";

    uint32 shapeIdx = 0;
    if ( nSubsets > 1 ) {
      shapeIdx = strm.ReadBits(mode == 0? 4 : 6);
      PrintStat(log, ss.str().c_str(), shapeIdx);
    } else {
      PrintStat(log, ss.str().c_str(), -1);
    }

    return mse;
  }

  void CompressNVTTWithStats(const FasTC::CompressionJob &cj, std::ostream *logStream) {
    const uint32 *inPixels = reinterpret_cast<const uint32 *>(cj.InBuf());
    const uint32 kBlockSz = GetBlockSize(FasTC::eCompressionFormat_BPTC);
    uint8 *outBuf = cj.OutBuf() + cj.CoordsToBlockIdx(cj.XStart(), cj.YStart()) * kBlockSz;

    uint32 startX = cj.XStart();
    const uint32 endY = std::min(cj.YEnd(), cj.Height() - 4);
    for(uint32 j = cj.YStart(); j <= endY; j += 4) {
      const uint32 endX = j == cj.YEnd()? cj.XEnd() : cj.Width();
      for(uint32 i = startX; i < endX; i += 4) {

        Tile block(4, 4);
        GetBlock(i, j, cj.Width(), inPixels, block);

        if(logStream) {
          BlockLogger logger(cj.CoordsToBlockIdx(i, j), *logStream);

          char tempblock[16];
          double msebest = 1e30;
          for(uint32 mode = 0; mode < 8; mode++) {
            double mse_mode = CompressMode(mode, block, tempblock, logger);
            if(mse_mode < msebest) {
              msebest = mse_mode;
              memcpy(outBuf, tempblock, AVPCL::BLOCKSIZE);
            }
          }
        } else {
          AVPCL::compress(block, reinterpret_cast<char *>(outBuf), NULL);          
        }

        outBuf += 16;
      }

      startX = 0;
    }
  }

}  // namespace BC7C
