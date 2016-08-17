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

// The original lisence from the code available at the following location:
// http://software.intel.com/en-us/vcsource/samples/fast-texture-compression
//
// This code has been modified significantly from the original.

//------------------------------------------------------------------------------
// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works
// of this software for any purpose and without fee, provided, that the above
// copyright notice and this statement appear in all copies.  Intel makes no
// representations about the suitability of this software for any purpose.  THIS
// SOFTWARE IS PROVIDED "AS IS." INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,
// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER
// INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR
// INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not assume
// any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//
//------------------------------------------------------------------------------

#include "FasTC/BPTCCompressor.h"

#include "FasTC/TexCompTypes.h"
#include "FasTC/BitStream.h"

using FasTC::BitStream;
using FasTC::BitStreamReadOnly;

#include "FasTC/Shapes.h"

#include "AnchorTables.h"
#include "CompressionMode.h"
#include "BCLookupTables.h"
#include "RGBAEndpoints.h"

#ifdef HAS_MSVC_ATOMICS
#   include "Windows.h"
#endif

#ifdef _MSC_VER
#  undef min
#  undef max
#endif  // _MSC_VER

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cfloat>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <limits>

enum EBlockStats {
  eBlockStat_Path,
  eBlockStat_Mode,

  eBlockStat_SingleShapeEstimate,
  eBlockStat_TwoShapeEstimate,
  eBlockStat_ThreeShapeEstimate,

  eBlockStat_ModeZeroEstimate,
  eBlockStat_ModeOneEstimate,
  eBlockStat_ModeTwoEstimate,
  eBlockStat_ModeThreeEstimate,
  eBlockStat_ModeFourEstimate,
  eBlockStat_ModeFiveEstimate,
  eBlockStat_ModeSixEstimate,
  eBlockStat_ModeSevenEstimate,

  eBlockStat_ModeZeroError,
  eBlockStat_ModeOneError,
  eBlockStat_ModeTwoError,
  eBlockStat_ModeThreeError,
  eBlockStat_ModeFourError,
  eBlockStat_ModeFiveError,
  eBlockStat_ModeSixError,
  eBlockStat_ModeSevenError,

  kNumBlockStats
};

static const char *kBlockStatString[kNumBlockStats] = {
  "BlockStat_Path",
  "BlockStat_Mode",

  "BlockStat_SingleShapeEstimate",
  "BlockStat_TwoShapeEstimate",
  "BlockStat_ThreeShapeEstimate",

  "BlockStat_ModeZeroEstimate",
  "BlockStat_ModeOneEstimate",
  "BlockStat_ModeTwoEstimate",
  "BlockStat_ModeThreeEstimate",
  "BlockStat_ModeFourEstimate",
  "BlockStat_ModeFiveEstimate",
  "BlockStat_ModeSixEstimate",
  "BlockStat_ModeSevenEstimate",

  "BlockStat_ModeZeroError",
  "BlockStat_ModeOneError",
  "BlockStat_ModeTwoError",
  "BlockStat_ModeThreeError",
  "BlockStat_ModeFourError",
  "BlockStat_ModeFiveError",
  "BlockStat_ModeSixError",
  "BlockStat_ModeSevenError",
};

namespace BPTCC {

static const uint32 kWMValues[] = {
  0x32b92180, 0x32ba3080, 0x31103200, 0x28103c80,
  0x32bb3080, 0x25903600, 0x3530b900, 0x3b32b180, 0x34b5b98
};
static const uint32 kNumWMVals = sizeof(kWMValues) / sizeof(kWMValues[0]);
static uint32 gWMVal = -1;

template <typename T>
static inline T sad(const T &a, const T &b) {
  return (a > b)? a - b : b - a;
}

template <typename T>
static void insert(T* buf, int bufSz, T newVal, int idx = 0) {
  int safeIdx = std::min(bufSz-1, std::max(idx, 0));
  for(int i = bufSz - 1; i > safeIdx; i--) {
    buf[i] = buf[i-1];
  }
  buf[safeIdx] = newVal;
}

template <typename T>
static inline void swap(T &a, T &b) { T t = a; a = b; b = t; }

const uint32 kInterpolationValues[4][16][2] = {
  { {64, 0}, {33, 31}, {0, 64}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
  { {64, 0}, {43, 21}, {21, 43}, {0, 64}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
  { {64, 0}, {55, 9}, {46, 18}, {37, 27}, {27, 37}, {18, 46}, {9, 55}, {0, 64},
    {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
  { {64, 0}, {60, 4}, {55, 9}, {51, 13}, {47, 17}, {43, 21}, {38, 26}, {34, 30},
    {30, 34}, {26, 38}, {21, 43}, {17, 47}, {13, 51}, {9, 55}, {4, 60}, {0, 64}}
};

CompressionMode::Attributes
CompressionMode::kModeAttributes[kNumModes] = {
  // Mode 0
  { 0, 4, 3, 3, 0, 4, 0,
    false, false, CompressionMode::ePBitType_NotShared },

  // Mode 1
  { 1, 6, 2, 3, 0, 6, 0,
    false, false, CompressionMode::ePBitType_Shared },

  // Mode 2
  { 2, 6, 3, 2, 0, 5, 0,
    false, false, CompressionMode::ePBitType_None },

  // Mode 3
  { 3, 6, 2, 2, 0, 7, 0,
    false, false, CompressionMode::ePBitType_NotShared },

  // Mode 4
  { 4, 0, 1, 2, 3, 5, 6,
    true,  true,   CompressionMode::ePBitType_None },

  // Mode 5
  { 5, 0, 1, 2, 2, 7, 8,
    true,  false, CompressionMode::ePBitType_None },

  // Mode 6
  { 6, 0, 1, 4, 0, 7, 7,
    false, false, CompressionMode::ePBitType_NotShared },

  // Mode 7
  { 7, 6, 2, 2, 0, 5, 5,
    false, false, CompressionMode::ePBitType_NotShared },
};

ALIGN_SSE const float kErrorMetrics[kNumErrorMetrics][kNumColorChannels] = {
  { 1.0f, 1.0f, 1.0f, 1.0f },
  { sqrtf(0.3f), sqrtf(0.56f), sqrtf(0.11f), 1.0f }
};

const float *GetErrorMetric(ErrorMetric e) { return kErrorMetrics[e]; }

void CompressionMode::ClampEndpointsToGrid(
  RGBAVector &p1, RGBAVector &p2, uint8 &bestPBitCombo
) const {
  const int nPbitCombos = GetNumPbitCombos();
  const bool hasPbits = nPbitCombos > 1;
  const uint32 qmask = GetQuantizationMask();

  ClampEndpoints(p1, p2);

  // !SPEED! This can be faster.
  float minDist = FLT_MAX;
  RGBAVector bp1, bp2;
  for(int i = 0; i < nPbitCombos; i++) {

    uint32 qp1, qp2;
    if(hasPbits) {
      qp1 = p1.ToPixel(qmask, GetPBitCombo(i)[0]);
      qp2 = p2.ToPixel(qmask, GetPBitCombo(i)[1]);
    } else {
      qp1 = p1.ToPixel(qmask);
      qp2 = p2.ToPixel(qmask);
    }

    RGBAVector np1 = RGBAVector(0, qp1);
    RGBAVector np2 = RGBAVector(0, qp2);

    RGBAVector d1 = np1 - p1;
    RGBAVector d2 = np2 - p2;
    float dist = (d1 * d1) + (d2 * d2);
    if(dist < minDist) {
      minDist = dist;
      bp1 = np1; bp2 = np2;
      bestPBitCombo = i;
    }
  }

  p1 = bp1;
  p2 = bp2;
}

double CompressionMode::CompressSingleColor(
  const RGBAVector &p, RGBAVector &p1, RGBAVector &p2,
  uint8 &bestPbitCombo
) const {
  const uint32 pixel = p.ToPixel();
  float bestError = FLT_MAX;

  for(int pbi = 0; pbi < GetNumPbitCombos(); pbi++) {
    const int *pbitCombo = GetPBitCombo(pbi);

    uint32 dist[4] = { 0x0, 0x0, 0x0, 0x0 };
    uint32 bestValI[kNumColorChannels];
    uint32 bestValJ[kNumColorChannels];
    memset(bestValI, 0xFF, sizeof(bestValI));
    memset(bestValJ, 0xFF, sizeof(bestValJ));

    for(uint32 ci = 0; ci < kNumColorChannels; ci++) {
      const uint8 val = (pixel >> (ci * 8)) & 0xFF;
      int nBits = m_Attributes->colorChannelPrecision;
      if(ci == 3) {
        nBits = GetAlphaChannelPrecision();
      }

      // If we don't handle this channel, then it must be the full value (alpha)
      if(nBits == 0) {
        bestValI[ci] = bestValJ[ci] = 0xFF;
        dist[ci] = std::max(dist[ci], static_cast<uint32>(0xFF - val));
        continue;
      }

      const int nPossVals = (1 << nBits);
      int possValsH[256];
      int possValsL[256];

      // Do we have a pbit?
      const bool havepbit = GetPBitType() != ePBitType_None;
      if(havepbit)
        nBits++;

      for(int i = 0; i < nPossVals; i++) {

        int vh = i, vl = i;
        if(havepbit) {
          vh <<= 1;
          vl <<= 1;

          vh |= pbitCombo[1];
          vl |= pbitCombo[0];
        }

        possValsH[i] = (vh << (8 - nBits));
        possValsH[i] |= (possValsH[i] >> nBits);

        possValsL[i] = (vl << (8 - nBits));
        possValsL[i] |= (possValsL[i] >> nBits);
      }

      const uint8 bpi = GetNumberOfBitsPerIndex() - 1;
      const uint32 interpVal0 = kInterpolationValues[bpi][1][0];
      const uint32 interpVal1 = kInterpolationValues[bpi][1][1];

      // Find the closest interpolated val that to the given val...
      uint32 bestChannelDist = 0xFF;
      for(int i = 0; bestChannelDist > 0 && i < nPossVals; i++)
      for(int j = 0; bestChannelDist > 0 && j < nPossVals; j++) {

        const uint32 v1 = possValsL[i];
        const uint32 v2 = possValsH[j];

        const uint32 combo = (interpVal0*v1 + (interpVal1 * v2) + 32) >> 6;
        const uint32 err = (combo > val)? combo - val : val - combo;

        if(err < bestChannelDist) {
          bestChannelDist = err;
          bestValI[ci] = v1;
          bestValJ[ci] = v2;
        }
      }

      dist[ci] = std::max(bestChannelDist, dist[ci]);
    }

    const float *errorWeights = kErrorMetrics[this->m_ErrorMetric];
    float error = 0.0;
    for(uint32 i = 0; i < kNumColorChannels; i++) {
      float e = static_cast<float>(dist[i]) * errorWeights[i];
      error += e * e;
    }

    if(error < bestError) {
      bestError = error;
      bestPbitCombo = pbi;

      for(uint32 ci = 0; ci < kNumColorChannels; ci++) {
        p1[ci] = static_cast<float>(bestValI[ci]);
        p2[ci] = static_cast<float>(bestValJ[ci]);
      }
    }
  }

  return bestError;
}

// Fast random number generator. See more information at
// http://software.intel.com/en-us/articles/fast-random-number-
// generator-on-the-intel-pentiumr-4-processor/
static uint32 g_seed = static_cast<uint32>(time(NULL));
static inline uint32 fastrand() {
  g_seed = (214013 * g_seed + 2531011);
  return (g_seed>>16) & RAND_MAX;
}

static void ChangePointForDirWithoutPbitChange(
  RGBAVector &v, uint32 dir, const float step[kNumColorChannels]
) {
  if(dir % 2) {
    v.X() -= step[0];
  } else {
    v.X() += step[0];
  }

  if(((dir / 2) % 2)) {
    v.Y() -= step[1];
  } else  {
    v.Y() += step[1];
  }

  if(((dir / 4) % 2)) {
    v.Z() -= step[2];
  } else {
    v.Z() += step[2];
  }

  if(((dir / 8) % 2)) {
    v.W() -= step[3];
  } else {
    v.W() += step[3];
  }
}

static void ChangePointForDirWithPbitChange(
  RGBAVector &v, uint32 dir, uint32 oldPbit, const float step[kNumColorChannels]
) {
  if(dir % 2 && oldPbit == 0) {
    v.X() -= step[0];
  } else if(!(dir % 2) && oldPbit == 1) {
    v.X() += step[0];
  }

  if(((dir / 2) % 2) && oldPbit == 0) {
    v.Y() -= step[1];
  } else if(!((dir / 2) % 2) && oldPbit == 1) {
    v.Y() += step[1];
  }

  if(((dir / 4) % 2) && oldPbit == 0) {
    v.Z() -= step[2];
  } else if(!((dir / 4) % 2) && oldPbit == 1) {
    v.Z() += step[2];
  }

  if(((dir / 8) % 2) && oldPbit == 0) {
    v.W() -= step[3];
  } else if(!((dir / 8) % 2) && oldPbit == 1) {
    v.W() += step[3];
  }
}

struct VisitedState {
  RGBAVector p1;
  RGBAVector p2;
  int pBitCombo;
};

void CompressionMode::PickBestNeighboringEndpoints(
  const RGBACluster &cluster,
  const RGBAVector &p1, const RGBAVector &p2, const int curPbitCombo,
  RGBAVector &np1, RGBAVector &np2, int &nPbitCombo,
  const VisitedState *visitedStates, int nVisited,
  float stepSz
) const {
  // !SPEED! There might be a way to make this faster since we're working
  // with floating point values that are powers of two. We should be able
  // to just set the proper bits in the exponent and leave the mantissa to 0.
  float step[kNumColorChannels] = {
    stepSz * static_cast<float>(1 << (8 - m_Attributes->colorChannelPrecision)),
    stepSz * static_cast<float>(1 << (8 - m_Attributes->colorChannelPrecision)),
    stepSz * static_cast<float>(1 << (8 - m_Attributes->colorChannelPrecision)),
    stepSz * static_cast<float>(1 << (8 - GetAlphaChannelPrecision()))
  };

  if(m_IsOpaque) {
    step[(GetRotationMode() + 3) % kNumColorChannels] = 0.0f;
  }

  // First, let's figure out the new pbit combo... if there's no pbit then we
  // don't need to worry about it.
  const bool hasPbits = GetPBitType() != ePBitType_None;
  if(hasPbits) {

    // If there is a pbit, then we must change it, because those will provide
    // the closest values to the current point.
    if(GetPBitType() == ePBitType_Shared) {
      nPbitCombo = (curPbitCombo + 1) % 2;
    } else {
      // Not shared... p1 needs to change and p2 needs to change... which means
      // that combo 0 gets rotated to combo 3, combo 1 gets rotated to combo 2
      // and vice versa...
      nPbitCombo = 3 - curPbitCombo;
    }

    assert(GetPBitCombo(curPbitCombo)[0] + GetPBitCombo(nPbitCombo)[0] == 1);
    assert(GetPBitCombo(curPbitCombo)[1] + GetPBitCombo(nPbitCombo)[1] == 1);
  }

  bool visited = true;
  int infLoopPrevent = -1;
  while(visited && ++infLoopPrevent < 16) {
    for(int pt = 0; pt < 2; pt++) {

      const RGBAVector &p = (pt)? p1 : p2;
      RGBAVector &np = (pt)? np1 : np2;

      np = p;
      if(hasPbits) {
        const uint32 rdir = fastrand() % 16;
        const uint32 pbit = GetPBitCombo(curPbitCombo)[pt];
        ChangePointForDirWithPbitChange(np, rdir, pbit, step);
      } else {
        ChangePointForDirWithoutPbitChange(np, fastrand() % 16, step);
      }

      for(uint32 i = 0; i < kNumColorChannels; i++) {
        np[i] = std::min(std::max(np[i], 0.0f), 255.0f);
      }
    }

    visited = false;
    for(int i = 0; i < nVisited; i++) {
      visited = visited || (
        visitedStates[i].p1 == np1 &&
        visitedStates[i].p2 == np2 &&
        visitedStates[i].pBitCombo == nPbitCombo
      );
    }
  }
}

// Fast generation of floats between 0 and 1. It generates a float
// whose exponent forces the value to be between 1 and 2, then it
// populates the mantissa with a random assortment of bits, and returns
// the bytes interpreted as a float. This prevents two things: 1, a
// division, and 2, a cast from an integer to a float.

static inline float frand() {
  // RAND_MAX is 0x7FFF, which offers 15 bits
  // of precision. Therefore, we move the bits
  // into the top of the 23 bit mantissa, and
  // repeat the most significant bits of r in
  // the least significant of the mantissa
  const uint16 r = fastrand();
  const uint32 m = (r << 8) | (r >> 7);
  const union {
    uint32 fltAsInt;
    float flt;
  } fltUnion = { (127 << 23) | m };
  return fltUnion.flt - 1.0f;
}

#define COMPILE_ASSERT(x) extern int __compile_assert_[static_cast<int>(x)];
COMPILE_ASSERT(RAND_MAX == 0x7FFF)

bool CompressionMode::AcceptNewEndpointError(
  double newError, double oldError, float temp
) const {
  // Always accept better endpoints.
  if(newError < oldError) {
    return true;
  }

  const double p = exp((0.1f * (oldError - newError)) / temp);
  const double r = frand();

  return r < p;
}

double CompressionMode::OptimizeEndpointsForCluster(
  const RGBACluster &cluster,
  RGBAVector &p1, RGBAVector &p2,
  uint8 *bestIndices,
  uint8 &bestPbitCombo
) const {

  const uint32 nBuckets = (1 << GetNumberOfBitsPerIndex());
  const uint32 qmask = GetQuantizationMask();

  // Here we use simulated annealing to traverse the space of clusters to find
  // the best possible endpoints.
  double curError = cluster.QuantizedError(
    p1, p2, nBuckets, qmask, GetErrorMetric(),
    GetPBitCombo(bestPbitCombo), bestIndices
  );

  int curPbitCombo = bestPbitCombo;
  double bestError = curError;

  // Clamp endpoints to the grid...
  uint32 qp1, qp2;
  if(GetPBitType() != ePBitType_None) {
    qp1 = p1.ToPixel(qmask, GetPBitCombo(bestPbitCombo)[0]);
    qp2 = p2.ToPixel(qmask, GetPBitCombo(bestPbitCombo)[1]);
  } else {
    qp1 = p1.ToPixel(qmask);
    qp2 = p2.ToPixel(qmask);
  }

  p1 = RGBAVector(0, qp1);
  p2 = RGBAVector(0, qp2);

  RGBAVector bp1 = p1, bp2 = p2;

  int lastVisitedState = 0;
  VisitedState visitedStates[kMaxAnnealingIterations];

  visitedStates[lastVisitedState].p1 = p1;
  visitedStates[lastVisitedState].p2 = p2;
  visitedStates[lastVisitedState].pBitCombo = curPbitCombo;
  lastVisitedState++;

  const int maxEnergy = this->m_SASteps;

  for(int energy = 0; bestError > 0 && energy < maxEnergy; energy++) {

    float temp = static_cast<float>(energy) / static_cast<float>(maxEnergy-1);

    uint8 indices[kMaxNumDataPoints];
    RGBAVector np1, np2;
    int nPbitCombo = 0;

    PickBestNeighboringEndpoints(
      cluster, p1, p2, curPbitCombo, np1, np2, nPbitCombo,
      visitedStates, lastVisitedState
    );

    double error = cluster.QuantizedError(
      np1, np2, nBuckets, qmask,
      GetErrorMetric(), GetPBitCombo(nPbitCombo), indices
    );

    if(AcceptNewEndpointError(error, curError, temp)) {
      curError = error;
      p1 = np1;
      p2 = np2;
      curPbitCombo = nPbitCombo;
    }

    if(error < bestError) {
      memcpy(bestIndices, indices, sizeof(indices));
      bp1 = np1;
      bp2 = np2;
      bestPbitCombo = nPbitCombo;
      bestError = error;

      lastVisitedState = 0;
      visitedStates[lastVisitedState].p1 = np1;
      visitedStates[lastVisitedState].p2 = np2;
      visitedStates[lastVisitedState].pBitCombo = nPbitCombo;
      lastVisitedState++;

      // Restart...
      energy = 0;
    }
  }

  p1 = bp1;
  p2 = bp2;

  return bestError;
}

double CompressionMode::CompressCluster(
  const RGBACluster &cluster,
  RGBAVector &p1, RGBAVector &p2,
  uint8 *bestIndices,
  uint8 *alphaIndices
) const {
  assert(GetModeNumber() == 4 || GetModeNumber() == 5);
  assert(GetNumberOfSubsets() == 1);
  assert(cluster.GetNumPoints() == kMaxNumDataPoints);
  assert(m_Attributes->alphaChannelPrecision > 0);

  // If all the points are the same in the cluster, then we need to figure out
  // what the best approximation to this point is....
  if(cluster.AllSamePoint()) {

    assert(!"We should only be using this function in modes 4 & 5 that have a"
            "single subset, in which case single colors should have been"
            "detected much earlier.");

    const RGBAVector &p = cluster.GetPoint(0);
    uint8 dummyPbit = 0;
    double bestErr = CompressSingleColor(p, p1, p2, dummyPbit);

    // We're assuming all indices will be index 1...
    for(uint32 i = 0; i < cluster.GetNumPoints(); i++) {
      bestIndices[i] = 1;
      alphaIndices[i] = 1;
    }

    return cluster.GetNumPoints() * bestErr;
  }

  RGBACluster rgbCluster(cluster);
  float alphaVals[kMaxNumDataPoints] = {0};

  float alphaMin = FLT_MAX, alphaMax = -FLT_MAX;
  for(uint32 i = 0; i < rgbCluster.GetNumPoints(); i++) {

    RGBAVector &v = rgbCluster.Point(i);
    switch(this->GetRotationMode()) {
      default:
      case 0:
        // Do nothing
      break;

      case 1:
        swap(v.R(), v.A());
        break;

      case 2:
        swap(v.G(), v.A());
        break;

      case 3:
        swap(v.B(), v.A());
        break;
    }

    alphaVals[i] = v.A();
    v.A() = 255.0f;

    alphaMin = std::min(alphaVals[i], alphaMin);
    alphaMax = std::max(alphaVals[i], alphaMax);
  }

  uint8 dummyPbit = 0;
  RGBAVector rgbp1, rgbp2;
  double rgbError = CompressCluster(
    rgbCluster, rgbp1, rgbp2, bestIndices, dummyPbit
  );

  float a1 = alphaMin, a2 = alphaMax;
  double alphaError = DBL_MAX;

  typedef uint32 tInterpPair[2];
  typedef tInterpPair tInterpLevel[16];

  const tInterpLevel *interpVals =
    kInterpolationValues + (GetNumberOfBitsPerAlpha() - 1);

  const float weight = GetErrorMetric().A();

  const uint32 nBuckets = (1 << GetNumberOfBitsPerAlpha());

  // If they're the same, then we can get them exactly.
  if(a1 == a2) {
    const uint8 a1be = uint8(a1);
    const uint8 a2be = uint8(a2);

    // Mode 5 has 8 bits of precision for alpha.
    if(GetModeNumber() == 5) {
      for(uint32 i = 0; i < kMaxNumDataPoints; i++)
        alphaIndices[i] = 0;

      alphaError = 0.0;
    } else {
      assert(GetModeNumber() == 4);

      // Mode 4 can be treated like the 6 channel of DXT1 compression.
      if(Optimal6CompressDXT1[a1be][0][0]) {
        a1 = static_cast<float>(
            (Optimal6CompressDXT1[a1be][1][1] << 2) |
            (Optimal6CompressDXT1[a1be][0][1] >> 4));

        a2 = static_cast<float>(
            (Optimal6CompressDXT1[a2be][1][2] << 2) |
            (Optimal6CompressDXT1[a2be][0][1] >> 4));
      } else {
        a1 = static_cast<float>(
            (Optimal6CompressDXT1[a1be][0][1] << 2) |
            (Optimal6CompressDXT1[a1be][0][1] >> 4));

        a2 = static_cast<float>(
            (Optimal6CompressDXT1[a2be][0][2] << 2) |
            (Optimal6CompressDXT1[a2be][0][1] >> 4));
      }

      if(m_IndexMode == 1) {
        for(uint32 i = 0; i < kMaxNumDataPoints; i++)
          alphaIndices[i] = 1;
      } else {
        for(uint32 i = 0; i < kMaxNumDataPoints; i++)
          alphaIndices[i] = 2;
      }

      uint32 interp0 = (*interpVals)[alphaIndices[0] & 0xFF][0];
      uint32 interp1 = (*interpVals)[alphaIndices[0] & 0xFF][1];

      const uint32 a1i = static_cast<uint32>(a1);
      const uint32 a2i = static_cast<uint32>(a2);

      const uint8 ip = (((a1i * interp0) + (a2i * interp1) + 32) >> 6) & 0xFF;
      float pxError =
        weight * static_cast<float>((a1be > ip)? a1be - ip : ip - a1be);
      pxError *= pxError;
      alphaError = 16 * pxError;
    }
  } else {  // (a1 != a2)
    float vals[1<<3];
    memset(vals, 0, sizeof(vals));

    uint32 buckets[kMaxNumDataPoints];

    // Figure out initial positioning.
    for(uint32 i = 0; i < nBuckets; i++) {
      const float fi = static_cast<float>(i);
      const float fb = static_cast<float>(nBuckets - 1);
      vals[i] = alphaMin + (fi/fb) * (alphaMax - alphaMin);
    }

    // Assign each value to a bucket
    for(uint32 i = 0; i < kMaxNumDataPoints; i++) {

      float minDist = 255.0f;
      for(uint32 j = 0; j < nBuckets; j++) {
        float dist = fabs(alphaVals[i] - vals[j]);
        if(dist < minDist) {
          minDist = dist;
          buckets[i] = j;
        }
      }
    }

    float npts[1 << 3];

    // Do k-means
    bool fixed = false;
    while(!fixed) {

      memset(npts, 0, sizeof(npts));

      float avg[1 << 3];
      memset(avg, 0, sizeof(avg));

      // Calculate average of each cluster
      for(uint32 i = 0; i < nBuckets; i++) {
        for(uint32 j = 0; j < kMaxNumDataPoints; j++) {

          if(buckets[j] == i) {
            avg[i] += alphaVals[j];
            npts[i] += 1.0f;
          }
        }

        if(npts[i] > 0.0f) {
          avg[i] /= npts[i];
        }
      }

      // Did we change anything?
      fixed = true;
      for(uint32 i = 0; i < nBuckets; i++) {
        fixed = fixed && (avg[i] == vals[i]);
      }

      // Reassign indices...
      memcpy(vals, avg, sizeof(vals));

      // Reassign each value to a bucket
      for(uint32 i = 0; i < kMaxNumDataPoints; i++) {

        float minDist = 255.0f;
        for(uint32 j = 0; j < nBuckets; j++) {
          float dist = fabs(alphaVals[i] - vals[j]);
          if(dist < minDist) {
            minDist = dist;
            buckets[i] = j;
          }
        }
      }
    }

    // Do least squares fit of vals.
    float asq = 0.0, bsq = 0.0, ab = 0.0;
    float ax(0.0), bx(0.0);
    for(uint32 i = 0; i < nBuckets; i++) {
      const float fbi = static_cast<float>(nBuckets - 1 - i);
      const float fb = static_cast<float>(nBuckets - 1);
      const float fi = static_cast<float>(i);

      float a = fbi / fb;
      float b = fi / fb;

      float n = npts[i];
      float x = vals[i];

      asq += n * a * a;
      bsq += n * b * b;
      ab += n * a * b;

      ax += x * a * n;
      bx += x * b * n;
    }

    float f = 1.0f / (asq * bsq - ab * ab);
    a1 = f * (ax * bsq - bx * ab);
    a2 = f * (bx * asq - ax * ab);

    // Clamp
    a1 = std::min(255.0f, std::max(0.0f, a1));
    a2 = std::min(255.0f, std::max(0.0f, a2));

    // Quantize
    const int8 maskSeed = -0x7F;
    const uint8 a1b = ::QuantizeChannel(
      uint8(a1), (maskSeed >> (GetAlphaChannelPrecision() - 1)));
    const uint8 a2b = ::QuantizeChannel(
      uint8(a2), (maskSeed >> (GetAlphaChannelPrecision() - 1)));

    // Compute error
    alphaError = 0.0;
    for(uint32 i = 0; i < kMaxNumDataPoints; i++) {

      uint8 val = uint8(alphaVals[i]);

      float minError = FLT_MAX;
      int bestBucket = -1;

      for(uint32 j = 0; j < nBuckets; j++) {
        uint32 interp0 = (*interpVals)[j][0];
        uint32 interp1 = (*interpVals)[j][1];

        uint32 a1i = static_cast<uint32>(a1b);
        uint32 a2i = static_cast<uint32>(a2b);

        const uint8 ip = (((a1i * interp0) + (a2i * interp1) + 32) >> 6) & 0xFF;
        float pxError =
          weight * static_cast<float>((val > ip)? val - ip : ip - val);
        pxError *= pxError;

        if(pxError < minError) {
          minError = pxError;
          bestBucket = j;
        }
      }

      alphaError += minError;
      alphaIndices[i] = bestBucket;
    }
  }

  for(uint32 i = 0; i < kNumColorChannels; i++) {
    p1[i] = (i == (kNumColorChannels-1))? a1 : rgbp1[i];
    p2[i] = (i == (kNumColorChannels-1))? a2 : rgbp2[i];
  }

  return rgbError + alphaError;
}

double CompressionMode::CompressCluster(
  const RGBACluster &cluster,
  RGBAVector &p1, RGBAVector &p2,
  uint8 *bestIndices,
  uint8 &bestPbitCombo
) const {
  // If all the points are the same in the cluster, then we need to figure out
  // what the best approximation to this point is....
  if(cluster.AllSamePoint()) {
    const RGBAVector &p = cluster.GetPoint(0);
    double bestErr = CompressSingleColor(p, p1, p2, bestPbitCombo);

    // We're assuming all indices will be index 1...
    for(uint32 i = 0; i < cluster.GetNumPoints(); i++) {
      bestIndices[i] = 1;
    }
    return cluster.GetNumPoints() * bestErr;
  }

  const uint32 nBuckets = (1 << GetNumberOfBitsPerIndex());

#if 1
  RGBADir axis;
  cluster.GetPrincipalAxis(axis, NULL, NULL);

  float mindp = FLT_MAX, maxdp = -FLT_MAX;
  for(uint32 i = 0 ; i < cluster.GetNumPoints(); i++) {
    float dp = (cluster.GetPoint(i) - cluster.GetAvg()) * axis;
    if(dp < mindp) mindp = dp;
    if(dp > maxdp) maxdp = dp;
  }

  p1 = cluster.GetAvg() + mindp * axis;
  p2 = cluster.GetAvg() + maxdp * axis;
#else
  cluster.GetBoundingBox(p1, p2);
#endif

  ClampEndpoints(p1, p2);

  RGBAVector pts[1 << 4];  // At most 4 bits per index.
  uint32 numPts[1<<4];
  assert(nBuckets <= 1 << 4);

  for(uint32 i = 0; i < nBuckets; i++) {
    float s = (static_cast<float>(i) / static_cast<float>(nBuckets - 1));
    pts[i] = (1.0f - s) * p1 + s * p2;
  }

  assert(pts[0] == p1);
  assert(pts[nBuckets - 1] == p2);

  // Do k-means clustering...
  uint32 bucketIdx[kMaxNumDataPoints] = {0};

  bool fixed = false;
  while(!fixed) {
    RGBAVector newPts[1 << 4];

    // Assign each of the existing points to one of the buckets...
    for(uint32 i = 0; i < cluster.GetNumPoints(); i++) {
      int minBucket = -1;
      float minDist = FLT_MAX;

      for(uint32 j = 0; j < nBuckets; j++) {
        RGBAVector v = cluster.GetPoint(i) - pts[j];
        float distSq = v * v;
        if(distSq < minDist) {
          minDist = distSq;
          minBucket = j;
        }
      }

      assert(minBucket >= 0);
      bucketIdx[i] = minBucket;
    }

    // Calculate new buckets based on centroids of clusters...
    for(uint32 i = 0; i < nBuckets; i++) {
      numPts[i] = 0;
      newPts[i] = RGBAVector(0.0f);
      for(uint32 j = 0; j < cluster.GetNumPoints(); j++) {
        if(bucketIdx[j] == i) {
          numPts[i]++;
          newPts[i] += cluster.GetPoint(j);
        }
      }

      // If there are no points in this cluster, then it should
      // remain the same as last time and avoid a divide by zero.
      if(0 != numPts[i])
        newPts[i] /= static_cast<float>(numPts[i]);
    }

    // If we haven't changed, then we're done.
    fixed = true;
    for(uint32 i = 0; i < nBuckets; i++) {
      if(pts[i] != newPts[i])
        fixed = false;
    }

    // Assign the new points to be the old points.
    for(uint32 i = 0; i < nBuckets; i++) {
      pts[i] = newPts[i];
    }
  }

  // If there's only one bucket filled, then just compress for that single color
  int numBucketsFilled = 0, lastFilledBucket = -1;
  for(uint32 i = 0; i < nBuckets; i++) {
    if(numPts[i] > 0) {
      numBucketsFilled++;
      lastFilledBucket = i;
    }
  }

  assert(numBucketsFilled > 0);
  if(1 == numBucketsFilled) {
    const RGBAVector &p = pts[lastFilledBucket];
    double bestErr = CompressSingleColor(p, p1, p2, bestPbitCombo);

    // We're assuming all indices will be index 1...
    for(uint32 i = 0; i < cluster.GetNumPoints(); i++) {
      bestIndices[i] = 1;
    }
    return cluster.GetNumPoints() * bestErr;
  }

  // Now that we know the index of each pixel, we can assign the endpoints based
  // on a least squares fit of the clusters. For more information, take a look
  // at this article by NVidia: http://developer.download.nvidia.com/compute/
  // cuda/1.1-Beta/x86_website/projects/dxtc/doc/cuda_dxtc.pdf
  float asq = 0.0, bsq = 0.0, ab = 0.0;
  RGBAVector ax(0.0), bx(0.0);
  for(uint32 i = 0; i < nBuckets; i++) {
    const RGBAVector x = pts[i];
    const int n = numPts[i];

    const float fbi = static_cast<float>(nBuckets - 1 - i);
    const float fb = static_cast<float>(nBuckets - 1);
    const float fi = static_cast<float>(i);
    const float fn = static_cast<float>(n);

    const float a = fbi / fb;
    const float b = fi / fb;

    asq += fn * a * a;
    bsq += fn * b * b;
    ab += fn * a * b;

    ax += x * a * fn;
    bx += x * b * fn;
  }

  float f = 1.0f / (asq * bsq - ab * ab);
  p1 = f * (ax * bsq - bx * ab);
  p2 = f * (bx * asq - ax * ab);

  ClampEndpointsToGrid(p1, p2, bestPbitCombo);

  #ifdef _DEBUG
    uint8 pBitCombo = bestPbitCombo;
    RGBAVector tp1 = p1, tp2 = p2;
    ClampEndpointsToGrid(tp1, tp2, pBitCombo);

    assert(p1 == tp1);
    assert(p2 == tp2);
    assert(pBitCombo == bestPbitCombo);
  #endif

  return OptimizeEndpointsForCluster(
    cluster, p1, p2, bestIndices, bestPbitCombo
  );
}

void CompressionMode::Pack(Params &params, BitStream &stream) const {
  
  const int kModeNumber = GetModeNumber();
  const int nPartitionBits = GetNumberOfPartitionBits();
  const int nSubsets = GetNumberOfSubsets();

  // Mode #
  stream.WriteBits(1 << kModeNumber, kModeNumber + 1);

  // Partition #
  assert(!nPartitionBits ||
         (((1 << nPartitionBits) - 1) & params.m_ShapeIdx) == params.m_ShapeIdx);
  stream.WriteBits(params.m_ShapeIdx, nPartitionBits);

  stream.WriteBits(params.m_RotationMode, m_Attributes->hasRotation? 2 : 0);
  stream.WriteBits(params.m_IndexMode, m_Attributes->hasIdxMode? 1 : 0);

#ifdef _DEBUG
  for(int i = 0; i < kMaxNumDataPoints; i++) {

    int nSet = 0;
    for(int j = 0; j < nSubsets; j++) {
      if(params.m_Indices[j][i] < 255)
        nSet++;
    }

    assert(nSet == 1);
  }
#endif

  // Get the quantization mask
  const uint32 qmask = GetQuantizationMask();

  // Quantize the points...
  uint32 pixel1[kMaxNumSubsets], pixel2[kMaxNumSubsets];
  for(int i = 0; i < nSubsets; i++) {
    switch(GetPBitType()) {
      default:
      case ePBitType_None:
        pixel1[i] = params.m_P1[i].ToPixel(qmask);
        pixel2[i] = params.m_P2[i].ToPixel(qmask);
      break;

      case ePBitType_Shared:
      case ePBitType_NotShared:
        pixel1[i] = params.m_P1[i].ToPixel(qmask, GetPBitCombo(params.m_PbitCombo[i])[0]);
        pixel2[i] = params.m_P2[i].ToPixel(qmask, GetPBitCombo(params.m_PbitCombo[i])[1]);
      break;
    }
  }

  // If the anchor index does not have 0 in the leading bit, then
  // we need to swap EVERYTHING.
  for(int sidx = 0; sidx < nSubsets; sidx++) {

    int anchorIdx = GetAnchorIndexForSubset(sidx, params.m_ShapeIdx, nSubsets);
    assert(params.m_Indices[sidx][anchorIdx] != 255);

    const int nAlphaIndexBits = GetNumberOfBitsPerAlpha(params.m_IndexMode);
    const int nIndexBits = GetNumberOfBitsPerIndex(params.m_IndexMode);
    if(params.m_Indices[sidx][anchorIdx] >> (nIndexBits - 1)) {
      std::swap(pixel1[sidx], pixel2[sidx]);

      int nIndexVals = 1 << nIndexBits;
      for(int i = 0; i < 16; i++) {
        params.m_Indices[sidx][i] = (nIndexVals - 1) - params.m_Indices[sidx][i];
      }

      int nAlphaIndexVals = 1 << nAlphaIndexBits;
      if(m_Attributes->hasRotation) {
        for(int i = 0; i < 16; i++) {
          params.m_AlphaIndices[i] = (nAlphaIndexVals - 1) - params.m_AlphaIndices[i];
        }
      }
    }

    const bool rotated = (params.m_AlphaIndices[anchorIdx] >> (nAlphaIndexBits - 1)) > 0;
    if(m_Attributes->hasRotation && rotated) {
      uint8 * bp1 = reinterpret_cast<uint8 *>(&pixel1[sidx]);
      uint8 * bp2 = reinterpret_cast<uint8 *>(&pixel2[sidx]);
      uint8 t = bp1[3]; bp1[3] = bp2[3]; bp2[3] = t;

      int nAlphaIndexVals = 1 << nAlphaIndexBits;
      for(int i = 0; i < 16; i++) {
        params.m_AlphaIndices[i] = (nAlphaIndexVals - 1) - params.m_AlphaIndices[i];
      }
    }

    assert(!(params.m_Indices[sidx][anchorIdx] >> (nIndexBits - 1)));
    assert(!m_Attributes->hasRotation ||
           !(params.m_AlphaIndices[anchorIdx] >> (nAlphaIndexBits - 1)));
  }

  // Get the quantized values...
  uint8 r1[kMaxNumSubsets], g1[kMaxNumSubsets],
        b1[kMaxNumSubsets], a1[kMaxNumSubsets];
  uint8 r2[kMaxNumSubsets], g2[kMaxNumSubsets],
        b2[kMaxNumSubsets], a2[kMaxNumSubsets];
  for(int i = 0; i < nSubsets; i++) {
    r1[i] = pixel1[i] & 0xFF;
    r2[i] = pixel2[i] & 0xFF;

    g1[i] = (pixel1[i] >> 8) & 0xFF;
    g2[i] = (pixel2[i] >> 8) & 0xFF;

    b1[i] = (pixel1[i] >> 16) & 0xFF;
    b2[i] = (pixel2[i] >> 16) & 0xFF;

    a1[i] = (pixel1[i] >> 24) & 0xFF;
    a2[i] = (pixel2[i] >> 24) & 0xFF;
  }

  // Write them out...
  const int nRedBits = m_Attributes->colorChannelPrecision;
  for(int i = 0; i < nSubsets; i++) {
    stream.WriteBits(r1[i] >> (8 - nRedBits), nRedBits);
    stream.WriteBits(r2[i] >> (8 - nRedBits), nRedBits);
  }

  const int nGreenBits = m_Attributes->colorChannelPrecision;
  for(int i = 0; i < nSubsets; i++) {
    stream.WriteBits(g1[i] >> (8 - nGreenBits), nGreenBits);
    stream.WriteBits(g2[i] >> (8 - nGreenBits), nGreenBits);
  }

  const int nBlueBits = m_Attributes->colorChannelPrecision;
  for(int i = 0; i < nSubsets; i++) {
    stream.WriteBits(b1[i] >> (8 - nBlueBits), nBlueBits);
    stream.WriteBits(b2[i] >> (8 - nBlueBits), nBlueBits);
  }

  const int nAlphaBits = m_Attributes->alphaChannelPrecision;
  for(int i = 0; i < nSubsets; i++) {
    stream.WriteBits(a1[i] >> (8 - nAlphaBits), nAlphaBits);
    stream.WriteBits(a2[i] >> (8 - nAlphaBits), nAlphaBits);
  }

  // Write out the best pbits..
  if(GetPBitType() != ePBitType_None) {
    for(int s = 0; s < nSubsets; s++) {
      const int *pbits = GetPBitCombo(params.m_PbitCombo[s]);
      stream.WriteBits(pbits[0], 1);
      if(GetPBitType() != ePBitType_Shared)
        stream.WriteBits(pbits[1], 1);
    }
  }

  // If our index mode has changed, then we need to write the alpha indices
  // first.
  if(m_Attributes->hasIdxMode && params.m_IndexMode == 1) {

    assert(m_Attributes->hasRotation);

    for(int i = 0; i < 16; i++) {
      const int idx = params.m_AlphaIndices[i];
      assert(GetAnchorIndexForSubset(0, params.m_ShapeIdx, nSubsets) == 0);
      assert(GetNumberOfBitsPerAlpha(params.m_IndexMode) == 2);
      assert(idx >= 0 && idx < (1 << 2));
      assert(i != 0 ||
             !(idx >> 1) ||
             !"Leading bit of anchor index is not zero!");
      stream.WriteBits(idx, (i == 0)? 1 : 2);
    }

    for(int i = 0; i < 16; i++) {
      const int idx = params.m_Indices[0][i];
      assert(GetSubsetForIndex(i, params.m_ShapeIdx, nSubsets) == 0);
      assert(GetAnchorIndexForSubset(0, params.m_ShapeIdx, nSubsets) == 0);
      assert(GetNumberOfBitsPerIndex(params.m_IndexMode) == 3);
      assert(idx >= 0 && idx < (1 << 3));
      assert(i != 0 ||
             !(idx >> 2) ||
             !"Leading bit of anchor index is not zero!");
      stream.WriteBits(idx, (i == 0)? 2 : 3);
    }
  } else {
    for(int i = 0; i < 16; i++) {
      const int subs = GetSubsetForIndex(i, params.m_ShapeIdx, nSubsets);
      const int idx = params.m_Indices[subs][i];
      const int anchorIdx = GetAnchorIndexForSubset(subs, params.m_ShapeIdx, nSubsets);
      const int nBitsForIdx = GetNumberOfBitsPerIndex(params.m_IndexMode);
      assert(idx >= 0 && idx < (1 << nBitsForIdx));
      assert(i != anchorIdx ||
             !(idx >> (nBitsForIdx - 1)) ||
             !"Leading bit of anchor index is not zero!");
      stream.WriteBits(idx, (i == anchorIdx)? nBitsForIdx - 1 : nBitsForIdx);
    }

    if(m_Attributes->hasRotation) {
      for(int i = 0; i < 16; i++) {
        const int idx = params.m_AlphaIndices[i];
        const int anchorIdx = 0;
        const int nBitsForIdx = GetNumberOfBitsPerAlpha(params.m_IndexMode);
        assert(idx >= 0 && idx < (1 << nBitsForIdx));
        assert(i != anchorIdx ||
               !(idx >> (nBitsForIdx - 1)) ||
               !"Leading bit of anchor index is not zero!");
        stream.WriteBits(idx, (i == anchorIdx)? nBitsForIdx - 1 : nBitsForIdx);
      }
    }
  }
  assert(stream.GetBitsWritten() == 128);
}

double CompressionMode::Compress(
  Params &params, const int shapeIdx, RGBACluster &cluster
) {

  const int kModeNumber = GetModeNumber();
  const int nSubsets = GetNumberOfSubsets();

  params = Params(shapeIdx);

  double totalErr = 0.0;
  for(int cidx = 0; cidx < nSubsets; cidx++) {
    uint8 indices[kMaxNumDataPoints] = {0};
    cluster.SetPartition(cidx);

    if(m_Attributes->hasRotation) {

      assert(nSubsets == 1);

      uint8 alphaIndices[kMaxNumDataPoints];

      double bestError = DBL_MAX;
      for(int rotMode = 0; rotMode < 4; rotMode++) {

        SetRotationMode(rotMode);
        const int nIdxModes = kModeNumber == 4? 2 : 1;

        for(int idxMode = 0; idxMode < nIdxModes; idxMode++) {

          SetIndexMode(idxMode);

          RGBAVector v1, v2;
          double error = CompressCluster(
            cluster, v1, v2, indices, alphaIndices
          );

          if(error < bestError) {
            bestError = error;

            memcpy(params.m_Indices[cidx], indices, sizeof(indices));
            memcpy(params.m_AlphaIndices, alphaIndices, sizeof(alphaIndices));

            params.m_RotationMode = rotMode;
            params.m_IndexMode = idxMode;

            params.m_P1[cidx] = v1;
            params.m_P2[cidx] = v2;
          }
        }
      }

      totalErr += bestError;
    } else {  // ! m_Attributes->hasRotation
      // Compress this cluster
      totalErr += CompressCluster(
        cluster,
        params.m_P1[cidx], params.m_P2[cidx],
        indices, params.m_PbitCombo[cidx]
      );

      // Map the indices to their proper position.
      int idx = 0;
      for(int i = 0; i < 16; i++) {
        int subs = GetSubsetForIndex(i, shapeIdx, GetNumberOfSubsets());
        if(subs == cidx) {
          params.m_Indices[cidx][i] = indices[idx++];
        }
      }
    }
  }

  return totalErr;
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

// Function prototypes
static void CompressBC7Block(
  const uint32 x, const uint32 y,
  const uint32 block[16], uint8 *outBuf,
  const CompressionSettings = CompressionSettings()
);
static void CompressBC7Block(
  const uint32 x, const uint32 y,
  const uint32 block[16], uint8 *outBuf, const BlockLogger &logStream,
  const CompressionSettings = CompressionSettings()
);

// Returns true if the entire block is a single color.
static bool AllOneColor(const uint32 block[16]) {
  const uint32 pixel = block[0];
  for(int i = 1; i < 16; i++) {
    if( block[i] != pixel )
      return false;
  }

  return true;
}

// Write out a transparent block.
static void WriteTransparentBlock(BitStream &stream) {
  // Use mode 6
  stream.WriteBits(1 << 6, 7);
  stream.WriteBits(0, 128-7);
  assert(stream.GetBitsWritten() == 128);
}

// Compresses a single color optimally and outputs the result.
static void CompressOptimalColorBC7(uint32 pixel, BitStream &stream) {

  stream.WriteBits(1 << 5, 6);  // Mode 5
  stream.WriteBits(0, 2);  // No rotation bits.

  uint8 r = pixel & 0xFF;
  uint8 g = (pixel >> 8) & 0xFF;
  uint8 b = (pixel >> 16) & 0xFF;
  uint8 a = (pixel >> 24) & 0xFF;

  // Red endpoints
  stream.WriteBits(Optimal7CompressBC7Mode5[r][0], 7);
  stream.WriteBits(Optimal7CompressBC7Mode5[r][1], 7);

  // Green endpoints
  stream.WriteBits(Optimal7CompressBC7Mode5[g][0], 7);
  stream.WriteBits(Optimal7CompressBC7Mode5[g][1], 7);

  // Blue endpoints
  stream.WriteBits(Optimal7CompressBC7Mode5[b][0], 7);
  stream.WriteBits(Optimal7CompressBC7Mode5[b][1], 7);

  // Alpha endpoints... are just the same.
  stream.WriteBits(a, 8);
  stream.WriteBits(a, 8);

  // Color indices are 1 for each pixel...
  // Anchor index is 0, so 1 bit for the first pixel, then
  // 01 for each following pixel giving the sequence of 31 bits:
  // ...010101011
  stream.WriteBits(0xaaaaaaab, 31);

  // Alpha indices...
  stream.WriteBits(kWMValues[gWMVal = (gWMVal+1) % kNumWMVals], 31);
}

void GetBlock(const uint32 x, const uint32 y, const uint32 pixelsWide,
              const uint32 *inPixels, uint32 block[16]) {
  memcpy(block, inPixels + y*pixelsWide + x, 4 * sizeof(uint32));
  memcpy(block + 4, inPixels + (y+1)*pixelsWide + x, 4 * sizeof(uint32));
  memcpy(block + 8, inPixels + (y+2)*pixelsWide + x, 4 * sizeof(uint32));
  memcpy(block + 12, inPixels + (y+3)*pixelsWide + x, 4 * sizeof(uint32));
}

// Compress an image using BC7 compression. Use the inBuf parameter to point
// to an image in 4-byte RGBA format. The width and height parameters specify
// the size of the image in pixels. The buffer pointed to by outBuf should be
// large enough to store the compressed image. This implementation has an 4:1
// compression ratio.
void Compress(const FasTC::CompressionJob &cj, CompressionSettings settings) {
  const uint32 *inPixels = reinterpret_cast<const uint32 *>(cj.InBuf());
  const uint32 kBlockSz = GetBlockSize(FasTC::eCompressionFormat_BPTC);
  uint8 *outBuf = cj.OutBuf() + cj.CoordsToBlockIdx(cj.XStart(), cj.YStart()) * kBlockSz;

  const uint32 endY = std::min(cj.YEnd(), cj.Height() - 4);
  uint32 startX = cj.XStart();
  for(uint32 j = cj.YStart(); j <= endY; j += 4) {
    const uint32 endX = j == cj.YEnd()? cj.XEnd() : cj.Width();
    for(uint32 i = startX; i < endX; i += 4) {

      uint32 block[16];
      GetBlock(i, j, cj.Width(), inPixels, block);
      CompressBC7Block(i, j, block, outBuf, settings);

#ifndef NDEBUG
      const uint8 *inBlock = reinterpret_cast<const uint8 *>(block);
      const uint8 *cmpblock = reinterpret_cast<const uint8 *>(outBuf);
      uint32 unComp[16];
      uint8* unCompData = reinterpret_cast<uint8 *>(unComp);

      FasTC::DecompressionJob dj(FasTC::eCompressionFormat_BPTC,
                                 cmpblock, unCompData, 4, 4);
      Decompress(dj);

      double diffSum = 0.0;
      for(int k = 0; k < 64; k+=4) {
        double rdiff = sad(unCompData[k], inBlock[k]);
        double gdiff = sad(unCompData[k+1], inBlock[k+1]);
        double bdiff = sad(unCompData[k+2], inBlock[k+2]);
        double adiff = sad(unCompData[k+3], inBlock[k+3]);
        const double asrc = static_cast<double>(inBlock[k+3]);
        const double adst = static_cast<double>(unCompData[k+3]);
        double avga = ((asrc + adst)*0.5)/255.0;
        diffSum += (rdiff + gdiff + bdiff + adiff) * avga;
      }
      double blockError = static_cast<double>(diffSum) / 64.0;
      if(blockError > 5.0) {
        fprintf(stderr, "WARNING: Block error very high"
                        " at <%d, %d>: (%.2f)\n", i, j, blockError);
      }
#endif

      outBuf += kBlockSz;
    }
    startX = 0;
  }
}

#ifdef HAS_ATOMICS
#ifdef HAS_MSVC_ATOMICS
static uint32 TestAndSet(uint32 *x) {
  return InterlockedExchange(x, 1);
}

static uint32 FetchAndAdd(uint32 *x) {
  return InterlockedIncrement(x)-1;
}
#elif defined HAS_GCC_ATOMICS
static uint32 TestAndSet(uint32 *x) {
  return __sync_lock_test_and_set(x, 1);
}

static uint32 FetchAndAdd(uint32 *x) {
  return __sync_fetch_and_add(x, 1);
}
#endif

// Variables used for synchronization in threadsafe implementation.
void CompressAtomic(FasTC::CompressionJobList &cjl) {
  uint32 jobIdx;
  while((jobIdx = cjl.m_CurrentJobIndex) < cjl.GetNumJobs()) {
    // !HACK! ... Microsoft has this defined
    #undef GetJob

    const FasTC::CompressionJob *cj = cjl.GetJob(jobIdx);
    const uint32 nBlocks = (cj->Height() * cj->Width()) / 16;

    // Help finish whatever texture we're compressing before we start again on
    // my work...
    uint32 blockIdx;
    while((blockIdx = FetchAndAdd(&(cjl.m_CurrentBlockIndex))) < nBlocks &&
          *(cjl.GetFinishedFlag(jobIdx)) == 0) {
      unsigned char *out = cj->OutBuf() + (16 * blockIdx);

      uint32 block[16];
      uint32 x = cj->XStart() + 4 * (blockIdx % (cj->Width() / 4));
      uint32 y = cj->YStart() + 4 * (blockIdx / (cj->Width() / 4));
      const uint32 *inPixels = reinterpret_cast<const uint32 *>(cj->InBuf());
      GetBlock(x, y, cj->Width(), inPixels, block);
      CompressBC7Block(x, y, block, out);
    }

    if(TestAndSet(cjl.GetFinishedFlag(jobIdx)) == 0) {
      cjl.m_CurrentBlockIndex = 0;
      cjl.m_CurrentJobIndex++;
    }

    // Wait until this texture finishes.
    while(cjl.m_CurrentJobIndex == jobIdx) { }
  }
}
#endif  // HAS_ATOMICS

  void CompressWithStats(const FasTC::CompressionJob &cj, std::ostream *logStream,
                         CompressionSettings settings) {
  const uint32 *inPixels = reinterpret_cast<const uint32 *>(cj.InBuf());
  const uint32 kBlockSz = GetBlockSize(FasTC::eCompressionFormat_BPTC);
  uint8 *outBuf = cj.OutBuf() + cj.CoordsToBlockIdx(cj.XStart(), cj.YStart()) * kBlockSz;

  uint32 startX = cj.XStart();
  for(uint32 j = cj.YStart(); j <= cj.YEnd(); j += 4) {
    const uint32 endX = j == cj.YEnd()? cj.XEnd() : cj.Width();
    for(uint32 i = startX; i < endX; i += 4) {

      uint32 block[16];
      GetBlock(i, j, cj.Width(), inPixels, block);

      if(logStream) {
        uint64 blockIdx = cj.CoordsToBlockIdx(i, j);
        CompressBC7Block(i, j, block, outBuf, BlockLogger(blockIdx, *logStream), settings);
      } else {
        CompressBC7Block(i, j, block, outBuf, settings);
      }

#ifndef NDEBUG
      const uint8 *inBlock = reinterpret_cast<const uint8 *>(block);
      const uint8 *cmpData = outBuf;
      uint32 unComp[16];
      uint8* unCompData = reinterpret_cast<uint8 *>(unComp);

      FasTC::DecompressionJob dj(FasTC::eCompressionFormat_BPTC,
                                 cmpData, unCompData, 4, 4);
      Decompress(dj);

      uint32 diffSum = 0;
      for(uint32 k = 0; k < 64; k++) {
        diffSum += sad(unCompData[k], inBlock[k]);
      }
      double blockError = static_cast<double>(diffSum) / 64.0;
      if(blockError > 50.0) {
        fprintf(stderr, "WARNING: Block error very high"
                        " (%.2f)\n", blockError);
      }
#endif

      outBuf += 16;
    }

    startX = 0;
  }
}

static double EstimateTwoClusterError(ErrorMetric metric, RGBACluster &c) {
  RGBAVector Min, Max, v;
  c.GetBoundingBox(Min, Max);
  v = Max - Min;
  if(v * v == 0) {
    return 0.0;
  }

  const float *w = kErrorMetrics[metric];

  double error = 0.0001;
  error += c.QuantizedError(Min, Max, 8,
                            0xFFFFFFFF, RGBAVector(w[0], w[1], w[2], w[3]));
  return error;
}

static double EstimateThreeClusterError(ErrorMetric metric, RGBACluster &c) {
  RGBAVector Min, Max, v;
  c.GetBoundingBox(Min, Max);
  v = Max - Min;
  if(v * v == 0) {
    return 0.0;
  }

  const float *w = kErrorMetrics[metric];
  double error = 0.0001;
  error += c.QuantizedError(Min, Max, 4,
                            0xFFFFFFFF, RGBAVector(w[0], w[1], w[2], w[3]));
  return error;
}

static uint32 kTwoPartitionModes = 
  static_cast<uint32>(eBlockMode_One) |
  static_cast<uint32>(eBlockMode_Three) |
  static_cast<uint32>(eBlockMode_Seven);
static uint32 kThreePartitionModes =
  static_cast<uint32>(eBlockMode_Zero) |
  static_cast<uint32>(eBlockMode_Two);
static uint32 kAlphaModes =
  static_cast<uint32>(eBlockMode_Four) |
  static_cast<uint32>(eBlockMode_Five) |
  static_cast<uint32>(eBlockMode_Six)  |
  static_cast<uint32>(eBlockMode_Seven);

static ShapeSelection BoxSelection(
  uint32, uint32, const uint32 pixels[16], const void *userData
) {

  ErrorMetric metric = *(reinterpret_cast<const ErrorMetric *>(userData));
  ShapeSelection result;

  bool opaque = true;
  for(uint32 i = 0; i < 16; i++) {
    uint32 a = (pixels[i] >> 24) & 0xFF;
    opaque = opaque && (a >= 250); // For all intents and purposes...
  }

  // First we must figure out which shape to use. To do this, simply
  // see which shape has the smallest sum of minimum bounding spheres.
  double bestError[2] = { std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max() };

  RGBACluster cluster(pixels);

  result.m_NumShapesToSearch = 1;
  for(unsigned int i = 0; i < kNumShapes2; i++) {
    cluster.SetShapeIndex(i, 2);

    double err = 0.0;
    for(int ci = 0; ci < 2; ci++) {
      cluster.SetPartition(ci);
      err += EstimateTwoClusterError(metric, cluster);
    }

    if(err < bestError[0]) {
      bestError[0] = err;
      result.m_Shapes[0].m_Index = i;
      result.m_Shapes[0].m_NumPartitions = 2;
    }

    // If it's small, we'll take it!
    if(err < 1e-9) {
      result.m_SelectedModes = kTwoPartitionModes;
      return result;
    }
  }

  // There are not 3 subset blocks that support alpha, so only check these
  // if the entire block is opaque.
  if(!opaque) {
    result.m_SelectedModes &= kAlphaModes;
    return result;
  }

  // If it's opaque, we get more value out of mode 6 than modes
  // 4 and 5, so just ignore those.
  result.m_SelectedModes &=
    ~(static_cast<uint32>(eBlockMode_Four) |
      static_cast<uint32>(eBlockMode_Five));

  result.m_NumShapesToSearch++;
  for(unsigned int i = 0; i < kNumShapes3; i++) {
    cluster.SetShapeIndex(i, 3);

    double err = 0.0;
    for(int ci = 0; ci < 3; ci++) {
      cluster.SetPartition(ci);
      err += EstimateThreeClusterError(metric, cluster);
    }

    if(err < bestError[1]) {
      bestError[1] = err;
      result.m_Shapes[1].m_Index = i;
      result.m_Shapes[1].m_NumPartitions = 3;
    }

    // If it's small, we'll take it!
    if(err < 1e-9) {
      result.m_SelectedModes = kThreePartitionModes;
      return result;
    }
  }

  return result;
}

static void CompressClusters(const ShapeSelection &selection, const uint32 pixels[16],
                             const CompressionSettings &settings, uint8 *outBuf,
                             double *errors, int *modeChosen) {
  RGBACluster cluster(pixels);
  double bestError = std::numeric_limits<double>::max();
  uint32 modes[8] = {0, 2, 1, 3, 7, 4, 5, 6};
  uint32 bestMode = 8;
  CompressionMode::Params bestParams;

  uint32 selectedModes = selection.m_SelectedModes;
  uint32 numShapeIndices = std::min<uint32>(5, selection.m_NumShapesToSearch);

  // If we don't have any indices, turn off two and three partition modes,
  // since the compressor will simply ignore the shapeIndex variable afterwards...
  if(numShapeIndices == 0) {
    numShapeIndices = 1;
    selectedModes &= ~(kTwoPartitionModes | kThreePartitionModes);
  }

  for(uint32 modeIdx = 0; modeIdx < 8; modeIdx++) {

    uint32 mode = modes[modeIdx];
    if((selectedModes & (1 << mode)) == 0) {
      continue;
    }

    for(uint32 shapeIdx = 0; shapeIdx < numShapeIndices; shapeIdx++) {
      const Shape &shape = selection.m_Shapes[shapeIdx];

      // If the shape doesn't support the number of subsets then skip it.
      uint32 nParts = CompressionMode::GetAttributesForMode(mode)->numSubsets;
      if(nParts != 1 && nParts != shape.m_NumPartitions) {
        continue;
      }

      // Block mode zero only has four bits for the partition index,
      // so if the chosen three-partition shape is not within this range,
      // then we shouldn't consider using this block mode...
      if(shape.m_Index >= 16 && mode == 0) {
        continue;
      }

      uint32 idx = shape.m_Index;
      cluster.SetShapeIndex(idx, nParts);

      CompressionMode::Params params;
      double error = CompressionMode(mode, settings).Compress(params, idx, cluster);

      if(errors)
        errors[mode] = std::min(error, errors[mode]);

      if(error < bestError) {
        bestError = error;
        bestMode = mode;
        bestParams = params;
      }
    }
  }

  assert(bestMode < 8);

  BitStream stream(outBuf, 128, 0);
  CompressionMode(bestMode, settings).Pack(bestParams, stream);
  if(modeChosen)
    *modeChosen = bestMode;
}

static void CompressBC7Block(const uint32 x, const uint32 y,
                             const uint32 block[16], uint8 *outBuf,
                             const CompressionSettings settings) {
  // All a single color?
  if(AllOneColor(block)) {
    BitStream bStrm(outBuf, 128, 0);
    CompressOptimalColorBC7(*block, bStrm);
    return;
  }

  RGBACluster blockCluster(block);
  bool transparent = true;

  for(uint32 i = 0; i < blockCluster.GetNumPoints(); i++) {
    const RGBAVector &p = blockCluster.GetPoint(i);
    if(p.A() > 0.0f) {
      transparent = false;
      break;
    }
  }

  // The whole block is transparent?
  if(transparent) {
    BitStream bStrm(outBuf, 128, 0);
    WriteTransparentBlock(bStrm);
    return;
  }

  ShapeSelectionFn selectionFn = BoxSelection;
  const void *userData = &settings.m_ErrorMetric;
  if(settings.m_ShapeSelectionFn != NULL) {
    selectionFn = settings.m_ShapeSelectionFn;
    userData = settings.m_ShapeSelectionUserData;
  }
  assert(selectionFn);

  ShapeSelection selection =
    selectionFn(x, y, block, userData);
  selection.m_SelectedModes &= settings.m_BlockModes;
  assert(selection.m_SelectedModes);
  CompressClusters(selection, block, settings, outBuf, NULL, NULL);
}

static double EstimateTwoClusterErrorStats(
  ErrorMetric metric, RGBACluster &c, double (&estimates)[2]
) {
  RGBAVector Min, Max, v;
  c.GetBoundingBox(Min, Max);
  v = Max - Min;
  if(v * v == 0) {
    estimates[0] = estimates[1] = 0.0;
    return 0.0;
  }

  const float *w = kErrorMetrics[metric];

  const double err1 = c.QuantizedError(
    Min, Max, 8, 0xFFFCFCFC, RGBAVector(w[0], w[1], w[2], w[3])
  );

  if(err1 >= 0.0) {
    estimates[0] = err1;
  } else {
    estimates[0] = std::min(estimates[0], err1);
  }

  const double err3 = c.QuantizedError(
    Min, Max, 8, 0xFFFEFEFE, RGBAVector(w[0], w[1], w[2], w[3])
  );

  if(err3 >= 0.0) {
    estimates[1] = err3;
  } else {
    estimates[1] = std::min(estimates[1], err3);
  }

  double error = 0.0001;
  error += std::min(err1, err3);
  return error;
}

static double EstimateThreeClusterErrorStats(
  ErrorMetric metric, RGBACluster &c, double (&estimates)[2]
) {
  RGBAVector Min, Max, v;
  c.GetBoundingBox(Min, Max);
  v = Max - Min;
  if(v * v == 0) {
    estimates[0] = estimates[1] = 0.0;
    return 0.0;
  }

  const float *w = kErrorMetrics[metric];
  const double err0 = 0.0001 + c.QuantizedError(
    Min, Max, 4, 0xFFF0F0F0, RGBAVector(w[0], w[1], w[2], w[3])
  );

  if(err0 >= 0.0) {
    estimates[0] = err0;
  } else {
    estimates[0] = std::min(estimates[0], err0);
  }

  const double err2 = 0.0001 + c.QuantizedError(
    Min, Max, 4, 0xFFF8F8F8, RGBAVector(w[0], w[1], w[2], w[3])
  );

  if(err2 >= 0.0) {
    estimates[1] = err2;
  } else {
    estimates[1] = std::min(estimates[1], err2);
  }

  double error = 0.0001;
  error += std::min(err0, err2);
  return error;
}

static void UpdateErrorEstimate(double *estimates, uint32 mode, double est) {
  assert(estimates);
  assert(mode >= 0);
  assert(mode < CompressionMode::kNumModes);
  if(estimates[mode] == -1.0 || est < estimates[mode]) {
    estimates[mode] = est;
  }
}

template<typename T>
static void PrintStat(const BlockLogger &lgr, const char *stat, const T &v) {
  std::stringstream ss;
  ss << stat << " -- " << v << std::endl;
  lgr << ss.str();
}

// Compress a single block but collect statistics as well...
static void CompressBC7Block(
  const uint32 x, const uint32 y,
  const uint32 block[16], uint8 *outBuf, const BlockLogger &logStream,
  const CompressionSettings settings
) {

  class RAIIStatSaver {
  private:
    const BlockLogger &m_Logger;

    int *m_ModePtr;
    double *m_Estimates;
    double *m_Errors;

  public:
    RAIIStatSaver(const BlockLogger &logger)
      : m_Logger(logger)
      , m_ModePtr(NULL), m_Estimates(NULL), m_Errors(NULL) { }
    void SetMode(int *modePtr) { m_ModePtr = modePtr; }
    void SetEstimates(double *estimates) { m_Estimates = estimates; }
    void SetErrors(double *errors) { m_Errors = errors; }

    ~RAIIStatSaver() {

      assert(m_ModePtr);
      assert(m_Estimates);
      assert(m_Errors);

      PrintStat(m_Logger, kBlockStatString[eBlockStat_Mode], *m_ModePtr);

      for(uint32 i = 0; i < CompressionMode::kNumModes; i++) {

        PrintStat(m_Logger,
                  kBlockStatString[eBlockStat_ModeZeroEstimate + i],
                  m_Estimates[i]);
        PrintStat(m_Logger,
                  kBlockStatString[eBlockStat_ModeZeroError + i],
                  m_Errors[i]);
      }
    }
  };

  int bestMode = 0;
  double modeEstimate[CompressionMode::kNumModes];
  double modeError[CompressionMode::kNumModes];

  // reset global variables...
  bestMode = 0;
  for(uint32 i = 0; i < CompressionMode::kNumModes; i++) {
    modeError[i] = modeEstimate[i] = -1.0;
  }

  RAIIStatSaver __statsaver__(logStream);
  __statsaver__.SetMode(&bestMode);
  __statsaver__.SetEstimates(modeEstimate);
  __statsaver__.SetErrors(modeError);

  // All a single color?
  if(AllOneColor(block)) {
    BitStream bStrm(outBuf, 128, 0);
    CompressOptimalColorBC7(*block, bStrm);
    bestMode = 5;

    PrintStat(logStream, kBlockStatString[eBlockStat_Path], 0);
    return;
  }

  RGBACluster blockCluster(block);
  bool opaque = true;
  bool transparent = true;

  for(uint32 i = 0; i < blockCluster.GetNumPoints(); i++) {
    const RGBAVector &p = blockCluster.GetPoint(i);
    if(fabs(p.A() - 255.0f) > 1e-10) {
      opaque = false;
    }

    if(p.A() > 0.0f) {
      transparent = false;
    }
  }

  // The whole block is transparent?
  if(transparent) {
    BitStream bStrm(outBuf, 128, 0);
    WriteTransparentBlock(bStrm);
    bestMode = 6;

    PrintStat(logStream, kBlockStatString[eBlockStat_Path], 1);
    return;
  }

  // First, estimate the error it would take to compress a single line with
  // mode 6...
  {
    RGBAVector Min, Max, v;
    blockCluster.GetBoundingBox(Min, Max);
    v = Max - Min;
    if(v * v == 0) {
      modeEstimate[6] = 0.0;
    } else {
      const float *w = kErrorMetrics[settings.m_ErrorMetric];
      const double err = 0.0001 + blockCluster.QuantizedError(
        Min, Max, 4, 0xFEFEFEFE, RGBAVector(w[0], w[1], w[2], w[3])
      );
      UpdateErrorEstimate(modeEstimate, 6, err);
    }
  }

  // First we must figure out which shape to use. To do this, simply
  // see which shape has the smallest sum of minimum bounding spheres.
  double bestError[2] = { DBL_MAX, DBL_MAX };

  ShapeSelection selection;
  uint32 path = 0;

  selection.m_NumShapesToSearch = 1;
  for(unsigned int i = 0; i < kNumShapes2; i++) {
    blockCluster.SetShapeIndex(i, 2);

    double err = 0.0;
    double errEstimate[2] = { -1.0, -1.0 };
    for(int ci = 0; ci < 2; ci++) {
      blockCluster.SetPartition(ci);
      double shapeEstimate[2] = { -1.0, -1.0 };
      err += EstimateTwoClusterErrorStats(settings.m_ErrorMetric, 
                                          blockCluster, shapeEstimate);

      for(int ei = 0; ei < 2; ei++) {
        if(shapeEstimate[ei] >= 0.0) {
          if(errEstimate[ei] == -1.0) {
            errEstimate[ei] = shapeEstimate[ei];
          } else {
            errEstimate[ei] += shapeEstimate[ei];
          }
        }
      }
    }

    if(errEstimate[0] != -1.0) {
      UpdateErrorEstimate(modeEstimate, 1, errEstimate[0]);
    }

    if(errEstimate[1] != -1.0) {
      UpdateErrorEstimate(modeEstimate, 3, errEstimate[1]);
    }

    if(err < bestError[0]) {
      PrintStat(logStream, 
        kBlockStatString[eBlockStat_TwoShapeEstimate], err
      );
    }

    if(err < bestError[0]) {
      bestError[0] = err;
      selection.m_Shapes[0].m_Index = i;
      selection.m_Shapes[0].m_NumPartitions = 2;
    }

    // If it's small, we'll take it!
    if(err < 1e-9) {
      path = 2;
      selection.m_SelectedModes = kTwoPartitionModes;
      break;
    }
  }

  // There are not 3 subset blocks that support alpha, so only check these
  // if the entire block is opaque.
  if(opaque) {
    selection.m_NumShapesToSearch++;
    for(unsigned int i = 0; i < kNumShapes3; i++) {
      blockCluster.SetShapeIndex(i, 3);

      double err = 0.0;
      double errEstimate[2] = { -1.0, -1.0 };
      for(int ci = 0; ci < 3; ci++) {
        blockCluster.SetPartition(ci);
        double shapeEstimate[2] = { -1.0, -1.0 };
        err += EstimateThreeClusterErrorStats(settings.m_ErrorMetric, 
                                              blockCluster, shapeEstimate);

        for(int ei = 0; ei < 2; ei++) {
          if(shapeEstimate[ei] >= 0.0) {
            if(errEstimate[ei] == -1.0) {
              errEstimate[ei] = shapeEstimate[ei];
            } else {
              errEstimate[ei] += shapeEstimate[ei];
            }
          }
        }
      }

      if(errEstimate[0] != -1.0) {
        UpdateErrorEstimate(modeEstimate, 0, errEstimate[0]);
      }

      if(errEstimate[1] != -1.0) {
        UpdateErrorEstimate(modeEstimate, 2, errEstimate[1]);
      }

      if(err < bestError[1]) {
        PrintStat(logStream, 
          kBlockStatString[eBlockStat_ThreeShapeEstimate], err
        );
      }

      if(err < bestError[1]) {
        bestError[1] = err;
        selection.m_Shapes[1].m_Index = i;
        selection.m_Shapes[1].m_NumPartitions = 3;
      }

      // If it's small, we'll take it!
      if(err < 1e-9) {
        path = 2;
        selection.m_SelectedModes = kThreePartitionModes;
        break;
      }
    }
  }

  if(path == 0) path = 3;

  selection.m_SelectedModes &= settings.m_BlockModes;
  assert(selection.m_SelectedModes);
  CompressClusters(selection, block, settings, outBuf, modeError, &bestMode);

  PrintStat(logStream, kBlockStatString[eBlockStat_Path], path);
}

}  // namespace BPTCC
