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

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

#include "Pixel.h"
#include "PVRTCImage.h"
#include "Block.h"

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

  template <typename T>
  static T Clamp(const T &v, const T &low, const T &high) {
    return ::std::min(::std::max(low, v), high);
  }

  static const Pixel &Lookup(const Image &img,
                             int32 x, int32 y,
                             uint32 width, uint32 height,
                             const EWrapMode wrapMode) {
    int32 w = static_cast<int32>(width);
    int32 h = static_cast<int32>(height);

    assert(w >= 0);
    assert(h >= 0);

    while(x >= w) {
      x = (wrapMode == eWrapMode_Wrap)? x - w : w - 1;
    }

    while(x < 0) {
      x = (wrapMode == eWrapMode_Wrap)? x + w : 0;
    }

    while(y >= h) {
      y = (wrapMode == eWrapMode_Wrap)? y - h : h - 1;
    }

    while(y < 0) {
      y = (wrapMode == eWrapMode_Wrap)? y + h : 0;
    }

    return img(x, y);
  }

  void Compress(const CompressionJob &dcj,
                bool bTwoBitMode,
                const EWrapMode wrapMode) {
    Image img(dcj.height, dcj.width);
    uint32 nPixels = dcj.height * dcj.width;
    for(uint32 i = 0; i < nPixels; i++) {
      uint32 x = i % dcj.width;
      uint32 y = i / dcj.width;

      const uint32 *pixels = reinterpret_cast<const uint32 *>(dcj.inBuf);
      img(x, y).Unpack(pixels[i]);      
    }

    Image original = img;
    img.DebugOutput("Original");

    // Downscale it using anisotropic diffusion based scheme in order to preserve
    // image features, then reupscale and compute deltas. Use deltas to generate
    // initial A & B images followed by modulation data.
    img.ContentAwareDownscale(1, 1, eWrapMode_Wrap, true);
    img.ContentAwareDownscale(1, 1, eWrapMode_Wrap, false);

    Image downscaled = img;

    // Upscale it again
    img.BilinearUpscale(2, 2, eWrapMode_Wrap);

    img.DebugOutput("Reconstruction");

    // Compute difference...
    ::std::vector<int16> difference;
    difference.resize(dcj.height * dcj.width * 4);
    for(uint32 j = 0; j < dcj.height; j++) {
      for(uint32 i = 0; i < dcj.width; i++) {
        for(uint32 c = 0; c < 4; c++) {
          int16 o = original(i, j).Component(c);
          int16 n = img(i, j).Component(c);
          difference[j*dcj.width*4 + i*4 + c] = o - n;
        }
      }
    }

    const uint32 blocksW = dcj.width / 4;
    const uint32 blocksH = dcj.height / 4;

    // Go over the 7x7 texel blocks and extract bounding box diagonals for each
    // block. We should be able to choose which diagonal we want...
    const int32 kKernelSz = 7;

    Image imgA = downscaled;
    Image imgB = downscaled;
    for(uint32 j = 0; j < blocksH; j++) {
      for(uint32 i = 0; i < blocksW; i++) {
        int32 startX = i*4 + 2 - (kKernelSz / 2);
        int32 startY = j*4 + 2 - (kKernelSz / 2);
        for(int32 y = startY; y < startY + kKernelSz; y++) {
          for(int32 x = startX; x < startX + kKernelSz; x++) {
            const Pixel &po = Lookup(original, x, y, dcj.width, dcj.height, wrapMode);
            Pixel &pa = imgA(i, j);
            Pixel &pb = imgB(i, j);
            for(uint32 c = 0; c < 4; c++) {
              pa.Component(c) = ::std::max(po.Component(c), pa.Component(c));
              pb.Component(c) = ::std::min(po.Component(c), pb.Component(c));
            }
          }
        }
      }
    }

    imgA.DebugOutput("ImageA");
    imgB.DebugOutput("ImageB");

    // Determine modulation values...
    Image upA = imgA;
    Image upB = imgB;

    upA.BilinearUpscale(2, 2, wrapMode);
    upB.BilinearUpscale(2, 2, wrapMode);

    assert(upA.GetHeight() == dcj.height && upA.GetWidth() == dcj.width);
    assert(upB.GetHeight() == dcj.height && upB.GetWidth() == dcj.width);

    upA.DebugOutput("UpscaledA");
    upB.DebugOutput("UpscaledB");

    // Choose the most appropriate modulation values for the two images...
    ::std::vector<uint8> modValues;
    modValues.resize(dcj.width * dcj.height);
    for(uint32 j = 0; j < dcj.height; j++) {
      for(uint32 i = 0; i < dcj.width; i++) {
        uint8 &mv = modValues[j * dcj.width + i];

        const Pixel pa = upA(i, j);
        const Pixel pb = upB(i, j);
        const Pixel po = original(i, j);
        
        // !FIXME! there are two modulation modes... we're only using one.
        uint8 modSteps[4] = { 8, 5, 3, 0 };
        uint8 bestMod = 0;
        uint32 bestError = 0xFFFFFFFF;
        for(uint32 s = 0; s < 4; s++) {
          uint32 error = 0;
          for(uint32 c = 0; c < 4; c++) {
            uint16 va = static_cast<uint16>(pa.Component(c));
            uint16 vb = static_cast<uint16>(pb.Component(c));
            uint16 vo = static_cast<uint16>(po.Component(c));

            uint16 lerpVal = modSteps[s];
            uint16 res = (va * (8 - lerpVal) + vb * lerpVal) / 8;
            uint16 e = (res > vo)? res - vo : vo - res;
            error += e * e;
          }

          if(error < bestError) {
            bestError = error;
            bestMod = s;
          }
        }

        mv = bestMod;
      }
    }

    // Pack everything into a PVRTC blocks.
    assert(imgA.GetHeight() == blocksH);
    assert(imgA.GetWidth() == blocksW);

    std::vector<uint64> blocks;
    blocks.reserve(blocksW * blocksH);
    for(uint32 j = 0; j < blocksH; j++) {
      for(uint32 i = 0; i < blocksW; i++) {
        Block b;
        b.SetColorA(imgA(i, j), true);
        b.SetColorB(imgB(i, j), true);
        for(uint32 t = 0; t < 16; t++) {
          uint32 x = i*4 + (t%4);
          uint32 y = j*4 + (t/4);
          b.SetLerpValue(t, modValues[y*dcj.width + x]);
        }
        blocks.push_back(b.Pack());
      }
    }

    // Spit out the blocks...
    for(uint32 j = 0; j < blocksH; j++) {
      for(uint32 i = 0; i < blocksW; i++) {

        // The blocks are initially arranged in morton order. Let's
        // linearize them...
        uint32 idx = Interleave(j, i);

        uint64 *outPtr = reinterpret_cast<uint64 *>(dcj.outBuf);
        outPtr[idx] = blocks[j*blocksW + i];
      }
    }
  }

}  // namespace PVRTCC
