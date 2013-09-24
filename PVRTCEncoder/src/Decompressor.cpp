/* FasTC
 * Copyright (c) 2013 University of North Carolina at Chapel Hill.
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

#include "PVRTCCompressor.h"

#include <cassert>
#include <vector>

#include "Pixel.h"
#include "Block.h"
#include "Image.h"

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

    Image debugModulation(h, w);
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

        Pixel result;
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

        for(uint32 c = 0; c < 4; c++) {
          uint16 va = static_cast<uint16>(pa.Component(c));
          uint16 vb = static_cast<uint16>(pb.Component(c));

          uint16 res = (va * (8 - lerpVal) + vb * lerpVal) / 8;
          result.Component(c) = static_cast<uint8>(res);
        }

        if(punchThrough) {
          result.A() = 0;
        }

        uint32 *outPixels = reinterpret_cast<uint32 *>(outBuf);
        outPixels[(j * w) + i] = result.PackRGBA();
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

        Pixel result;
        for(uint32 c = 0; c < 4; c++) {
          uint16 va = static_cast<uint16>(pa.Component(c));
          uint16 vb = static_cast<uint16>(pb.Component(c));

          uint16 res = (va * (8 - lerpVal) + vb * lerpVal) / 8;
          result.Component(c) = static_cast<uint8>(res);
        }

        uint32 *outPixels = reinterpret_cast<uint32 *>(outBuf);
        outPixels[(j * w) + i] = result.PackRGBA();
      }
    }

    if(bDebugImages) {
      Image dbgMod(h, w);
      for(uint32 i = 0; i < h*w; i++) {
        float fb = static_cast<float>(modValues[i]);
        uint8 val = static_cast<uint8>((fb / 8.0f) * 15.0f);

        for(int k = 1; k < 4; k++) {
          dbgMod(i%w, i/w).Component(k) = val;
        }
        dbgMod(i%w, i/w).A() = 0xFF;
      }

      dbgMod.DebugOutput("Modulation");
    }
  }

  void Decompress(const DecompressionJob &dcj,
                  const bool bTwoBitMode,
                  const EWrapMode wrapMode,
                  bool bDebugImages) {
    const uint32 w = dcj.width;
    const uint32 h = dcj.height;

    assert(w > 0);
    assert(h > 0);
    assert(bTwoBitMode || w % 4 == 0);
    assert(!bTwoBitMode || w % 8 == 0);
    assert(h % 4 == 0);

    // First, extract all of the block information...
    std::vector<Block> blocks;
    blocks.reserve(w * h);

    const uint32 blocksW = bTwoBitMode? (w / 8) : (w / 4);
    const uint32 blocksH = h / 4;

    for(uint32 j = 0; j < blocksH; j++) {
      for(uint32 i = 0; i < blocksW; i++) {

        // The blocks are initially arranged in morton order. Let's
        // linearize them...
        uint32 idx = Interleave(j, i);

        uint32 offset = idx * kBlockSize;
        blocks.push_back( Block(dcj.inBuf + offset) );
      }
    }

    assert(blocks.size() > 0);

    // Extract the endpoints into A and B images
    Image imgA(blocksH, blocksW);
    Image imgB(blocksH, blocksW);

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
      Decompress2BPP(imgA, imgB, blocks, dcj.outBuf, bDebugImages);
    } else {
      Decompress4BPP(imgA, imgB, blocks, dcj.outBuf, bDebugImages);
    }
  }

}  // namespace PVRTCC
