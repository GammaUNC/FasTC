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

#include "FasTC/PVRTCCompressor.h"

#include <cassert>
#include <vector>

#include "FasTC/Pixel.h"

#include "Block.h"
#include "PVRTCImage.h"

namespace PVRTCC {

  static uint32 Interleave(uint16 inx, uint16 iny) {
    // Taken from:
    // http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN

    static const uint32 B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF};
    static const uint32 S[] = {1, 2, 4, 8};

    uint32 x = static_cast<uint32>(inx);
    uint32 y = static_cast<uint32>(iny);

    x = (x | (x << S[3])) & B[3];
    x = (x | (x << S[2])) & B[2];
    x = (x | (x << S[1])) & B[1];
    x = (x | (x << S[0])) & B[0];

    y = (y | (y << S[3])) & B[3];
    y = (y | (y << S[2])) & B[2];
    y = (y | (y << S[1])) & B[1];
    y = (y | (y << S[0])) & B[0];

    return x | (y << 1);
  }

  static void Decompress4BPP(const Image &imgA, const Image &imgB,
                             const std::vector<Block> &blocks,
                             uint8 *const outBuf,
                             bool bDebugImages = false) {
    const uint32 w = imgA.GetWidth();
    const uint32 h = imgA.GetHeight();

    assert(imgA.GetWidth() == imgB.GetWidth());
    assert(imgA.GetHeight() == imgB.GetHeight());

    Image debugModulation(w, h);
    const uint8 debugModulationBitDepth[4] = { 8, 4, 4, 4 };
    debugModulation.ChangeBitDepth(debugModulationBitDepth);

    for(uint32 j = 0; j < h; j++) {
      for(uint32 i = 0; i < w; i++) {
        const uint32 blockWidth = 4;
        const uint32 blockHeight = 4;

        const uint32 blockIdx =
          (j/blockHeight) * (w/blockWidth) + (i/blockWidth);
        const Block &b = blocks[blockIdx];

        const uint32 texelIndex =
          (j % blockHeight) * blockWidth + (i % blockWidth);

        const Pixel &pa = imgA(i, j);
        const Pixel &pb = imgB(i, j);

        bool punchThrough = false;
        uint8 lerpVal = 0;
        if(b.GetModeBit()) {
          const uint8 lerpVals[3] = { 8, 4, 0 };
          uint8 modVal = b.GetLerpValue(texelIndex);

          if(modVal >= 2) {
            if(modVal == 2) {
              punchThrough = true;
            }
            modVal -= 1;
          }

          lerpVal = lerpVals[modVal];
        } else {
          const uint8 lerpVals[4] = { 8, 5, 3, 0 };
          lerpVal = lerpVals[b.GetLerpValue(texelIndex)];
        }

        if(bDebugImages) {
          Pixel &modPx = debugModulation(i, j);
          modPx.A() = 0xFF;
          for(uint32 c = 1; c < 4; c++) {
            float fv = (static_cast<float>(lerpVal) / 8.0f) * 15.0f;
            modPx.Component(c) = static_cast<uint8>(fv);
          }

          // Make punch through pixels red.
          if(punchThrough) {
            modPx.G() = modPx.B() = 0;
          }
        }

        Pixel result = (pa * (8 - lerpVal) + pb * lerpVal) / 8;
        if(punchThrough) {
          result.A() = 0;
        }

        uint32 *outPixels = reinterpret_cast<uint32 *>(outBuf);
        outPixels[(j * w) + i] = result.Pack();
      }
    }

    if(bDebugImages) {
      debugModulation.DebugOutput("Modulation");
    }
  }

  static void Decompress2BPP(const Image &imgA, const Image &imgB,
                             const std::vector<Block> &blocks,
                             uint8 *const outBuf,
                             bool bDebugImages) {
    const uint32 w = imgA.GetWidth();
    const uint32 h = imgA.GetHeight();

    assert(w > 0);
    assert(h > 0);
    assert(imgA.GetWidth() == imgB.GetWidth());
    assert(imgA.GetHeight() == imgB.GetHeight());

    std::vector<uint8> modValues;
    modValues.reserve(w * h);

    const uint32 blockWidth = 8;
    const uint32 blockHeight = 4;

    for(uint32 j = 0; j < h; j++) {
      for(uint32 i = 0; i < w; i++) {

        const uint32 blockIdx =
          (j/blockHeight) * (w/blockWidth) + (i/blockWidth);
        const Block &b = blocks[blockIdx];

        const uint32 texelIndex =
          (j % blockHeight) * blockWidth + (i % blockWidth);

        uint8 lerpVal = 0;
        if(b.GetModeBit()) {
          uint32 texelX = texelIndex % blockWidth;
          uint32 texelY = texelIndex / blockWidth;

          const uint8 lerpVals[4] = { 8, 5, 3, 0 };
          if(((texelX ^ texelY) & 0x1) == 0) {
            uint32 lerpIdx = texelY * (blockWidth / 2) + (texelX / 2);
            lerpVal = lerpVals[b.Get2BPPLerpValue(lerpIdx)];
          }
        } else {
          lerpVal = b.Get2BPPLerpValue(texelIndex);
          lerpVal = lerpVal? 0 : 8;
        }
        modValues.push_back(lerpVal);
      }
    }

    assert(modValues.size() == w * h);

    for(uint32 j = 0; j < h; j++) {
      for(uint32 i = 0; i < w; i++) {

        const uint32 blockIdx =
          (j/blockHeight) * (w/blockWidth) + (i/blockWidth);
        const Block &b = blocks[blockIdx];

        uint8 lerpVal = 0;
        #define GET_LERP_VAL(x, y) modValues[(y) * w + (x)]
        if(b.GetModeBit() && ((i ^ j) & 0x1)) {

          switch(b.Get2BPPSubMode()) {
            case Block::e2BPPSubMode_Horizontal:
              lerpVal += GET_LERP_VAL((i + w - 1) % w, j);
              lerpVal += GET_LERP_VAL((i + w + 1) % w, j);
              lerpVal /= 2;
            break;

            case Block::e2BPPSubMode_Vertical:
              lerpVal += GET_LERP_VAL(i, (j + h - 1) % h);
              lerpVal += GET_LERP_VAL(i, (j + h + 1) % h);
              lerpVal /= 2;
            break;

            default:
            case Block::e2BPPSubMode_All:
              lerpVal += GET_LERP_VAL(i, (j + h - 1) % h);
              lerpVal += GET_LERP_VAL(i, (j + h + 1) % h);
              lerpVal += GET_LERP_VAL((i + w - 1) % w, j);
              lerpVal += GET_LERP_VAL((i + w + 1) % w, j);
              lerpVal = (lerpVal + 1) / 4;
            break;
          }
          GET_LERP_VAL(i, j) = lerpVal;
        } else {
          lerpVal = GET_LERP_VAL(i, j);
        }
        #undef GET_LERP_VAL

        const Pixel &pa = imgA(i, j);
        const Pixel &pb = imgB(i, j);

        Pixel result = (pa * (8 - lerpVal) + pb * lerpVal) / 8;
        uint32 *outPixels = reinterpret_cast<uint32 *>(outBuf);
        outPixels[(j * w) + i] = result.Pack();
      }
    }

    if(bDebugImages) {
      Image dbgMod(w, h);
      for(uint32 i = 0; i < h*w; i++) {
        float fb = static_cast<float>(modValues[i]);
        uint8 val = static_cast<uint8>((fb / 8.0f) * 15.0f);

        for(uint32 k = 1; k < 4; k++) {
          dbgMod(i%w, i/w).Component(k) = val;
        }
        dbgMod(i%w, i/w).A() = 0xFF;
      }

      dbgMod.DebugOutput("Modulation");
    }
  }

  void Decompress(const FasTC::DecompressionJob &dcj,
                  const EWrapMode wrapMode,
                  bool bDebugImages) {
    const bool bTwoBitMode = dcj.Format() == FasTC::eCompressionFormat_PVRTC2;
    const uint32 w = dcj.Width();
    const uint32 h = dcj.Height();

    assert(w > 0);
    assert(h > 0);
    assert(bTwoBitMode || w % 4 == 0);
    assert(!bTwoBitMode || w % 8 == 0);
    assert(h % 4 == 0);

    // First, extract all of the block information...
    std::vector<Block> blocks;

    const uint32 blocksW = bTwoBitMode? (w / 8) : (w / 4);
    const uint32 blocksH = h / 4;
    blocks.reserve(blocksW * blocksH);

    for(uint32 j = 0; j < blocksH; j++) {
      for(uint32 i = 0; i < blocksW; i++) {

        // The blocks are initially arranged in morton order. Let's
        // linearize them...
        uint32 idx = Interleave(j, i);

        uint32 offset = idx * kBlockSize;
        blocks.push_back( Block(dcj.InBuf() + offset) );
      }
    }

    assert(blocks.size() > 0);

    // Extract the endpoints into A and B images
    Image imgA(blocksW, blocksH);
    Image imgB(blocksW, blocksH);

    for(uint32 j = 0; j < blocksH; j++) {
      for(uint32 i = 0; i < blocksW; i++) {

        uint32 idx = j * blocksW + i;
        assert(idx < static_cast<uint32>(blocks.size()));

        Block &b = blocks[idx];
        imgA(i, j) = b.GetColorA();
        imgB(i, j) = b.GetColorB();
      }
    }

    // Change the pixel mode so that all of the pixels are at the same
    // bit depth.
    const uint8 scaleDepths[4] = { 4, 5, 5, 5 };
    imgA.ChangeBitDepth(scaleDepths);
    if(bDebugImages)
      imgA.DebugOutput("UnscaledImgA");
    imgB.ChangeBitDepth(scaleDepths);
    if(bDebugImages)
      imgB.DebugOutput("UnscaledImgB");

    // Go through and change the alpha value of any pixel that came from
    // a transparent block. For some reason, alpha is not treated the same
    // as the other channels (to minimize hardware costs?) and the channels
    // do not their MSBs replicated.
    for(uint32 j = 0; j < blocksH; j++) {
      for(uint32 i = 0; i < blocksW; i++) {
        const uint32 blockIdx = j * blocksW + i;
        Block &b = blocks[blockIdx];

        uint8 bitDepths[4];
        b.GetColorA().GetBitDepth(bitDepths);
        if(bitDepths[0] > 0) {
          Pixel &p = imgA(i, j);
          p.A() = p.A() & 0xFE;
        }

        b.GetColorB().GetBitDepth(bitDepths);
        if(bitDepths[0] > 0) {
          Pixel &p = imgB(i, j);
          p.A() = p.A() & 0xFE;
        }
      }
    }

    // Bilinearly upscale the images.
    if(bTwoBitMode) {
      imgA.BilinearUpscale(3, 2, wrapMode);
      imgB.BilinearUpscale(3, 2, wrapMode);
    } else {
      imgA.BilinearUpscale(2, 2, wrapMode);
      imgB.BilinearUpscale(2, 2, wrapMode);
    }

    if(bDebugImages) {
      imgA.DebugOutput("RawScaledImgA");
      imgB.DebugOutput("RawScaledImgB");
    }

    // Change the bitdepth to full resolution
    imgA.ExpandTo8888();
    imgB.ExpandTo8888();

    if(bDebugImages) {
      imgA.DebugOutput("ScaledImgA");
      imgB.DebugOutput("ScaledImgB");
    }

    if(bTwoBitMode) {
      Decompress2BPP(imgA, imgB, blocks, dcj.OutBuf(), bDebugImages);
    } else {
      Decompress4BPP(imgA, imgB, blocks, dcj.OutBuf(), bDebugImages);
    }
  }

}  // namespace PVRTCC
