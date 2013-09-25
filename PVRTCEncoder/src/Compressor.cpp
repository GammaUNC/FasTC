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
#include "Image.h"
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

  template <typename T>
  static T Lookup(const ::std::vector<T> &vals,
                  uint32 x, uint32 y,
                  uint32 width, uint32 height,
                  const EWrapMode wrapMode) {
    while(x >= width) {
      if(wrapMode == eWrapMode_Wrap) {
        x -= width;
      } else {
        x = width - 1;
      }
    }

    while(x < 0) {
      if(wrapMode == eWrapMode_Wrap) {
        x += width;
      } else {
        x = 0;
      }
    }

    while(y >= height) {
      if(wrapMode == eWrapMode_Wrap) {
        y -= height;
      } else {
        y = height - 1;
      }
    }

    while(y < 0) {
      if(wrapMode == eWrapMode_Wrap) {
        y += height;
      } else {
        y = 0;
      }
    }

    return vals[y * width + x];
  }

  void Compress(const CompressionJob &dcj,
                bool bTwoBitMode,
                const EWrapMode wrapMode) {
    Image img(dcj.height, dcj.width);
    uint32 nPixels = dcj.height * dcj.width;
    for(uint32 i = 0; i < nPixels; i++) {
      // Assume block stream order (whyyyy)
      uint32 blockIdx = i / 16;
      uint32 blockWidth = dcj.width / 4;
      uint32 blockX = blockIdx % blockWidth;
      uint32 blockY = blockIdx / blockWidth;

      uint32 x = blockX * 4 + (i % 4);
      uint32 y = blockY * 4 + (i % 16) / 4;

      const uint32 *pixels = reinterpret_cast<const uint32 *>(dcj.inBuf);
      img(x, y).UnpackRGBA(pixels[i]);      
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

    // Go over the 7x7 texel blocks and extract bounding box diagonals for each
    // block. We should be able to choose which diagonal we want...
    const uint32 kKernelSz = 7;
    ::std::vector<int16> maxDiff;
    ::std::vector<int16> minDiff;

    const uint32 kNumBlockChannels = dcj.height * dcj.width / 4;
    maxDiff.resize(kNumBlockChannels);
    minDiff.resize(kNumBlockChannels);

    for(uint32 j = 2; j < dcj.height; j += 4) {
      for(uint32 i = 2; i < dcj.width; i += 4) {
        const uint32 startX = i - (kKernelSz / 2);
        const uint32 startY = j - (kKernelSz / 2);
        for(uint32 c = 0; c < 4; c++) {
          int32 pos = 0;
          int32 neg = 0;
          for(uint32 y = startY; y < startY + kKernelSz; y++) {
            for(uint32 x = startX; x < startX + kKernelSz; x++) {
              int16 val = Lookup(difference, x*4 + c, y,
                                 dcj.width*4, dcj.height, wrapMode);
              if(val > 0) {
                pos += val;
              } else {
                neg += val;
              }
            }
          }

          uint32 blockIdx = ((j-2)/4) * dcj.width + (i-2) + c;
          assert(blockIdx < kNumBlockChannels);
          if(pos > -neg) {
            maxDiff[blockIdx] = pos;
            minDiff[blockIdx] = 0;
          } else {
            maxDiff[blockIdx] = 0;
            minDiff[blockIdx] = neg;       
          }
        }
      }
    }

    // Add maxDiff to image to get high signal, and lowdiff to image to
    // get low signal...
    Image imgA = downscaled;
    Image imgB = downscaled;

    for(uint32 j = 0; j < dcj.height / 4; j++) {
      for(uint32 i = 0; i < dcj.width / 4; i++) {
        for(uint32 c = 0; c < 4; c++) {
          const uint32 cIdx = j*dcj.width/4 + i*4 + c;
          uint8 &a = imgA(i, j).Component(c);
          a = static_cast<uint8>(Clamp<int16>(a + maxDiff[cIdx], 0, 255));

          uint8 &b = imgB(i, j).Component(c);
          b = static_cast<uint8>(Clamp<int16>(b + minDiff[cIdx], 0, 255));
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
        uint8 modSteps[4] = { 0, 3, 5, 8 };
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
            bestMod = modSteps[s];
          }
        }

        mv = bestMod;
      }
    }

    // Pack everything into a PVRTC blocks.
    const uint32 blocksW = dcj.width / 4;
    const uint32 blocksH = dcj.height / 4;
    std::vector<uint64> blocks;
    for(uint32 j = 0; j < blocksH; j++) {
      for(uint32 i = 0; i < blocksW; i++) {
        Block b;
        b.SetColorA(imgA(i, j));
        b.SetColorB(imgB(i, j));
        for(uint32 t = 0; t < 16; t++) {
          uint32 x = i + (t%4);
          uint32 y = j + (t/4);
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

        uint32 offset = idx * PVRTCC::kBlockSize;
        uint64 *outPtr = reinterpret_cast<uint64 *>(dcj.outBuf + offset);
        *outPtr = blocks[j * blocksW + i];
      }
    }
  }

}  // namespace PVRTCC
