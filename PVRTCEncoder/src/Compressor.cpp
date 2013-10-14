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
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

#include "Pixel.h"
#include "Color.h"
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

  struct Label {
    uint8 distance;
    uint8 nLabels;
    static const int kMaxNumIdxs = 16;
    uint8 times[kMaxNumIdxs];
    uint32 idxs[kMaxNumIdxs];
    float blockIntensity;

    void AddIdx(uint32 idx) {
      for(uint32 i = 0; i < nLabels; i++) {
        if(idxs[i] == idx) {
          times[i]++;
          return;
        }
      }

      assert(nLabels < kMaxNumIdxs);
      times[nLabels] = 1;
      idxs[nLabels] = idx;
      nLabels++;
    }

    void Combine(const Label &other) {
      for(uint32 i = 0; i < other.nLabels; i++) {
        AddIdx(other.idxs[i]);
      }
    }
  };

  struct CompressionLabel {
    bool bCachedIntensity;
    float intensity;
    Label highLabel;
    Label lowLabel;
  };

  static float LookupIntensity(CompressionLabel *labels, const uint32 *pixels,
                               const uint32 idx) {
    if(labels[idx].bCachedIntensity) {
      return labels[idx].intensity;
    }

    uint32 pixel = pixels[idx];
    const float a = static_cast<float>((pixel >> 24) & 0xFF) / 255.0f;
    const float r = a * static_cast<float>(pixel & 0xFF) / 255.0f;
    const float g = a * static_cast<float>((pixel >> 8) & 0xFF) / 255.0f;
    const float b = a * static_cast<float>((pixel >> 16) & 0xFF) / 255.0f;

    labels[idx].intensity = r * 0.2126f + g * 0.7152f + b * 0.0722f;
    labels[idx].bCachedIntensity = true;
    return labels[idx].intensity;
  }

  enum EExtremaResult {
    eExtremaResult_Neither,
    eExtremaResult_LocalMin,
    eExtremaResult_LocalMax
  };

#ifndef NDEBUG
  template<typename T>
  void AssertPOT(const T &t) {
    assert(t & (t - 1) == 0);
  }
#else
  #define AssertPOT(x) (void)(0)
#endif

  static EExtremaResult ComputeLocalExtrema(
    CompressionLabel *labels, const uint8 *inBuf,
    const uint32 x, const uint32 y, const uint32 width, const uint32 height) {

    AssertPOT(width);
    AssertPOT(height);

    assert(x < width);
    assert(y < height);

    const uint32 *pixels = reinterpret_cast<const uint32 *>(inBuf);
    uint8 i0 = static_cast<uint8>(255.0f * LookupIntensity(labels, pixels, y*width + x) + 0.5f);

    int32 ng = 0;
    int32 nl = 0;

    const int32 kKernelSz = 3;
    const int32 kHalfKernelSz = kKernelSz >> 1;
    for(int32 j = -kHalfKernelSz; j <= kHalfKernelSz; j++)
    for(int32 i = -kHalfKernelSz; i <= kHalfKernelSz; i++) {

      if(i == 0 && j == 0) continue;

      int32 xx = (i + static_cast<int32>(x + width)) % width;
      int32 yy = (j + static_cast<int32>(y + height)) % height;

      assert(xx >= 0 && xx < static_cast<int32>(width));
      assert(yy >= 0 && yy < static_cast<int32>(height));

      uint32 idx = static_cast<uint32>(xx) + width * static_cast<uint32>(yy);
      uint8 ix = static_cast<uint8>(255.0f * LookupIntensity(labels, pixels, idx) + 0.5f);
      if(ix >= i0) {
        ng++;
      }

      if(ix <= i0) {
        nl++;
      }
    }

    EExtremaResult result = eExtremaResult_Neither;
    if(ng == nl) {
      return result;
    }

    CompressionLabel &l = labels[y*width + x];
    const int32 kThreshold = kKernelSz * kKernelSz - 1;
    if(ng >= kThreshold) {
      l.lowLabel.distance = 1;
      l.lowLabel.AddIdx(y*width+x);
      result = eExtremaResult_LocalMin;
    } else if(nl >= kThreshold) {
      l.highLabel.distance = 1;
      l.highLabel.AddIdx(y*width+x);
      result = eExtremaResult_LocalMax;
    }

    return result;
  }

  static void DilateLabelForward(Label &l, const Label &up, const Label &left) {

    if(l.distance == 1) {
      return;
    }

    if(l.nLabels == 0) {
      l.nLabels = 0;
    }

    // Are we in no position to dilate?
    if(up.distance == 0 && left.distance == 0) {
      return;
    }

    // Have we visited up yet?
    if(up.distance == 0) {
      if(left.distance < 4) {
        l.distance = left.distance + 1;
        l.AddIdx(left.idxs[0]);
      }
      return;
    }

    // Have we visited left yet?
    if(left.distance == 0) {
      if(up.distance < 4) {
        l.distance = up.distance + 1;
        l.AddIdx(up.idxs[0]);
      }
      return;
    }

    // Otherwise, if they're the same, then we're at a corner...
    if(left.distance == up.distance) {
      if(left.idxs[0] == up.idxs[0]) {
        l.distance = left.distance;
        l.AddIdx(left.idxs[0]);
      } else {
        if(up.distance < 4) {
          l.distance = up.distance + 1;
          l.AddIdx(up.idxs[0]);
        }
      }
      return;
    }

    // Otherwise, we're at a disjoint part, so take the minimum and add
    // one to the distance and assume their index...
    if(left.distance < up.distance) {
      l.distance = left.distance + 1;
      l.AddIdx(left.idxs[0]);
    } else { // up.distance < left.distance
      l.distance = up.distance + 1;
      l.AddIdx(up.idxs[0]);
    }
  }

  static void LabelImageForward(CompressionLabel *labels,
                                const uint8 *inBuf,
                                const uint32 w, const uint32 h) {

    AssertPOT(w);
    AssertPOT(h);

    for(uint32 j = 0; j < h+3; j++) {
      for(uint32 i = 0; i < w; i++) {
        EExtremaResult result = ComputeLocalExtrema(labels, inBuf, i, j % h, w, h);
        bool dilateMax = result != eExtremaResult_LocalMax;
        bool dilateMin = result != eExtremaResult_LocalMin;

        if(dilateMax || dilateMin) {
          // Look up and to the left to determine the distance...
          uint32 upIdx = ((j+h-1) % h) * w + i;
          uint32 leftIdx = (j % h) * w + ((i+w-1) % w);

          CompressionLabel &l = labels[(j % h)*w + i];
          CompressionLabel &up = labels[upIdx];
          CompressionLabel &left = labels[leftIdx];

          if(dilateMax) {
            DilateLabelForward(l.highLabel, up.highLabel, left.highLabel);
          }

          if(dilateMin) {
            DilateLabelForward(l.lowLabel, up.lowLabel, left.lowLabel);
          }
        }
      }
    }
  }

  static void DilateLabelBackward(Label &l,
                                  CompressionLabel *neighbors[5],
                                  bool bHighLabel) {
    if(l.distance == 1)
      return;

    const Label *nbs[5] = {
      bHighLabel? &(neighbors[0]->highLabel) : &(neighbors[0]->lowLabel),
      bHighLabel? &(neighbors[1]->highLabel) : &(neighbors[1]->lowLabel),
      bHighLabel? &(neighbors[2]->highLabel) : &(neighbors[2]->lowLabel),
      bHighLabel? &(neighbors[3]->highLabel) : &(neighbors[3]->lowLabel),
      bHighLabel? &(neighbors[4]->highLabel) : &(neighbors[4]->lowLabel)
    };

    // Figure out which labels are closest...
    uint8 minDist = 5;
    for(uint32 i = 0; i < 5; i++) {
      if(nbs[i]->distance > 0)
        minDist = ::std::min(nbs[i]->distance, minDist);
    }

    assert(minDist > 0);

    uint8 newDist = minDist + 1;
    if((l.distance != 0 && l.distance < newDist) || newDist > 4) {
      return;
    }

    if(l.distance != newDist) {
      l.nLabels = 0;
    }

    for(uint32 i = 0; i < 5; i++) {
      if(nbs[i]->distance == minDist) {
        l.Combine(*nbs[i]);
      }
    }
    l.distance = newDist;
  }

  static void LabelImageBackward(CompressionLabel *labels,
                                 const uint32 w, const uint32 h) {

    AssertPOT(w);
    AssertPOT(h);

    CompressionLabel *neighbors[5] = { 0 };
    for(int32 j = static_cast<int32>(h)+2; j >= 0; j--) {
      for(int32 i = static_cast<int32>(w)-1; i >= 0; i--) {

        CompressionLabel &l = labels[(j % h) * w + i];

        // Add top right corner
        neighbors[0] = &(labels[((j + h - 1) % h) * w + ((i + 1) % w)]);

        // Add right label
        neighbors[1] = &(labels[(j % h) * w + ((i + 1) % w)]);

        // Add bottom right label
        neighbors[2] = &(labels[((j + 1) % h) * w + ((i + 1) % w)]);

        // Add bottom label
        neighbors[3] = &(labels[((j + 1) % h) * w + i]);

        // Add bottom left label
        neighbors[4] = &(labels[((j + 1) % h) * w + ((i + w - 1) % w)]);

        DilateLabelBackward(l.highLabel, neighbors, true);
        DilateLabelBackward(l.lowLabel, neighbors, false);
      }
    }
  }

  static FasTC::Color CollectLabel(const uint32 *pixels, const Label &label) {
    FasTC::Color ret;
    uint32 nPs = 0;
    for(uint32 p = 0; p < label.nLabels; p++) {
      FasTC::Color c; c.Unpack(pixels[label.idxs[p]]);
      ret += c * label.times[p];
      nPs += label.times[p];
    }
    ret /= nPs;
    return ret;
  }

  static void GenerateLowHighImages(CompressionLabel *labels,
                                    const uint8 *inBuf, uint8 *outBuf,
                                    const uint32 w, const uint32 h) {
    assert((w % 4) == 0);
    assert((h % 4) == 0);
    AssertPOT(w);
    AssertPOT(h);

    uint32 blocksW = w / 4;
    uint32 blocksH = h / 4;

    FasTC::Color blockColors[2][16];
    const uint32 *pixels = reinterpret_cast<const uint32 *>(inBuf);

    for(uint32 j = 0; j < blocksH; j++) {
      for(uint32 i = 0; i < blocksW; i++) {

        float minIntensity = 1.1, maxIntensity = -0.1;
        uint32 minIntensityIdx = 0, maxIntensityIdx = 0;
        for(uint32 y = j*4; y <= (j+1)*4; y++)
        for(uint32 x = i*4; x <= (i+1)*4; x++) {

          uint32 idx = (y%h)*w + (x%w);
          float intensity = labels[idx].intensity;
          if(intensity < minIntensity) {
            minIntensity = intensity;
            minIntensityIdx = idx;
          }

          if(intensity > maxIntensity) {
            maxIntensity = intensity;
            maxIntensityIdx = idx;
          }

          if(x == ((i + 1) * 4) || y == ((j + 1) * 4))
            continue;

          uint32 localIdx = (y-(j*4))*4 + x-(i*4);
          assert(localIdx < 16);

          if(labels[idx].highLabel.distance > 0) {
            blockColors[0][localIdx] = CollectLabel(pixels, labels[idx].highLabel);
          } else {
            // Mark the color as unused
            blockColors[0][localIdx].A() = -1.0f;
          }

          if(labels[idx].lowLabel.distance > 0) {
            blockColors[1][localIdx] = CollectLabel(pixels, labels[idx].lowLabel);
          } else {
            // Mark the color as unused
            blockColors[1][localIdx].A() = -1.0f;
          }
        }

        // Average all of the values together now...
        FasTC::Color high, low;
        for(uint32 y = 0; y < 4; y++)
        for(uint32 x = 0; x < 4; x++) {
          uint32 idx = y * 4 + x;
          FasTC::Color c = blockColors[0][idx];
          if(c.A() < 0.0f) {
            c.Unpack(pixels[maxIntensityIdx]);
          }
          high += c * (1.0f / 16.0f);

          c = blockColors[1][idx];
          if(c.A() < 0.0f) {
            c.Unpack(pixels[minIntensityIdx]);
          }
          low += c * (1.0f / 16.0f);
        }

        // Store them as our endpoints for this block...
        Block b;
        FasTC::Pixel p;
        p.Unpack(high.Pack());
        b.SetColorA(p);

        p.Unpack(low.Pack());
        b.SetColorB(p);

        uint64 *outBlocks = reinterpret_cast<uint64 *>(outBuf);
        outBlocks[j * blocksW + i] = b.Pack();
      }
    }
  }

  static FasTC::Pixel BilerpPixels(uint32 x, uint32 y,
                                   const FasTC::Pixel &p, FasTC::Pixel &fp,
                                   const FasTC::Pixel &topLeft,
                                   const FasTC::Pixel &topRight,
                                   const FasTC::Pixel &bottomLeft,
                                   const FasTC::Pixel &bottomRight) {

    const uint32 highXWeight = x;
    const uint32 lowXWeight = 4 - x;
    const uint32 highYWeight = y;
    const uint32 lowYWeight = 4 - y;
 
    const uint32 topLeftWeight = lowXWeight * lowYWeight;
    const uint32 topRightWeight = highXWeight * lowYWeight;
    const uint32 bottomLeftWeight = lowXWeight * highYWeight;
    const uint32 bottomRightWeight = highXWeight * highYWeight;

    // bilerp each channel....
    const FasTC::Pixel tl = topLeft * topLeftWeight;
    const FasTC::Pixel tr = topRight * topRightWeight;
    const FasTC::Pixel bl = bottomLeft * bottomLeftWeight;
    const FasTC::Pixel br = bottomRight * bottomRightWeight;
    const FasTC::Pixel sum = tl + tr + bl + br;

    for(uint32 c = 0; c < 4; c++) {
      fp.Component(c) = sum.Component(c) & 15;
    }

    FasTC::Pixel tmp(p);
    tmp = sum / (16);

    const uint8 fullDepth[4] = { 8, 8, 8, 8 };
    tmp.ChangeBitDepth(fullDepth);

    const uint8 currentDepth[4] = { 4, 5, 5, 5 };
    const uint8 fractionDepth[4] = { 4, 4, 4, 4 };

    for(uint32 c = 0; c < 4; c++) {
      const uint32 denominator = (1 << currentDepth[c]);
      const uint32 numerator = denominator + 1;

      const uint32 shift = fractionDepth[c] - (fullDepth[c] - currentDepth[c]);
      const uint32 fractionBits = tmp.Component(c) >> shift;

      uint32 component = p.Component(c);
      component += ((fractionBits * numerator) / denominator);

      tmp.Component(c) = component;
    }

    return tmp;
  }

  static void ChangePixelTo4555(FasTC::Pixel &p) {
    uint8 refDepths[4];
    p.GetBitDepth(refDepths);

    const uint8 scaleDepths[4] = { 4, 5, 5, 5 };
    p.ChangeBitDepth(scaleDepths);

    if(refDepths[0] > 0) {
      p.A() = p.A() & 0xFE;
    }
  }

  static void GenerateModulationValues(uint8 *outBuf, const uint8 *inBuf, uint32 w, uint32 h) {

    AssertPOT(w);
    AssertPOT(h);

    // Start from the beginning block and generate the lerp values for the intermediate values
    uint64 *outBlocks = reinterpret_cast<uint64 *>(outBuf);
    const uint32 blocksW = w >> 2;
    const uint32 blocksH = h >> 2;

    const uint32 *pixels = reinterpret_cast<const uint32 *>(inBuf);

    // Make sure the bit depth matches the original...
    FasTC::Pixel p;
    uint8 bitDepth[4] = { 4, 5, 5, 5 };
    p.ChangeBitDepth(bitDepth);

    // Save fractional bits
    FasTC::Pixel fp;
    uint8 fpDepths[4] = { 4, 4, 4, 4 };
    fp.ChangeBitDepth(fpDepths);

    for(uint32 j = 0; j < blocksH; j++) {
      for(uint32 i = 0; i < blocksW; i++) {

        const int32 lowXIdx = i;
        const int32 highXIdx = (i + 1) % w;
        const int32 lowYIdx = j;
        const int32 highYIdx = (j + 1) % h;

        const uint32 topLeftBlockIdx = lowYIdx * blocksW + lowXIdx;
        const uint32 topRightBlockIdx = lowYIdx * blocksW + highXIdx;
        const uint32 bottomLeftBlockIdx = highYIdx * blocksW + lowXIdx;
        const uint32 bottomRightBlockIdx = highYIdx * blocksW + highXIdx;

        Block topLeftBlock(reinterpret_cast<uint8 *>(outBlocks + topLeftBlockIdx));
        Block topRightBlock(reinterpret_cast<uint8 *>(outBlocks + topRightBlockIdx));
        Block bottomLeftBlock(reinterpret_cast<uint8 *>(outBlocks + bottomLeftBlockIdx));
        Block bottomRightBlock(reinterpret_cast<uint8 *>(outBlocks + bottomRightBlockIdx));

        FasTC::Pixel topLeftA (topLeftBlock.GetColorA());
        FasTC::Pixel topLeftB (topLeftBlock.GetColorB());

        FasTC::Pixel topRightA (topRightBlock.GetColorA());
        FasTC::Pixel topRightB (topRightBlock.GetColorB());

        FasTC::Pixel bottomLeftA (bottomLeftBlock.GetColorA());
        FasTC::Pixel bottomLeftB (bottomLeftBlock.GetColorB());

        FasTC::Pixel bottomRightA (topLeftBlock.GetColorA());
        FasTC::Pixel bottomRightB (topLeftBlock.GetColorB());

        ChangePixelTo4555(topLeftA);
        ChangePixelTo4555(topLeftB);

        ChangePixelTo4555(topRightA);
        ChangePixelTo4555(topRightB);

        ChangePixelTo4555(bottomLeftA);
        ChangePixelTo4555(bottomLeftB);

        ChangePixelTo4555(bottomRightA);
        ChangePixelTo4555(bottomRightB);

        for(uint32 x = 0; x < 4; x++) {
          for(uint32 y = 0; y < 4; y++) {
            uint32 pixelX = (i + 2 + x) % w;
            uint32 pixelY = (j + 2 + y) % h;
            FasTC::Pixel colorA = BilerpPixels(x, y, p, fp, topLeftA, topRightA, bottomLeftA, bottomRightA);
            FasTC::Pixel colorB = BilerpPixels(x, y, p, fp, topLeftB, topRightB, bottomLeftB, bottomRightB);
            FasTC::Pixel original(pixels[pixelY * w + pixelX]);

            // !FIXME! there are two modulation modes... we're only using one.
            uint8 modSteps[4] = { 8, 5, 3, 0 };
            uint8 bestMod = 0;
            uint32 bestError = 0xFFFFFFFF;
            for(uint32 s = 0; s < 4; s++) {
              uint16 lerpVal = modSteps[s];
              FasTC::Pixel result = (colorA * (8 - lerpVal) + colorB * lerpVal) / 8;

              FasTC::Vector4<int32> errorVec;
              for(uint32 c = 0; c < 4; c++) {
                int32 r = result.Component(c);
                int32 o = original.Component(c);
                errorVec[c] = r - o;
              }
              uint32 error = static_cast<uint64>(errorVec.LengthSq());

              if(error < bestError) {
                bestError = error;
                bestMod = s;
              }
            }

            Block *pixelBlock = &topLeftBlock;
            uint32 pixelBlockIdx = topLeftBlockIdx;
            if(x > 1) {
              if(y > 1) {
                pixelBlock = &bottomRightBlock;
                pixelBlockIdx = bottomRightBlockIdx;
              } else {
                pixelBlock = &topRightBlock;
                pixelBlockIdx = topRightBlockIdx;
              }
            } else if(y > 1) {
              pixelBlock = &bottomLeftBlock;
              pixelBlockIdx = bottomLeftBlockIdx;
            }

            pixelBlock->SetLerpValue((pixelY % 4) * 4 + (pixelX % 4), bestMod);
            outBlocks[pixelBlockIdx] = outBlocks[pixelBlockIdx] | pixelBlock->Pack();
          }
        }
      }
    }
  }

  void Compress(const CompressionJob &cj, bool bTwoBit, EWrapMode wrapMode) {
    const uint32 width = cj.width;
    const uint32 height = cj.height;

    // Make sure that width and height are a power of two.
    assert(width & (width - 1) == 0);
    assert(height & (height - 1) == 0);

    memset(cj.outBuf, 0, (width * height / 16) * kBlockSize);

    CompressionLabel *labels =
      (CompressionLabel *)calloc(width * height, sizeof(CompressionLabel));

    // First traverse forward...
    LabelImageForward(labels, cj.inBuf, width, height);

#ifndef NDEBUG
    Image highForwardLabels(width, height);
    Image lowForwardLabels(width, height);

    const FasTC::Color kLabelPalette[4] = {
      FasTC::Color(0.0, 0.0, 1.0, 1.0),
      FasTC::Color(1.0, 0.0, 1.0, 1.0),
      FasTC::Color(1.0, 0.0, 0.0, 1.0),
      FasTC::Color(1.0, 1.0, 0.0, 1.0)
    };

    for(uint32 j = 0; j < height; j++) {
      for(uint32 i = 0; i < width; i++) {
        const CompressionLabel &l = labels[j*width + i];

        const Label &hl = l.highLabel;
        if(hl.distance > 0) {
          highForwardLabels(i, j).Unpack(kLabelPalette[hl.distance-1].Pack());
        }

        const Label &ll = l.lowLabel;
        if(ll.distance > 0) {
          lowForwardLabels(i, j).Unpack(kLabelPalette[ll.distance-1].Pack());
        }
      }
    }

    highForwardLabels.DebugOutput("HighForwardLabels");
    lowForwardLabels.DebugOutput("LowForwardLabels");

    Image highForwardImg(width, height);
    Image lowForwardImg(width, height);
    const uint32 *pixels = reinterpret_cast<const uint32 *>(cj.inBuf);
    for(uint32 j = 0; j < height; j++) {
      for(uint32 i = 0; i < width; i++) {
        const CompressionLabel &l = labels[j*width + i];

        const Label &hl = l.highLabel;
        if(hl.distance > 0) {
          FasTC::Color c;
          uint32 nPs = 0;
          for(uint32 p = 0; p < hl.nLabels; p++) {
            FasTC::Color pc; pc.Unpack(pixels[hl.idxs[p]]);
            c += pc * static_cast<float>(hl.times[p]);
            nPs += hl.times[p];
          }
          c /= nPs;
          highForwardImg(i, j).Unpack(c.Pack());
        }

        const Label &ll = l.lowLabel;
        if(ll.distance > 0) {
          FasTC::Color c;
          uint32 nPs = 0;
          for(uint32 p = 0; p < ll.nLabels; p++) {
            FasTC::Color pc; pc.Unpack(pixels[ll.idxs[p]]);
            c += pc * static_cast<float>(ll.times[p]);
            nPs += ll.times[p];
          }
          c /= nPs;
          lowForwardImg(i, j).Unpack(c.Pack());
        }
      }
    }

    highForwardImg.DebugOutput("HighForwardImg");
    lowForwardImg.DebugOutput("LowForwardImg");

    std::cout << "Output Forward images." << std::endl;
#endif

    // Then traverse backward...
    LabelImageBackward(labels, width, height);

#ifndef NDEBUG
    Image highImg(width, height);
    Image lowImg(width, height);
    for(uint32 j = 0; j < height; j++) {
      for(uint32 i = 0; i < width; i++) {
        const CompressionLabel &l = labels[j*width + i];

        const Label &hl = l.highLabel;
        if(hl.distance > 0) {
          FasTC::Color c;
          for(uint32 p = 0; p < hl.nLabels; p++) {
            FasTC::Color pc; pc.Unpack(pixels[hl.idxs[p]]);
            c += pc;
          }
          c /= hl.nLabels;
          highImg(i, j).Unpack(c.Pack());
        }

        const Label &ll = l.lowLabel;
        if(ll.distance > 0) {
          FasTC::Color c;
          for(uint32 p = 0; p < ll.nLabels; p++) {
            FasTC::Color pc; pc.Unpack(pixels[ll.idxs[p]]);
            c += pc;
          }
          c /= ll.nLabels;
          lowImg(i, j).Unpack(c.Pack());
        }
      }
    }

    highImg.DebugOutput("HighImg");
    lowImg.DebugOutput("LowImg");

    std::cout << "Output images." << std::endl;
#endif

    // Then combine everything...
    GenerateLowHighImages(labels, cj.inBuf, cj.outBuf, width, height);

    // Then compute modulation values
    GenerateModulationValues(cj.outBuf, cj.inBuf, width, height);

    // Cleanup
    free(labels);
  }
}  // namespace PVRTCC
