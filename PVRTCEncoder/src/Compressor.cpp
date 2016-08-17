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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

#include "FasTC/Pixel.h"
#include "FasTC/Color.h"

#ifndef NDEBUG
#  include "PVRTCImage.h"
#endif
#include "Block.h"
#include "Indexer.h"

// !FIXME! Figure out why the PSNR of these LUTs is worse than when
// we don't use them -- they should be optimal. This is reflected
// in Github issue #19
// #define USE_CONSTANT_LUTS

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

#ifdef USE_CONSTANT_LUTS
  static const uint8 kConstFiveBitLUT[256][2] = {
    {0, 0}, {0, 0}, {8, 0}, {8, 0}, {8, 0}, {0, 8}, {16, 0}, {8, 8},
    {8, 8}, {24, 0}, {0, 16}, {16, 8}, {32, 0}, {8, 16}, {24, 8}, {0, 24},
    {16, 16}, {32, 8}, {8, 24}, {24, 16}, {40, 8}, {16, 24}, {32, 16}, {8, 32},
    {24, 24}, {40, 16}, {16, 32}, {32, 24}, {48, 16}, {24, 32}, {40, 24}, {16, 40},
    {72, 8}, {32, 32}, {24, 40}, {0, 56}, {40, 32}, {72, 16}, {32, 40}, {48, 32},
    {80, 16}, {40, 40}, {56, 32}, {32, 48}, {48, 40}, {64, 32}, {40, 48}, {56, 40},
    {32, 56}, {48, 48}, {64, 40}, {40, 56}, {56, 48}, {72, 40}, {48, 56}, {64, 48},
    {40, 64}, {56, 56}, {72, 48}, {48, 64}, {64, 56}, {80, 48}, {56, 64}, {72, 56},
    {48, 72}, {104, 40}, {64, 64}, {56, 72}, {32, 88}, {72, 64}, {104, 48}, {64, 72},
    {80, 64}, {112, 48}, {72, 72}, {88, 64}, {64, 80}, {80, 72}, {96, 64}, {72, 80},
    {88, 72}, {64, 88}, {80, 80}, {96, 72}, {72, 88}, {88, 80}, {104, 72}, {80, 88},
    {96, 80}, {72, 96}, {88, 88}, {104, 80}, {80, 96}, {96, 88}, {112, 80}, {88, 96},
    {104, 88}, {80, 104}, {136, 72}, {96, 96}, {88, 104}, {64, 120}, {104, 96}, {136, 80},
    {96, 104}, {112, 96}, {144, 80}, {104, 104}, {120, 96}, {96, 112}, {112, 104}, {128, 96},
    {104, 112}, {120, 104}, {96, 120}, {112, 112}, {128, 104}, {104, 120}, {120, 112}, {136, 104},
    {112, 120}, {128, 112}, {104, 128}, {120, 120}, {136, 112}, {112, 128}, {128, 120}, {144, 112},
    {120, 128}, {136, 120}, {112, 136}, {168, 104}, {128, 128}, {120, 136}, {96, 152}, {136, 128},
    {168, 112}, {128, 136}, {144, 128}, {176, 112}, {136, 136}, {152, 128}, {128, 144}, {144, 136},
    {160, 128}, {136, 144}, {152, 136}, {128, 152}, {144, 144}, {160, 136}, {136, 152}, {152, 144},
    {168, 136}, {144, 152}, {160, 144}, {136, 160}, {152, 152}, {168, 144}, {144, 160}, {160, 152},
    {176, 144}, {152, 160}, {168, 152}, {144, 168}, {200, 136}, {160, 160}, {152, 168}, {128, 184},
    {168, 160}, {200, 144}, {160, 168}, {176, 160}, {208, 144}, {168, 168}, {184, 160}, {160, 176},
    {176, 168}, {192, 160}, {168, 176}, {184, 168}, {160, 184}, {176, 176}, {192, 168}, {168, 184},
    {184, 176}, {200, 168}, {176, 184}, {192, 176}, {168, 192}, {184, 184}, {200, 176}, {176, 192},
    {192, 184}, {208, 176}, {184, 192}, {200, 184}, {176, 200}, {232, 168}, {192, 192}, {184, 200},
    {160, 216}, {200, 192}, {232, 176}, {192, 200}, {208, 192}, {240, 176}, {200, 200}, {216, 192},
    {192, 208}, {208, 200}, {224, 192}, {200, 208}, {216, 200}, {192, 216}, {208, 208}, {224, 200},
    {200, 216}, {216, 208}, {232, 200}, {208, 216}, {224, 208}, {200, 224}, {216, 216}, {232, 208},
    {208, 224}, {224, 216}, {240, 208}, {216, 224}, {232, 216}, {208, 232}, {184, 248}, {224, 224},
    {216, 232}, {192, 248}, {232, 224}, {232, 224}, {224, 232}, {240, 224}, {232, 232}, {232, 232},
    {248, 224}, {224, 240}, {240, 232}, {240, 232}, {232, 240}, {248, 232}, {224, 248}, {240, 240},
    {240, 240}, {232, 248}, {248, 240}, {248, 240}, {240, 248}, {240, 248}, {248, 248}, {248, 248}
  };

  static const uint8 kConstFourBitLUT[256][2] = {
    {0, 0}, {0, 0}, {8, 0}, {8, 0}, {8, 0}, {16, 0}, {16, 0}, {16, 0},
    {24, 0}, {24, 0}, {0, 16}, {0, 16}, {32, 0}, {8, 16}, {8, 16}, {40, 0},
    {16, 16}, {16, 16}, {48, 0}, {24, 16}, {0, 32}, {56, 0}, {32, 16}, {8, 32},
    {64, 0}, {40, 16}, {16, 32}, {72, 0}, {48, 16}, {24, 32}, {0, 48}, {56, 16},
    {32, 32}, {32, 32}, {64, 16}, {40, 32}, {40, 32}, {72, 16}, {48, 32}, {48, 32},
    {80, 16}, {0, 64}, {56, 32}, {32, 48}, {8, 64}, {64, 32}, {40, 48}, {16, 64},
    {72, 32}, {48, 48}, {24, 64}, {80, 32}, {56, 48}, {32, 64}, {88, 32}, {64, 48},
    {40, 64}, {96, 32}, {72, 48}, {48, 64}, {24, 80}, {80, 48}, {56, 64}, {32, 80},
    {88, 48}, {144, 16}, {64, 64}, {96, 48}, {152, 16}, {72, 64}, {104, 48}, {0, 112},
    {80, 64}, {112, 48}, {32, 96}, {88, 64}, {64, 80}, {40, 96}, {96, 64}, {72, 80},
    {48, 96}, {104, 64}, {80, 80}, {56, 96}, {112, 64}, {88, 80}, {64, 96}, {120, 64},
    {96, 80}, {72, 96}, {128, 64}, {104, 80}, {80, 96}, {56, 112}, {112, 80}, {88, 96},
    {64, 112}, {120, 80}, {176, 48}, {96, 96}, {128, 80}, {24, 144}, {104, 96}, {136, 80},
    {32, 144}, {112, 96}, {144, 80}, {64, 128}, {120, 96}, {96, 112}, {72, 128}, {128, 96},
    {104, 112}, {80, 128}, {136, 96}, {112, 112}, {88, 128}, {144, 96}, {120, 112}, {96, 128},
    {152, 96}, {128, 112}, {104, 128}, {160, 96}, {136, 112}, {112, 128}, {88, 144}, {144, 112},
    {120, 128}, {96, 144}, {152, 112}, {208, 80}, {128, 128}, {160, 112}, {56, 176}, {136, 128},
    {168, 112}, {64, 176}, {144, 128}, {176, 112}, {96, 160}, {152, 128}, {128, 144}, {104, 160},
    {160, 128}, {136, 144}, {112, 160}, {168, 128}, {144, 144}, {120, 160}, {176, 128}, {152, 144},
    {128, 160}, {184, 128}, {160, 144}, {136, 160}, {192, 128}, {168, 144}, {144, 160}, {120, 176},
    {176, 144}, {152, 160}, {128, 176}, {184, 144}, {240, 112}, {160, 160}, {192, 144}, {88, 208},
    {168, 160}, {200, 144}, {96, 208}, {176, 160}, {208, 144}, {128, 192}, {184, 160}, {160, 176},
    {136, 192}, {192, 160}, {168, 176}, {144, 192}, {200, 160}, {176, 176}, {152, 192}, {208, 160},
    {184, 176}, {160, 192}, {216, 160}, {192, 176}, {168, 192}, {224, 160}, {200, 176}, {176, 192},
    {152, 208}, {208, 176}, {184, 192}, {160, 208}, {216, 176}, {112, 240}, {192, 192}, {224, 176},
    {120, 240}, {200, 192}, {232, 176}, {128, 240}, {208, 192}, {240, 176}, {160, 224}, {216, 192},
    {192, 208}, {168, 224}, {224, 192}, {200, 208}, {176, 224}, {232, 192}, {208, 208}, {184, 224},
    {240, 192}, {216, 208}, {192, 224}, {248, 192}, {224, 208}, {200, 224}, {176, 240}, {232, 208},
    {208, 224}, {184, 240}, {240, 208}, {216, 224}, {192, 240}, {248, 208}, {224, 224}, {224, 224},
    {224, 224}, {232, 224}, {232, 224}, {232, 224}, {240, 224}, {240, 224}, {240, 224}, {248, 224},
    {248, 224}, {224, 240}, {224, 240}, {232, 240}, {232, 240}, {232, 240}, {240, 240}, {240, 240},
    {240, 240}, {248, 240}, {248, 240}, {248, 240}, {248, 240}, {248, 240}, {248, 240}, {248, 240}
  };
#endif  // USE_CONSTANT_LUTS

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
    assert((t & (t - 1)) == 0);
  }
#else
  #define AssertPOT(x) (void)(0)
#endif

  static uint8 LookupIntensityByte(CompressionLabel *labels,
				   const uint32 *pixels,
				   uint32 index) {
    float i = 255.0f * LookupIntensity(labels, pixels, index);
    return static_cast<uint8>(i + 0.5f);
  }

  static EExtremaResult ComputeLocalExtrema(
    CompressionLabel *labels, const uint8 *inBuf,
    const uint32 x, const uint32 y, const Indexer &idxr) {

    const uint32 *pixels = reinterpret_cast<const uint32 *>(inBuf);
    uint32 idx0 = idxr(x, y);
    uint8 i0 = LookupIntensityByte(labels, pixels, idx0);

    int32 ng = 0;
    int32 nl = 0;

    const int32 kKernelSz = 3;
    const int32 kHalfKernelSz = kKernelSz >> 1;
    for(int32 j = -kHalfKernelSz; j <= kHalfKernelSz; j++)
    for(int32 i = -kHalfKernelSz; i <= kHalfKernelSz; i++) {

      if(i == 0 && j == 0) continue;

      int32 xx = i + static_cast<int32>(x);
      int32 yy = j + static_cast<int32>(y);
      uint8 ix = LookupIntensityByte(labels, pixels, idxr(xx, yy));

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

    CompressionLabel &l = labels[idx0];
    const int32 kThreshold = kKernelSz * kKernelSz - 1;
    if(ng >= kThreshold) {
      l.lowLabel.distance = 1;
      l.lowLabel.AddIdx(idx0);
      result = eExtremaResult_LocalMin;
    } else if(nl >= kThreshold) {
      l.highLabel.distance = 1;
      l.highLabel.AddIdx(idx0);
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
                                const Indexer &idxr) {
    uint32 w = idxr.GetWidth();
    uint32 h = idxr.GetHeight();
    
    AssertPOT(w);
    AssertPOT(h);

    for(uint32 j = 0; j < h+3; j++) {
      for(uint32 i = 0; i < w; i++) {
        EExtremaResult result = ComputeLocalExtrema(labels, inBuf, i, j, idxr);
        bool dilateMax = result != eExtremaResult_LocalMax;
        bool dilateMin = result != eExtremaResult_LocalMin;

        if(dilateMax || dilateMin) {
          // Look up and to the left to determine the distance...
          uint32 upIdx = idxr(i, j - 1);
          uint32 leftIdx = idxr(i - 1, j);

          CompressionLabel &l = labels[idxr(i, j)];
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
#if 0
    // We've already visited this label, but we should have dilated from here,
    // so try and dilate now...
    if(l.distance < 4 && nbs[4]->distance == 0) {
      nbs[4]->distance = l.distance + 1;
      nbs[4]->Combine(l);
    }
#endif
  }

  static void LabelImageBackward(CompressionLabel *labels, const Indexer &idxr) {

    uint32 w = idxr.GetWidth();
    uint32 h = idxr.GetHeight();

    AssertPOT(w);
    AssertPOT(h);

    CompressionLabel *neighbors[5] = { 0 };
    for(int32 j = static_cast<int32>(h)+2; j >= 0; j--) {
      for(int32 i = static_cast<int32>(w)-1; i >= 0; i--) {

        CompressionLabel &l = labels[idxr(i, j)];

        // Add top right corner
        neighbors[0] = &(labels[idxr(i+1, j-1)]);

        // Add right label
        neighbors[1] = &(labels[idxr(i+1, j)]);

        // Add bottom right label
        neighbors[2] = &(labels[idxr(i+1, j+1)]);

        // Add bottom label
        neighbors[3] = &(labels[idxr(i, j+1)]);

        // Add bottom left label
        neighbors[4] = &(labels[idxr(i-1, j+1)]);

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

  static uint32 GetBlockIndex(uint32 i, uint32 j) {
    return Interleave(j, i);
  }

  static void GenerateLowHighImages(CompressionLabel *labels,
                                    const uint8 *inBuf, uint8 *outBuf,
                                    const Indexer &idxr) {
    uint32 w = idxr.GetWidth();
    uint32 h = idxr.GetHeight();
    
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

        bool isHole[2][16];
        memset(isHole, 0, sizeof(isHole));

        float minIntensity = 1.1f, maxIntensity = -0.1f;
        uint32 minIntensityIdx = 0, maxIntensityIdx = 0;
        for(uint32 y = j*4; y <= (j+1)*4; y++)
        for(uint32 x = i*4; x <= (i+1)*4; x++) {

          uint32 idx = idxr(x, y);
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
            isHole[0][localIdx] = true;
          }

          if(labels[idx].lowLabel.distance > 0) {
            blockColors[1][localIdx] = CollectLabel(pixels, labels[idx].lowLabel);
          } else {
            isHole[1][localIdx] = true;
          }
        }

        Block b;
#ifdef USE_CONSTANT_LUTS
        if(minIntensity == maxIntensity) {
          // Assume all same color
          FasTC::Pixel color(pixels[minIntensityIdx]);
          if(color.A() < 0xFF) {
            if (color.A() == 0) {
              color.Unpack(0);    // Set to total black
              b.SetColorA(color);
              b.SetColorB(color);
            } else {
              // !FIXME! Actually compute better lookup tables for
              // this case...
              b.SetColorA(color, color.A() < 200);
              b.SetColorB(color, color.A() < 200);
            }
          } else {
            FasTC::Pixel high, low;
            high.A() = low.A() = 0xFF;

            high.R() = kConstFiveBitLUT[color.R()][0];
            low.R() = kConstFiveBitLUT[color.R()][1];

            high.G() = kConstFiveBitLUT[color.G()][0];
            low.G() = kConstFiveBitLUT[color.G()][1];

            high.B() = kConstFourBitLUT[color.B()][0];
            low.B() = kConstFourBitLUT[color.B()][1];

            b.SetColorA(high);
            b.SetColorB(low);
          }
        } else {
#endif
          // Average all of the values together now...
          FasTC::Color high, low;
          Indexer localIdxr(4, 4);
          for(uint32 y = 0; y < localIdxr.GetHeight(); y++)
          for(uint32 x = 0; x < localIdxr.GetWidth(); x++) {
            uint32 idx = localIdxr(x, y);
            FasTC::Color c = blockColors[0][idx];
            if(isHole[0][idx]) {
              c.Unpack(pixels[maxIntensityIdx]);
            }
            high += c * (1.0f / 16.0f);

            c = blockColors[1][idx];
            if(isHole[1][idx]) {
              c.Unpack(pixels[minIntensityIdx]);
            }
            low += c * (1.0f / 16.0f);
          }

          // Store them as our endpoints for this block...
          FasTC::Pixel p;
          p.Unpack(high.Pack());
          b.SetColorA(p, p.A() < 200);

          p.Unpack(low.Pack());
          b.SetColorB(p, p.B() < 200);
#ifdef USE_CONSTANT_LUTS
        }
#endif
        uint64 *outBlocks = reinterpret_cast<uint64 *>(outBuf);
        outBlocks[GetBlockIndex(i, j)] = b.Pack();
      }
    }
  }

  static FasTC::Pixel BilerpPixels(uint32 x, uint32 y,
    const FasTC::Pixel &topLeft,    const FasTC::Pixel &topRight,
    const FasTC::Pixel &bottomLeft, const FasTC::Pixel &bottomRight) {

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

    FasTC::Pixel fp;
    for(uint32 c = 0; c < 4; c++) {
      fp.Component(c) = sum.Component(c) & 15;
    }

    FasTC::Pixel tmp(sum / 16);
    tmp.A() = (tmp.A() << 4) | tmp.A();
    tmp.G() = (tmp.G() << 3) | (tmp.G() >> 2);
    tmp.B() = (tmp.B() << 3) | (tmp.B() >> 2);
    tmp.R() = (tmp.R() << 3) | (tmp.R() >> 2);

    tmp.Component(0) += ((fp.Component(0) * 17) >> 4);
    tmp.Component(1) += (((fp.Component(1) >> 1) * 33) >> 5);
    tmp.Component(2) += (((fp.Component(2) >> 1) * 33) >> 5);
    tmp.Component(3) += (((fp.Component(3) >> 1) * 33) >> 5);
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

  static void GenerateModulationValues(uint8 *outBuf, const uint8 *inBuf,
				       const Indexer &idxr) {
    uint32 w = idxr.GetWidth();
    uint32 h = idxr.GetHeight();

    AssertPOT(w);
    AssertPOT(h);

    // Start from the beginning block and generate the lerp values for the intermediate values
    uint64 *outBlocks = reinterpret_cast<uint64 *>(outBuf);
    const uint32 blocksW = w >> 2;
    const uint32 blocksH = h >> 2;

    const uint32 *pixels = reinterpret_cast<const uint32 *>(inBuf);

    // !SPEED! When we're iterating over the blocks here, we don't need to load from outBlocks
    // every iteration of the loop. Once we finish with a block, topLeft becomes topRight and
    // bottomLeft becomes bottomRight. Also, when we go to the next row, bottomRight becomes
    // topLeft.
    Indexer blkIdxr(blocksW, blocksH, idxr.GetWrapMode());
    for(uint32 j = 0; j < blocksH; j++) {
      for(uint32 i = 0; i < blocksW; i++) {

        const int32 lowXIdx = i;
        const int32 highXIdx = blkIdxr.ResolveX(i + 1);
        const int32 lowYIdx = j;
        const int32 highYIdx = blkIdxr.ResolveY(j + 1);

        const uint32 topLeftBlockIdx = GetBlockIndex(lowXIdx, lowYIdx);
        const uint32 topRightBlockIdx = GetBlockIndex(highXIdx, lowYIdx);
        const uint32 bottomLeftBlockIdx = GetBlockIndex(lowXIdx, highYIdx);
        const uint32 bottomRightBlockIdx = GetBlockIndex(highXIdx, highYIdx);

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

        FasTC::Pixel bottomRightA (bottomRightBlock.GetColorA());
        FasTC::Pixel bottomRightB (bottomRightBlock.GetColorB());

        ChangePixelTo4555(topLeftA);
        ChangePixelTo4555(topLeftB);

        ChangePixelTo4555(topRightA);
        ChangePixelTo4555(topRightB);

        ChangePixelTo4555(bottomLeftA);
        ChangePixelTo4555(bottomLeftB);

        ChangePixelTo4555(bottomRightA);
        ChangePixelTo4555(bottomRightB);

        for(uint32 y = 0; y < 4; y++) {
          for(uint32 x = 0; x < 4; x++) {
            uint32 pixelX = idxr.ResolveX(i*4 + 2 + x);
            uint32 pixelY = idxr.ResolveY(j*4 + 2 + y);
            FasTC::Pixel colorA = BilerpPixels(x, y, topLeftA, topRightA, bottomLeftA, bottomRightA);
            FasTC::Pixel colorB = BilerpPixels(x, y, topLeftB, topRightB, bottomLeftB, bottomRightB);
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
            if(x > 1) {
              if(y > 1) {
                pixelBlock = &bottomRightBlock;
              } else {
                pixelBlock = &topRightBlock;
              }
            } else if(y > 1) {
              pixelBlock = &bottomLeftBlock;
            }

            pixelBlock->SetLerpValue((pixelY & 3) * 4 + (pixelX & 3), bestMod);
          }
        }

        outBlocks[topLeftBlockIdx] = topLeftBlock.Pack();
        outBlocks[topRightBlockIdx] = topRightBlock.Pack();
        outBlocks[bottomLeftBlockIdx] = bottomLeftBlock.Pack();
        outBlocks[bottomRightBlockIdx] = bottomRightBlock.Pack();
      }
    }
  }

#ifndef NDEBUG
  typedef FasTC::Pixel (*LabelFunc)(const CompressionLabel &);

  static const uint32 *gDbgPixels = NULL;
  void DebugOutputImage(const char *imageName, const CompressionLabel *labels,
                        uint32 width, uint32 height, LabelFunc func) {
    Image output(width, height);
    for(uint32 j = 0; j < height; j++)
    for(uint32 i = 0; i < width; i++) {
      output(i, j) = func(labels[j*width + i]);
    }
    
    output.DebugOutput(imageName);
  }

  static const FasTC::Color kLabelPalette[4] = {
    FasTC::Color(0.0, 0.0, 1.0, 1.0),
    FasTC::Color(1.0, 0.0, 1.0, 1.0),
    FasTC::Color(1.0, 0.0, 0.0, 1.0),
    FasTC::Color(1.0, 1.0, 0.0, 1.0)
  };

  static FasTC::Pixel HighLabelDistance(const CompressionLabel &l) {
    FasTC::Pixel ret;
    const Label &hl = l.highLabel;
    if(hl.distance > 0) {
      ret.Unpack(kLabelPalette[hl.distance-1].Pack());
    }
    return ret;
  }

  static FasTC::Pixel HighPixel(const CompressionLabel &l) {
    assert(gDbgPixels);
    FasTC::Pixel ret;
    const Label &hl = l.highLabel;
    if(hl.distance > 0) {
      FasTC::Color c;
      uint32 nPs = 0;
      for(uint32 p = 0; p < hl.nLabels; p++) {
        FasTC::Color pc; pc.Unpack(gDbgPixels[hl.idxs[p]]);
        c += pc * static_cast<float>(hl.times[p]);
        nPs += hl.times[p];
      }
      c /= nPs;
      ret.Unpack(c.Pack());
    }
    return ret;
  }

  static FasTC::Pixel LowPixel(const CompressionLabel &l) {
    assert(gDbgPixels);
    FasTC::Pixel ret;
    const Label &ll = l.lowLabel;
    if(ll.distance > 0) {
      FasTC::Color c;
      uint32 nPs = 0;
      for(uint32 p = 0; p < ll.nLabels; p++) {
        FasTC::Color pc; pc.Unpack(gDbgPixels[ll.idxs[p]]);
        c += pc * static_cast<float>(ll.times[p]);
        nPs += ll.times[p];
      }
      c /= nPs;
      ret.Unpack(c.Pack());
    }
    return ret;
  }

  static FasTC::Pixel LowLabelDistance(const CompressionLabel &l) {
    FasTC::Pixel ret;
    const Label &ll = l.lowLabel;
    if(ll.distance > 0) {
      ret.Unpack(kLabelPalette[ll.distance-1].Pack());
    }
    return ret;
  }

  static FasTC::Pixel LabelIntensity(const CompressionLabel &l) {
    assert(l.intensity <= 1.0f && l.intensity >= 0.0f);
    uint32 iv = static_cast<uint32>(l.intensity * 255.0f + 0.5);
    assert(iv < 256);
    return FasTC::Pixel(static_cast<uint32>(0xFF000000 | (iv) | (iv << 8) | (iv << 16)));
  }

  static FasTC::Pixel ExtremaLabels(const CompressionLabel &l) {
    assert(!(l.highLabel.distance == 1 && l.lowLabel.distance == 1));

    if(l.highLabel.distance == 1) {
      return FasTC::Pixel(0xFF00FF00U);
    } 

    if(l.lowLabel.distance == 1) {
      return FasTC::Pixel(0xFFFF0000U);
    } 

    return LabelIntensity(l);
  }

  void DebugOutputLabels(const char *outputPrefix, const CompressionLabel *labels,
                         uint32 width, uint32 height) {
    ::std::string highName = ::std::string(outputPrefix);
    highName += ::std::string("HighLabels");
    DebugOutputImage(highName.c_str(), labels, width, height, HighLabelDistance);

    ::std::string lowName = ::std::string(outputPrefix);
    lowName += ::std::string("LowLabels");
    DebugOutputImage(lowName.c_str(), labels, width, height, LowLabelDistance);
  }
#endif

  void Compress(const FasTC::CompressionJob &cj, EWrapMode wrapMode) {
    const uint32 width = cj.Width();
    const uint32 height = cj.Height();

    // We only support 4bpp currently
    assert(cj.Format() == FasTC::eCompressionFormat_PVRTC4);

    // Make sure that width and height are a power of two.
    assert((width & (width - 1)) == 0);
    assert((height & (height - 1)) == 0);

    // Make sure that we aren't doing any shenanigans with threading or otherwise
    // assuming that we're not ending at the end of the texture...
    assert(cj.XStart() == 0 && cj.YStart() == 0);
    assert(cj.XEnd() == cj.Width() && cj.YEnd() == cj.Width());

    CompressionLabel *labels =
      (CompressionLabel *)calloc(width * height, sizeof(CompressionLabel));

    Indexer idxr(width, height, wrapMode);

    // First traverse forward...
    LabelImageForward(labels, cj.InBuf(), idxr);

#ifndef NDEBUG
    gDbgPixels = reinterpret_cast<const uint32 *>(cj.InBuf());

    Image original(width, height);
    for(uint32 j = 0; j < height; j++)
    for(uint32 i = 0; i < width; i++) {
      original(i, j).Unpack(gDbgPixels[j*width + i]);
    }
    original.DebugOutput("Original");

    DebugOutputImage("Intensity", labels, width, height, LabelIntensity);
    DebugOutputImage("Labels", labels, width, height, ExtremaLabels);

    DebugOutputLabels("Forward-", labels, width, height);

    DebugOutputImage("HighForwardImg", labels, width, height, HighPixel);
    DebugOutputImage("LowForwardImg", labels, width, height, LowPixel);
#endif

    // Then traverse backward...
    LabelImageBackward(labels, idxr);

#ifndef NDEBUG
    DebugOutputLabels("Backward-", labels, width, height);

    DebugOutputImage("HighImg", labels, width, height, HighPixel);
    DebugOutputImage("LowImg", labels, width, height, LowPixel);

    Image outputHigh(width, height);
    Image outputLow(width, height);
    for(uint32 j = 0; j < height; j++)
    for(uint32 i = 0; i < width; i++) {
      uint32 idx = j*width + i;
      if(labels[idx].highLabel.distance == 1) {
        outputHigh(i, j).Unpack(gDbgPixels[idx]);
      } else {
        outputHigh(i, j).Unpack(0xFF000000U);
      }

      if(labels[idx].lowLabel.distance == 1) {
        outputLow(i, j).Unpack(gDbgPixels[idx]);
      } else {
        outputLow(i, j).Unpack(0xFF000000U);
      }
    }
    
    outputHigh.DebugOutput("HighPixels");
    outputLow.DebugOutput("LowPixels");
#endif

    // Then combine everything...
    GenerateLowHighImages(labels, cj.InBuf(), cj.OutBuf(), idxr);

    // Then compute modulation values
    GenerateModulationValues(cj.OutBuf(), cj.InBuf(), idxr);

    // Cleanup
    free(labels);
  }
}  // namespace PVRTCC
