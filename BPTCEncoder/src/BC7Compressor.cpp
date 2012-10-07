//--------------------------------------------------------------------------------------
// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//
//--------------------------------------------------------------------------------------

#include "TexCompTypes.h"
#include "BC7Compressor.h"
#include "BC7CompressionMode.h"
#include "BCLookupTables.h"
#include "RGBAEndpoints.h"
#include "BitStream.h"

#include "BlockStats.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cfloat>
#include <ctime>

#ifdef _MSC_VER
#define ALIGN_SSE __declspec( align(16) )
#else
#define ALIGN_SSE __attribute__((aligned(16)))
#endif

enum EBlockStats {
  eBlockStat_Path,
  eBlockStat_Mode,

  kNumBlockStats
};

static const char *kBlockStatString[kNumBlockStats] = {
  "BlockStat_Path",
  "BlockStat_Mode"
};

static const uint32 kNumShapes2 = 64;
static const uint16 kShapeMask2[kNumShapes2] = {
  0xcccc, 0x8888, 0xeeee, 0xecc8, 0xc880, 0xfeec, 0xfec8, 0xec80,
  0xc800, 0xffec, 0xfe80, 0xe800, 0xffe8, 0xff00, 0xfff0, 0xf000,
  0xf710, 0x008e, 0x7100, 0x08ce, 0x008c, 0x7310, 0x3100, 0x8cce,
  0x088c, 0x3110, 0x6666, 0x366c, 0x17e8, 0x0ff0, 0x718e, 0x399c,
  0xaaaa, 0xf0f0, 0x5a5a, 0x33cc, 0x3c3c, 0x55aa, 0x9696, 0xa55a,
  0x73ce, 0x13c8, 0x324c, 0x3bdc, 0x6996, 0xc33c, 0x9966, 0x0660,
  0x0272, 0x04e4, 0x4e40, 0x2720, 0xc936, 0x936c, 0x39c6, 0x639c,
  0x9336, 0x9cc6, 0x817e, 0xe718, 0xccf0, 0x0fcc, 0x7744, 0xee22
};

static const int kAnchorIdx2[kNumShapes2] = {
  15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,
    15, 2, 8, 2, 2, 8, 8,15,
     2, 8, 2, 2, 8, 8, 2, 2,
    15,15, 6, 8, 2, 8,15,15,
     2, 8, 2, 2, 2,15,15, 6,
     6, 2, 6, 8,15,15, 2, 2,
    15,15,15,15,15, 2, 2, 15
};

static const uint32 kNumShapes3 = 64;
static const uint16 kShapeMask3[kNumShapes3][2] = {
  { 0xfecc, 0xf600 }, { 0xffc8, 0x7300 }, { 0xff90, 0x3310 }, { 0xecce, 0x00ce }, { 0xff00, 0xcc00 }, { 0xcccc, 0xcc00 }, { 0xffcc, 0x00cc }, { 0xffcc, 0x3300 },
  { 0xff00, 0xf000 }, { 0xfff0, 0xf000 }, { 0xfff0, 0xff00 }, { 0xcccc, 0x8888 }, { 0xeeee, 0x8888 }, { 0xeeee, 0xcccc }, { 0xffec, 0xec80 }, { 0x739c, 0x7310 },
  { 0xfec8, 0xc800 }, { 0x39ce, 0x3100 }, { 0xfff0, 0xccc0 }, { 0xfccc, 0x0ccc }, { 0xeeee, 0xee00 }, { 0xff88, 0x7700 }, { 0xeec0, 0xcc00 }, { 0x7730, 0x3300 },
  { 0x0cee, 0x00cc }, { 0xffcc, 0xfc88 }, { 0x6ff6, 0x0660 }, { 0xff60, 0x6600 }, { 0xcbbc, 0xc88c }, { 0xf966, 0xf900 }, { 0xceec, 0x0cc0 }, { 0xff10, 0x7310 },
  { 0xff80, 0xec80 }, { 0xccce, 0x08ce }, { 0xeccc, 0xec80 }, { 0x6666, 0x4444 }, { 0x0ff0, 0x0f00 }, { 0x6db6, 0x4924 }, { 0x6bd6, 0x4294 }, { 0xcf3c, 0x0c30 },
  { 0xc3fc, 0x03c0 }, { 0xffaa, 0xff00 }, { 0xff00, 0x5500 }, { 0xfcfc, 0xcccc }, { 0xcccc, 0x0c0c }, { 0xf6f6, 0x6666 }, { 0xaffa, 0x0ff0 }, { 0xfff0, 0x5550 },
  { 0xfaaa, 0xf000 }, { 0xeeee, 0x0e0e }, { 0xf8f8, 0x8888 }, { 0xfff0, 0x9990 }, { 0xeeee, 0xe00e }, { 0x8ff8, 0x8888 }, { 0xf666, 0xf000 }, { 0xff00, 0x9900 },
  { 0xff66, 0xff00 }, { 0xcccc, 0xc00c }, { 0xcffc, 0xcccc }, { 0xf000, 0x9000 }, { 0x8888, 0x0808 }, { 0xfefe, 0xeeee }, { 0xfffa, 0xfff0 }, { 0x7bde, 0x7310 }
};

static const uint32 kWMValues[] = { 0x32b92180, 0x32ba3080, 0x31103200, 0x28103c80, 0x32bb3080, 0x25903600, 0x3530b900, 0x3b32b180, 0x34b5b980 };
static const uint32 kNumWMVals = sizeof(kWMValues) / sizeof(kWMValues[0]);
static uint32 gWMVal = -1;

static const int kAnchorIdx3[2][kNumShapes3] = {
  { 3, 3,15,15, 8, 3,15,15,
     8, 8, 6, 6, 6, 5, 3, 3,
     3, 3, 8,15, 3, 3, 6,10,
     5, 8, 8, 6, 8, 5,15,15,
     8,15, 3, 5, 6,10, 8,15,
    15, 3,15, 5,15,15,15,15,
     3,15, 5, 5, 5, 8, 5,10,
   5,10, 8,13,15,12, 3, 3 },

  { 15, 8, 8, 3,15,15, 3, 8,
    15,15,15,15,15,15,15, 8,
    15, 8,15, 3,15, 8,15, 8,
     3,15, 6,10,15,15,10, 8,
    15, 3,15,10,10, 8, 9,10,
     6,15, 8,15, 3, 6, 6, 8,
    15, 3,15,15,15,15,15,15,
  15,15,15,15, 3,15,15, 8 }
};

template <typename T>
static inline T sad(const T &a, const T &b) {
  return (a > b)? a - b : b - a;
}

static int GetSubsetForIndex(int idx, const int shapeIdx, const int nSubsets) {
  int subset = 0;
  
  switch(nSubsets) {
    case 2:
    {
      subset = !!((1 << idx) & kShapeMask2[shapeIdx]);
    }
    break;

    case 3:
    {
      if(1 << idx & kShapeMask3[shapeIdx][0])
        subset = 1 + !!((1 << idx) & kShapeMask3[shapeIdx][1]);
      else
        subset = 0;
    }
    break;

    default:
    break;
  }

  return subset;
}

static int GetAnchorIndexForSubset(int subset, const int shapeIdx, const int nSubsets) {
  
  int anchorIdx = 0;
  switch(subset) {
    case 1:
    {
      if(nSubsets == 2) {
        anchorIdx = kAnchorIdx2[shapeIdx];
      }
      else {
        anchorIdx = kAnchorIdx3[0][shapeIdx];
      }
    }
    break;

    case 2:
    {
      assert(nSubsets == 3);
      anchorIdx = kAnchorIdx3[1][shapeIdx];
    }
    break;

    default:
    break;
  }

  return anchorIdx;
}

static int GetPointMaskForSubset(int subset, const int shapeIdx, const int nSubsets) {
  int mask = 0xFFFF;

  assert(subset < nSubsets);

  switch(nSubsets) {
    case 2:
    {
      mask = (subset)? kShapeMask2[shapeIdx] : ~(kShapeMask2[shapeIdx]);
    }
    break;

    case 3:
    {
      switch(subset) {
        default:
        case 0:
        {
          mask = ~(kShapeMask3[shapeIdx][0]);
        }
        break;

        case 1:
        {
          mask = ~(~(kShapeMask3[shapeIdx][0]) | kShapeMask3[shapeIdx][1]);
        }
        break;

        case 2:
        {
          mask = kShapeMask3[shapeIdx][1];
        }
        break;
      }
    }
    break;

    default:
    break;
  }

  return mask;
}

#ifndef min
#define min(a, b) (((a) > (b))? (b) : (a))
#endif

#ifndef max
#define max(a, b) (((a) > (b))? (a) : (b))
#endif

template <typename T>
static void insert(T* buf, int bufSz, T newVal, int idx = 0) {
  int safeIdx = min(bufSz-1, max(idx, 0));
  for(int i = bufSz - 1; i > safeIdx; i--) {
    buf[i] = buf[i-1];
  }
  buf[safeIdx] = newVal;
}

template <typename T>
static inline void swap(T &a, T &b) { T t = a; a = b; b = t; }

const uint32 kBC7InterpolationValues[4][16][2] = {
  { {64, 0}, {33, 31}, {0, 64}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { {64, 0}, {43, 21}, {21, 43}, {0, 64}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { {64, 0}, {55, 9}, {46, 18}, {37, 27}, {27, 37}, {18, 46}, {9, 55}, {0, 64}, 0, 0, 0, 0, 0, 0, 0, 0 },
  { {64, 0}, {60, 4}, {55, 9}, {51, 13}, {47, 17}, {43, 21}, {38, 26}, {34, 30}, {30, 34}, {26, 38}, {21, 43}, {17, 47}, {13, 51}, {9, 55}, {4, 60}, {0, 64} }
};

int BC7CompressionMode::MaxAnnealingIterations = 50; // This is a setting.
int BC7CompressionMode::NumUses[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

BC7CompressionMode::Attributes BC7CompressionMode::kModeAttributes[kNumModes] = {
  { 0, 4, 3, 3, 0, 4, 0, false, false, BC7CompressionMode::ePBitType_NotShared },
  { 1, 6, 2, 3, 0, 6, 0, false, false, BC7CompressionMode::ePBitType_Shared },
  { 2, 6, 3, 2, 0, 5, 0, false, false, BC7CompressionMode::ePBitType_None },
  { 3, 6, 2, 2, 0, 7, 0, false, false, BC7CompressionMode::ePBitType_NotShared },
  { 4, 0, 1, 2, 3, 5, 6, true,  true,  BC7CompressionMode::ePBitType_None },
  { 5, 0, 1, 2, 2, 7, 8, true,  false, BC7CompressionMode::ePBitType_None },
  { 6, 0, 1, 4, 0, 7, 7, false, false, BC7CompressionMode::ePBitType_NotShared },
  { 7, 6, 2, 2, 0, 5, 5, false, false, BC7CompressionMode::ePBitType_NotShared },
};

void BC7CompressionMode::ClampEndpointsToGrid(RGBAVector &p1, RGBAVector &p2, int &bestPBitCombo) const {
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
    }
    else {
      qp1 = p1.ToPixel(qmask);
      qp2 = p2.ToPixel(qmask);
    }

    uint8 *pqp1 = (uint8 *)&qp1;
    uint8 *pqp2 = (uint8 *)&qp2;

    RGBAVector np1 = RGBAVector(float(pqp1[0]), float(pqp1[1]), float(pqp1[2]), float(pqp1[3]));
    RGBAVector np2 = RGBAVector(float(pqp2[0]), float(pqp2[1]), float(pqp2[2]), float(pqp2[3]));

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

double BC7CompressionMode::CompressSingleColor(const RGBAVector &p, RGBAVector &p1, RGBAVector &p2, int &bestPbitCombo) const {

  const uint32 pixel = p.ToPixel();

  uint32 bestDist = 0xFF;
  bestPbitCombo = -1;

  for(int pbi = 0; pbi < GetNumPbitCombos(); pbi++) {

    const int *pbitCombo = GetPBitCombo(pbi);
    
    uint32 dist = 0x0;
    uint32 bestValI[kNumColorChannels] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
    uint32 bestValJ[kNumColorChannels] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

    for(int ci = 0; ci < kNumColorChannels; ci++) {

      const uint8 val = (pixel >> (ci * 8)) & 0xFF;
      int nBits = ci == 3? GetAlphaChannelPrecision() : m_Attributes->colorChannelPrecision;

      // If we don't handle this channel, then we don't need to
      // worry about how well we interpolate.
      if(nBits == 0) { bestValI[ci] = bestValJ[ci] = 0xFF; continue; }

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

      const uint32 interpVal0 = kBC7InterpolationValues[GetNumberOfBitsPerIndex() - 1][1][0];
      const uint32 interpVal1 = kBC7InterpolationValues[GetNumberOfBitsPerIndex() - 1][1][1];

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

      dist = max(bestChannelDist, dist);
    }

    if(dist < bestDist) {
      bestDist = dist;
      bestPbitCombo = pbi;

      for(int ci = 0; ci < kNumColorChannels; ci++) {
        p1.c[ci] = float(bestValI[ci]);
        p2.c[ci] = float(bestValJ[ci]);
      }
    }
  }

  return bestDist;
}

// Fast random number generator. See more information at 
// http://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor/
static uint32 g_seed = uint32(time(NULL));
static inline uint32 fastrand() { 
  g_seed = (214013 * g_seed + 2531011); 
  return (g_seed>>16) & RAND_MAX; 
} 

static const int kNumStepDirections = 8;
static const RGBADir kStepDirections[kNumStepDirections] = {

  // For pBit changes, we have 8 possible directions.
  RGBADir(RGBAVector(1.0f, 1.0f, 1.0f, 0.0f)), 
  RGBADir(RGBAVector(-1.0f, 1.0f, 1.0f, 0.0f)),
  RGBADir(RGBAVector(1.0f, -1.0f, 1.0f, 0.0f)), 
  RGBADir(RGBAVector(-1.0f, -1.0f, 1.0f, 0.0f)),
  RGBADir(RGBAVector(1.0f, 1.0f, -1.0f, 0.0f)), 
  RGBADir(RGBAVector(-1.0f, 1.0f, -1.0f, 0.0f)),
  RGBADir(RGBAVector(1.0f, -1.0f, -1.0f, 0.0f)), 
  RGBADir(RGBAVector(-1.0f, -1.0f, -1.0f, 0.0f))
};

static void ChangePointForDirWithoutPbitChange(RGBAVector &v, int dir, const float step[kNumColorChannels]) {
  if(dir % 2) {
    v.x -= step[0];
  }
  else {
    v.x += step[0];
  }

  if(((dir / 2) % 2)) {
    v.y -= step[1];
  }
  else  {
    v.y += step[1];
  }

  if(((dir / 4) % 2)) {
    v.z -= step[2];
  }
  else {
    v.z += step[2];
  }

  if(((dir / 8) % 2)) {
    v.a -= step[3];
  }
  else {
    v.a += step[3];
  }
}

static void ChangePointForDirWithPbitChange(RGBAVector &v, int dir, int oldPbit, const float step[kNumColorChannels]) {
  if(dir % 2 && oldPbit == 0) {
    v.x -= step[0];
  }
  else if(!(dir % 2) && oldPbit == 1) {
    v.x += step[0];
  }

  if(((dir / 2) % 2) && oldPbit == 0) {
    v.y -= step[1];
  }
  else if(!((dir / 2) % 2) && oldPbit == 1) {
    v.y += step[1];
  }

  if(((dir / 4) % 2) && oldPbit == 0) {
    v.z -= step[2];
  }
  else if(!((dir / 4) % 2) && oldPbit == 1) {
    v.z += step[2];
  }

  if(((dir / 8) % 2) && oldPbit == 0) {
    v.a -= step[3];
  }
  else if(!((dir / 8) % 2) && oldPbit == 1) {
    v.a += step[3];
  }
}

void BC7CompressionMode::PickBestNeighboringEndpoints(const RGBACluster &cluster, const RGBAVector &p1, const RGBAVector &p2, const int curPbitCombo, RGBAVector &np1, RGBAVector &np2, int &nPbitCombo, const VisitedState *visitedStates, int nVisited, float stepSz) const {
  
  // !SPEED! There might be a way to make this faster since we're working
  // with floating point values that are powers of two. We should be able
  // to just set the proper bits in the exponent and leave the mantissa to 0.
  float step[kNumColorChannels] = {
    stepSz * float(1 << (8 - m_Attributes->colorChannelPrecision)),
    stepSz * float(1 << (8 - m_Attributes->colorChannelPrecision)),
    stepSz * float(1 << (8 - m_Attributes->colorChannelPrecision)),
    stepSz * float(1 << (8 - GetAlphaChannelPrecision()))
  };

  if(m_IsOpaque) {
    step[(GetRotationMode() + 3) % kNumColorChannels] = 0.0f;
  }

  // First, let's figure out the new pbit combo... if there's no pbit then we don't need
  // to worry about it.
  const bool hasPbits = GetPBitType() != ePBitType_None;
  if(hasPbits) {

    // If there is a pbit, then we must change it, because those will provide the closest values
    // to the current point.
    if(GetPBitType() == ePBitType_Shared)
      nPbitCombo = (curPbitCombo + 1) % 2;
    else {
      // Not shared... p1 needs to change and p2 needs to change... which means that 
      // combo 0 gets rotated to combo 3, combo 1 gets rotated to combo 2 and vice
      // versa...
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
      if(hasPbits) 
        ChangePointForDirWithPbitChange(np, fastrand() % 16, GetPBitCombo(curPbitCombo)[pt], step);
      else
        ChangePointForDirWithoutPbitChange(np, fastrand() % 16, step);

      for(int i = 0; i < kNumColorChannels; i++) {
        np.c[i] = min(max(np.c[i], 0.0f), 255.0f);
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

#define COMPILE_ASSERT(x) extern int __compile_assert_[(int)(x)];
COMPILE_ASSERT(RAND_MAX == 0x7FFF)

static inline float frand() { 
  const uint16 r = fastrand();
  
  // RAND_MAX is 0x7FFF, which offers 15 bits
  // of precision. Therefore, we move the bits
  // into the top of the 23 bit mantissa, and 
  // repeat the most significant bits of r in 
  // the least significant of the mantissa
  const uint32 m = (r << 8) | (r >> 7);
  const uint32 flt = (127 << 23) | m;
  return *(reinterpret_cast<const float *>(&flt)) - 1.0f;
}

bool BC7CompressionMode::AcceptNewEndpointError(double newError, double oldError, float temp) const {

  // Always accept better endpoints.
  if(newError < oldError)
    return true;

  const double p = exp((0.1f * (oldError - newError)) / temp);
  const double r = frand();

  return r < p;
}

double BC7CompressionMode::OptimizeEndpointsForCluster(const RGBACluster &cluster, RGBAVector &p1, RGBAVector &p2, int *bestIndices, int &bestPbitCombo) const {
  
  const int nBuckets = (1 << GetNumberOfBitsPerIndex());
  const int nPbitCombos = GetNumPbitCombos();
  const uint32 qmask = GetQuantizationMask();

  // Here we use simulated annealing to traverse the space of clusters to find the best possible endpoints.
  double curError = cluster.QuantizedError(p1, p2, nBuckets, qmask, GetErrorMetric(), GetPBitCombo(bestPbitCombo), bestIndices);
  int curPbitCombo = bestPbitCombo;
  double bestError = curError;

  // Clamp endpoints to the grid...
  uint32 qp1, qp2;
  if(GetPBitType() != ePBitType_None) {
    qp1 = p1.ToPixel(qmask, GetPBitCombo(bestPbitCombo)[0]);
    qp2 = p2.ToPixel(qmask, GetPBitCombo(bestPbitCombo)[1]);
  }
  else {
    qp1 = p1.ToPixel(qmask);
    qp2 = p2.ToPixel(qmask);
  }

  uint8 *pqp1 = (uint8 *)&qp1;
  uint8 *pqp2 = (uint8 *)&qp2;

  p1 = RGBAVector(float(pqp1[0]), float(pqp1[1]), float(pqp1[2]), float(pqp1[3]));
  p2 = RGBAVector(float(pqp2[0]), float(pqp2[1]), float(pqp2[2]), float(pqp2[3]));

  RGBAVector bp1 = p1, bp2 = p2;

  assert(curError == cluster.QuantizedError(p1, p2, nBuckets, qmask, GetErrorMetric(), GetPBitCombo(bestPbitCombo)));
  
  int lastVisitedState = 0;
  VisitedState visitedStates[kMaxAnnealingIterations];

  visitedStates[lastVisitedState].p1 = p1;
  visitedStates[lastVisitedState].p2 = p2; 
  visitedStates[lastVisitedState].pBitCombo = curPbitCombo;
  lastVisitedState++;

  const int maxEnergy = MaxAnnealingIterations;

  for(int energy = 0; bestError > 0 && energy < maxEnergy; energy++) {

    float temp = float(energy) / float(maxEnergy-1);

    int indices[kMaxNumDataPoints];
    RGBAVector np1, np2;
    int nPbitCombo;

    PickBestNeighboringEndpoints(cluster, p1, p2, curPbitCombo, np1, np2, nPbitCombo, visitedStates, lastVisitedState);

    double error = cluster.QuantizedError(np1, np2, nBuckets, qmask, GetErrorMetric(), GetPBitCombo(nPbitCombo), indices);
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

double BC7CompressionMode::CompressCluster(const RGBACluster &cluster, RGBAVector &p1, RGBAVector &p2, int *bestIndices, int *alphaIndices) const {
    
  assert(GetModeNumber() == 4 || GetModeNumber() == 5);
  assert(GetNumberOfSubsets() == 1);
  assert(cluster.GetNumPoints() == kMaxNumDataPoints);
  assert(m_Attributes->alphaChannelPrecision > 0);

  // If all the points are the same in the cluster, then we need to figure out what the best
  // approximation to this point is....
  if(cluster.AllSamePoint()) {

    assert(!"We should only be using this function in modes 4 & 5 that have a single subset, in which case single colors should have been detected much earlier.");

    const RGBAVector &p = cluster.GetPoint(0);
    int dummyPbit = 0;
    double bestErr = CompressSingleColor(p, p1, p2, dummyPbit);

    // We're assuming all indices will be index 1...
    for(int i = 0; i < cluster.GetNumPoints(); i++) {
      bestIndices[i] = 1;
      alphaIndices[i] = 1;
    }
    
    return bestErr;
  }

  RGBACluster rgbCluster;
  float alphaVals[kMaxNumDataPoints];

  float alphaMin = FLT_MAX, alphaMax = -FLT_MAX;
  for(int i = 0; i < cluster.GetNumPoints(); i++) {

    RGBAVector v = cluster.GetPoint(i);
    switch(GetRotationMode()) {
      default:
      case 0:
        // Do nothing
      break;

      case 1:
        swap(v.r, v.a);
        break;

      case 2:
        swap(v.g, v.a);
        break;

      case 3:
        swap(v.b, v.a);
        break;
    }

    alphaVals[i] = v.a;
    v.a = 255.0f;

    alphaMin = min(alphaVals[i], alphaMin);
    alphaMax = max(alphaVals[i], alphaMax);

    rgbCluster.AddPoint(v);
  }

  int dummyPbit = 0;
  RGBAVector rgbp1, rgbp2;
  double rgbError = CompressCluster(rgbCluster, rgbp1, rgbp2, bestIndices, dummyPbit);

  float a1 = alphaMin, a2 = alphaMax;
  double alphaError = DBL_MAX;

  typedef uint32 tInterpPair[2];
  typedef tInterpPair tInterpLevel[16];
  const tInterpLevel *interpVals = kBC7InterpolationValues + (GetNumberOfBitsPerAlpha() - 1);
  const float weight = GetErrorMetric().a;

  const int nBuckets = (1 << GetNumberOfBitsPerAlpha());

  // If they're the same, then we can get them exactly.
  if(a1 == a2) 
  {  
    const uint8 step = 1 << (8-GetAlphaChannelPrecision());
    const uint8 a1be = uint8(a1);
    const uint8 a2be = uint8(a2);
    const uint8 a1b = ::QuantizeChannel(a1be, (((char)0x80) >> (GetAlphaChannelPrecision() - 1)));
    const uint8 a2b = ::QuantizeChannel(a2be, (((char)0x80) >> (GetAlphaChannelPrecision() - 1)));

    // Mode 5 has 8 bits of precision for alpha.
    if(GetModeNumber() == 5) {

      assert(a1 == float(a1b));
      assert(a2 == float(a2b));

      for(int i = 0; i < kMaxNumDataPoints; i++)
        alphaIndices[i] = 0;

      alphaError = 0.0;
    }
    else {
      assert(GetModeNumber() == 4);
      
      // Mode 4 can be treated like the 6 channel of DXT1 compression.
      if(Optimal6CompressDXT1[a1be][0][0]) {
        a1 = float((Optimal6CompressDXT1[a1be][1][1] << 2) | (Optimal6CompressDXT1[a1be][0][1] >> 4));
        a2 = float((Optimal6CompressDXT1[a2be][1][2] << 2) | (Optimal6CompressDXT1[a2be][0][1] >> 4));
      }
      else {
        a1 = float((Optimal6CompressDXT1[a1be][0][1] << 2) | (Optimal6CompressDXT1[a1be][0][1] >> 4));
        a2 = float((Optimal6CompressDXT1[a2be][0][2] << 2) | (Optimal6CompressDXT1[a2be][0][1] >> 4));
      }

      if(m_IndexMode == 1) {
        for(int i = 0; i < kMaxNumDataPoints; i++)
          alphaIndices[i] = 1;
      }
      else {
        for(int i = 0; i < kMaxNumDataPoints; i++)
          alphaIndices[i] = 2;
      }

      uint32 interp0 = (*interpVals)[alphaIndices[0] & 0xFF][0];
      uint32 interp1 = (*interpVals)[alphaIndices[0] & 0xFF][1];

      const uint8 ip = (((uint32(a1) * interp0) + (uint32(a2) * interp1) + 32) >> 6) & 0xFF;
      float pxError = weight * float((a1be > ip)? a1be - ip : ip - a1be);
      pxError *= pxError;
      alphaError = 16 * pxError;
    }
  }
  else {

    float vals[1<<3];
    memset(vals, 0, sizeof(vals));

    int buckets[kMaxNumDataPoints];

    // Figure out initial positioning.
    for(int i = 0; i < nBuckets; i++) {
      vals[i] = alphaMin + (float(i)/float(nBuckets-1)) * (alphaMax - alphaMin);
    }

    // Assign each value to a bucket
    for(int i = 0; i < kMaxNumDataPoints; i++) {

      float minDist = 255.0f;
      for(int j = 0; j < nBuckets; j++) {
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
      for(int i = 0; i < nBuckets; i++) {
        for(int j = 0; j < kMaxNumDataPoints; j++) {

          if(buckets[j] == i) {
            avg[i] += alphaVals[j];
            npts[i] += 1.0f;
          }
        }

        if(npts[i] > 0.0f) 
          avg[i] /= npts[i];
      }

      // Did we change anything?
      fixed = true;
      for(int i = 0; i < nBuckets; i++) {
        fixed = fixed && (avg[i] == vals[i]);
      }

      // Reassign indices...
      memcpy(vals, avg, sizeof(vals));

      // Reassign each value to a bucket
      for(int i = 0; i < kMaxNumDataPoints; i++) {

        float minDist = 255.0f;
        for(int j = 0; j < nBuckets; j++) {
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
    for(int i = 0; i < nBuckets; i++) {
      float a = float(nBuckets - 1 - i) / float(nBuckets - 1);
      float b = float(i) / float(nBuckets - 1);

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
    a1 = min(255.0f, max(0.0f, a1));
    a2 = min(255.0f, max(0.0f, a2));

    // Quantize
    const uint8 a1b = ::QuantizeChannel(uint8(a1), (((char)0x80) >> (GetAlphaChannelPrecision() - 1)));
    const uint8 a2b = ::QuantizeChannel(uint8(a2), (((char)0x80) >> (GetAlphaChannelPrecision() - 1)));

    // Compute error
    for(int i = 0; i < kMaxNumDataPoints; i++) {

      uint8 val = uint8(alphaVals[i]);

      float minError = FLT_MAX;
      int bestBucket = -1;

      for(int j = 0; j < nBuckets; j++) {
        uint32 interp0 = (*interpVals)[j][0];
        uint32 interp1 = (*interpVals)[j][1];

        const uint8 ip = (((uint32(a1b) * interp0) + (uint32(a2b) * interp1) + 32) >> 6) & 0xFF;
        float pxError = weight * float((val > ip)? val - ip : ip - val);
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

  for(int i = 0; i < kNumColorChannels; i++) {
    p1.c[i] = (i == (kNumColorChannels-1))? a1 : rgbp1.c[i];
    p2.c[i] = (i == (kNumColorChannels-1))? a2 : rgbp2.c[i];
  }

  return rgbError + alphaError;
}

double BC7CompressionMode::CompressCluster(const RGBACluster &cluster, RGBAVector &p1, RGBAVector &p2, int *bestIndices, int &bestPbitCombo) const {
    
  // If all the points are the same in the cluster, then we need to figure out what the best
  // approximation to this point is....
  if(cluster.AllSamePoint()) {
    const RGBAVector &p = cluster.GetPoint(0);
    double bestErr = CompressSingleColor(p, p1, p2, bestPbitCombo);

    // We're assuming all indices will be index 1...
    for(int i = 0; i < cluster.GetNumPoints(); i++) {
      bestIndices[i] = 1;
    }
    
    return bestErr;
  }
  
  const int nBuckets = (1 << GetNumberOfBitsPerIndex());
  const int nPbitCombos = GetNumPbitCombos();
  const uint32 qmask = GetQuantizationMask();

#if 1
  RGBAVector avg = cluster.GetTotal() / float(cluster.GetNumPoints());
  RGBADir axis;
  ::GetPrincipalAxis(cluster.GetNumPoints(), cluster.GetPoints(), axis);

  float mindp = FLT_MAX, maxdp = -FLT_MAX;
  for(int i = 0 ; i < cluster.GetNumPoints(); i++) {
    float dp = (cluster.GetPoint(i) - avg) * axis;
    if(dp < mindp) mindp = dp;
    if(dp > maxdp) maxdp = dp;
  }
  
  p1 = avg + mindp * axis;
  p2 = avg + maxdp * axis;
#else
  cluster.GetBoundingBox(p1, p2);
#endif

  ClampEndpoints(p1, p2);

  RGBAVector pts[1 << 4]; // At most 4 bits per index.
  int numPts[1<<4];
  assert(nBuckets <= 1 << 4);

  for(int i = 0; i < nBuckets; i++) {
    float s = (float(i) / float(nBuckets - 1));
    pts[i] = (1.0f - s) * p1 + s * p2;
  }

  assert(pts[0] == p1);
  assert(pts[nBuckets - 1] == p2);

  // Do k-means clustering...
  int bucketIdx[kMaxNumDataPoints];

  bool fixed = false;
  while(!fixed) {
    
    RGBAVector newPts[1 << 4];

    // Assign each of the existing points to one of the buckets...
    for(int i = 0; i < cluster.GetNumPoints(); i++) {

      int minBucket = -1;
      float minDist = FLT_MAX;
      for(int j = 0; j < nBuckets; j++) {
        RGBAVector v = cluster.GetPoint(i) - pts[j];
        float distSq = v * v;
        if(distSq < minDist)
        {
          minDist = distSq;
          minBucket = j;
        }
      }

      assert(minBucket >= 0);
      bucketIdx[i] = minBucket;
    }

    // Calculate new buckets based on centroids of clusters...
    for(int i = 0; i < nBuckets; i++) {
      
      numPts[i] = 0;
      newPts[i] = RGBAVector(0.0f);
      for(int j = 0; j < cluster.GetNumPoints(); j++) {
        if(bucketIdx[j] == i) {
          numPts[i]++;
          newPts[i] += cluster.GetPoint(j);
        }
      }

      // If there are no points in this cluster, then it should
      // remain the same as last time and avoid a divide by zero.
      if(0 != numPts[i])
        newPts[i] /= float(numPts[i]);
    }

    // If we haven't changed, then we're done.
    fixed = true;
    for(int i = 0; i < nBuckets; i++) {
      if(pts[i] != newPts[i])
        fixed = false;
    }

    // Assign the new points to be the old points.
    for(int i = 0; i < nBuckets; i++) {
      pts[i] = newPts[i];
    }
  }

  // If there's only one bucket filled, then just compress for that single color...
  int numBucketsFilled = 0, lastFilledBucket = -1;
  for(int i = 0; i < nBuckets; i++) {
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
    for(int i = 0; i < cluster.GetNumPoints(); i++) {
      bestIndices[i] = 1;
    }
      
    return bestErr;
  }

  // Now that we know the index of each pixel, we can assign the endpoints based on a least squares fit
  // of the clusters. For more information, take a look at this article by NVidia:
  // http://developer.download.nvidia.com/compute/cuda/1.1-Beta/x86_website/projects/dxtc/doc/cuda_dxtc.pdf
  float asq = 0.0, bsq = 0.0, ab = 0.0;
  RGBAVector ax(0.0), bx(0.0);
  for(int i = 0; i < nBuckets; i++) {
    float a = float(nBuckets - 1 - i) / float(nBuckets - 1);
    float b = float(i) / float(nBuckets - 1);

    int n = numPts[i];
    RGBAVector x = pts[i];

    asq += float(n) * a * a;
    bsq += float(n) * b * b;
    ab += float(n) * a * b;

    ax += x * a * float(n);
    bx += x * b * float(n);
  }

  float f = 1.0f / (asq * bsq - ab * ab);
  p1 = f * (ax * bsq - bx * ab);
  p2 = f * (bx * asq - ax * ab);

  ClampEndpointsToGrid(p1, p2, bestPbitCombo);

  #ifdef _DEBUG
    int pBitCombo = bestPbitCombo;
    RGBAVector tp1 = p1, tp2 = p2;
    ClampEndpointsToGrid(tp1, tp2, pBitCombo);

    assert(p1 == tp1);
    assert(p2 == tp2);
    assert(pBitCombo == bestPbitCombo);
  #endif

  assert(bestPbitCombo >= 0);

  return OptimizeEndpointsForCluster(cluster, p1, p2, bestIndices, bestPbitCombo);
}

double BC7CompressionMode::Compress(BitStream &stream, const int shapeIdx, const RGBACluster *clusters) {

  const int kModeNumber = GetModeNumber();
  const int nPartitionBits = GetNumberOfPartitionBits();
  const int nSubsets = GetNumberOfSubsets();

  // Mode #
  stream.WriteBits(1 << kModeNumber, kModeNumber + 1);

  // Partition #
  assert((((1 << nPartitionBits) - 1) & shapeIdx) == shapeIdx);
  stream.WriteBits(shapeIdx, nPartitionBits);
    
  RGBAVector p1[kMaxNumSubsets], p2[kMaxNumSubsets];
  int bestIndices[kMaxNumSubsets][kMaxNumDataPoints] = {
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
  };
  int bestAlphaIndices[kMaxNumDataPoints] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
  int bestPbitCombo[kMaxNumSubsets] = { -1, -1, -1 };
  int bestRotationMode = -1, bestIndexMode = -1;

  double totalErr = 0.0;
  for(int cidx = 0; cidx < nSubsets; cidx++) {
    int indices[kMaxNumDataPoints];

    if(m_Attributes->hasRotation) {

      assert(nSubsets == 1);

      int alphaIndices[kMaxNumDataPoints];

      double bestError = DBL_MAX;
      for(int rotMode = 0; rotMode < 4; rotMode++) {

        SetRotationMode(rotMode);
        const int nIdxModes = kModeNumber == 4? 2 : 1;

        for(int idxMode = 0; idxMode < nIdxModes; idxMode++) {

          SetIndexMode(idxMode);

          RGBAVector v1, v2;
          double error = CompressCluster(clusters[cidx], v1, v2, indices, alphaIndices);
          if(error < bestError) {
            bestError = error;

            memcpy(bestIndices[cidx], indices, sizeof(indices));
            memcpy(bestAlphaIndices, alphaIndices, sizeof(alphaIndices));

            bestRotationMode = rotMode;
            bestIndexMode = idxMode;

            p1[cidx] = v1;
            p2[cidx] = v2;
          }
        }
      }

      totalErr += bestError;
    }
    else {
      // Compress this cluster
      totalErr += CompressCluster(clusters[cidx], p1[cidx], p2[cidx], indices, bestPbitCombo[cidx]);

      // Map the indices to their proper position.
      int idx = 0;
      for(int i = 0; i < 16; i++) {
        int subs = GetSubsetForIndex(i, shapeIdx, GetNumberOfSubsets());
        if(subs == cidx) {
          bestIndices[cidx][i] = indices[idx++];
        }
      }
    }
  }

  stream.WriteBits(bestRotationMode, m_Attributes->hasRotation? 2 : 0);
  stream.WriteBits(bestIndexMode, m_Attributes->hasIdxMode? 1 : 0);

#ifdef _DEBUG
  for(int i = 0; i < kMaxNumDataPoints; i++) {

    int nSet = 0;
    for(int j = 0; j < nSubsets; j++) {
      if(bestIndices[j][i] >= 0)
        nSet++;
    }

    assert(nSet == 1);
  }
#endif

  // Get the quantization mask
  const uint32 qmask = GetQuantizationMask();

  //Quantize the points...
  uint32 pixel1[kMaxNumSubsets], pixel2[kMaxNumSubsets];
  for(int i = 0; i < nSubsets; i++) {
    switch(GetPBitType()) {
      default:
      case ePBitType_None: 
        pixel1[i] = p1[i].ToPixel(qmask); 
        pixel2[i] = p2[i].ToPixel(qmask); 
      break;

      case ePBitType_Shared: 
      case ePBitType_NotShared: 
        pixel1[i] = p1[i].ToPixel(qmask, GetPBitCombo(bestPbitCombo[i])[0]); 
        pixel2[i] = p2[i].ToPixel(qmask, GetPBitCombo(bestPbitCombo[i])[1]); 
      break;
    }
  }

  // If the anchor index does not have 0 in the leading bit, then 
  // we need to swap EVERYTHING.
  for(int sidx = 0; sidx < nSubsets; sidx++) {

    int anchorIdx = GetAnchorIndexForSubset(sidx, shapeIdx, nSubsets);
    assert(bestIndices[sidx][anchorIdx] != -1);

    const int nAlphaIndexBits = GetNumberOfBitsPerAlpha(bestIndexMode);
    const int nIndexBits = GetNumberOfBitsPerIndex(bestIndexMode);
    if(bestIndices[sidx][anchorIdx] >> (nIndexBits - 1)) {
      uint32 t = pixel1[sidx]; pixel1[sidx] = pixel2[sidx]; pixel2[sidx] = t;

      int nIndexVals = 1 << nIndexBits;
      for(int i = 0; i < 16; i++) {
        bestIndices[sidx][i] = (nIndexVals - 1) - bestIndices[sidx][i];
      }

      int nAlphaIndexVals = 1 << nAlphaIndexBits;
      if(m_Attributes->hasRotation) {
        for(int i = 0; i < 16; i++) {
          bestAlphaIndices[i] = (nAlphaIndexVals - 1) - bestAlphaIndices[i];
        }
      }
    }

    if(m_Attributes->hasRotation && bestAlphaIndices[anchorIdx] >> (nAlphaIndexBits - 1)) {
      uint8 * bp1 = (uint8 *)(&pixel1[sidx]); 
      uint8 * bp2 = (uint8 *)(&pixel2[sidx]); 
      uint8 t = bp1[3]; bp1[3] = bp2[3]; bp2[3] = t;

      int nAlphaIndexVals = 1 << nAlphaIndexBits;
      for(int i = 0; i < 16; i++) {
        bestAlphaIndices[i] = (nAlphaIndexVals - 1) - bestAlphaIndices[i];
      }
    }

    assert(!(bestIndices[sidx][anchorIdx] >> (nIndexBits - 1)));
    assert(!m_Attributes->hasRotation || !(bestAlphaIndices[anchorIdx] >> (nAlphaIndexBits - 1)));
  }

  // Get the quantized values...
  uint8 r1[kMaxNumSubsets], g1[kMaxNumSubsets], b1[kMaxNumSubsets], a1[kMaxNumSubsets];
  uint8 r2[kMaxNumSubsets], g2[kMaxNumSubsets], b2[kMaxNumSubsets], a2[kMaxNumSubsets];
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
      const int *pbits = GetPBitCombo(bestPbitCombo[s]);
      stream.WriteBits(pbits[0], 1);
      if(GetPBitType() != ePBitType_Shared)
        stream.WriteBits(pbits[1], 1);
    }
  }

  // If our index mode has changed, then we need to write the alpha indices first.
  if(m_Attributes->hasIdxMode && bestIndexMode == 1) {

    assert(m_Attributes->hasRotation);

    for(int i = 0; i < 16; i++) {
      const int idx = bestAlphaIndices[i];
      assert(GetAnchorIndexForSubset(0, shapeIdx, nSubsets) == 0);
      assert(GetNumberOfBitsPerAlpha(bestIndexMode) == 2);
      assert(idx >= 0 && idx < (1 << 2));
      assert(i != 0 || !(idx >> 1) || !"Leading bit of anchor index is not zero!");
      stream.WriteBits(idx, (i == 0)? 1 : 2);
    }

    for(int i = 0; i < 16; i++) {
      const int idx = bestIndices[0][i];
      assert(GetSubsetForIndex(i, shapeIdx, nSubsets) == 0);
      assert(GetAnchorIndexForSubset(0, shapeIdx, nSubsets) == 0);
      assert(GetNumberOfBitsPerIndex(bestIndexMode) == 3);
      assert(idx >= 0 && idx < (1 << 3));
      assert(i != 0 || !(idx >> 2) || !"Leading bit of anchor index is not zero!");
      stream.WriteBits(idx, (i == 0)? 2 : 3);
    }
  }
  else {
    for(int i = 0; i < 16; i++) {
      const int subs = GetSubsetForIndex(i, shapeIdx, nSubsets);
      const int idx = bestIndices[subs][i];
      const int anchorIdx = GetAnchorIndexForSubset(subs, shapeIdx, nSubsets);
      const int nBitsForIdx = GetNumberOfBitsPerIndex(bestIndexMode);
      assert(idx >= 0 && idx < (1 << nBitsForIdx));
      assert(i != anchorIdx || !(idx >> (nBitsForIdx - 1)) || !"Leading bit of anchor index is not zero!");
      stream.WriteBits(idx, (i == anchorIdx)? nBitsForIdx - 1 : nBitsForIdx);
    }

    if(m_Attributes->hasRotation) {
      for(int i = 0; i < 16; i++) {
        const int idx = bestAlphaIndices[i];
        const int anchorIdx = 0;
        const int nBitsForIdx = GetNumberOfBitsPerAlpha(bestIndexMode);
        assert(idx >= 0 && idx < (1 << nBitsForIdx));
        assert(i != anchorIdx || !(idx >> (nBitsForIdx - 1)) || !"Leading bit of anchor index is not zero!");
        stream.WriteBits(idx, (i == anchorIdx)? nBitsForIdx - 1 : nBitsForIdx);
      }
    }
  }
  assert(stream.GetBitsWritten() == 128);
  return totalErr;
}

namespace BC7C
{
  static ErrorMetric gErrorMetric = eErrorMetric_Uniform;
  void SetErrorMetric(ErrorMetric e) { gErrorMetric = e; }

  ALIGN_SSE const float kErrorMetrics[kNumErrorMetrics][kNumColorChannels] = {
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { sqrtf(0.3f), sqrtf(0.56f), sqrtf(0.11f), 1.0f }
  };

  const float *GetErrorMetric() { return kErrorMetrics[GetErrorMetricEnum()]; }
  ErrorMetric GetErrorMetricEnum() { return gErrorMetric; }

  // Function prototypes
  static void ExtractBlock(const uint8* inPtr, int width, uint32* colorBlock);
  static void CompressBC7Block(const uint32 *block, uint8 *outBuf, BlockStatManager *statManager);

  static int gQualityLevel = 50;
  void SetQualityLevel(int q) {
    gQualityLevel = max(0, q);
  }
  int GetQualityLevel() { return gQualityLevel; }

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

    stream.WriteBits(1 << 5, 6); // Mode 5
    stream.WriteBits(0, 2); // No rotation bits.

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

  static int gModeChosen = -1;
  static int gBestMode = -1;

  static void DecompressBC7Block(const uint8 block[16], uint32 outBuf[16]);

  // Compress an image using BC7 compression. Use the inBuf parameter to point to an image in
  // 4-byte RGBA format. The width and height parameters specify the size of the image in pixels.
  // The buffer pointed to by outBuf should be large enough to store the compressed image. This
  // implementation has an 4:1 compression ratio.
  void CompressImageBC7(const unsigned char *inBuf, unsigned char *outBuf, unsigned int width, unsigned int height)
  {
    uint32 block[16];
    BC7CompressionMode::ResetNumUses();
    BC7CompressionMode::MaxAnnealingIterations = min(BC7CompressionMode::kMaxAnnealingIterations, GetQualityLevel());

    for(int j = 0; j < height; j += 4)
    {
      for(int i = 0; i < width; i += 4)
      {
        // ExtractBlock(inBuf + i * 4, width, block);
        CompressBC7Block((const uint32 *)inBuf, outBuf, NULL);
        BC7CompressionMode::NumUses[gBestMode]++;

#ifndef NDEBUG
        uint8 *block = (uint8 *)outBuf;
        uint32 unComp[16];
        DecompressBC7Block(block, unComp);
        uint8* unCompData = (uint8 *)unComp;

        int diffSum = 0;
        for(int i = 0; i < 64; i++) {
          diffSum += sad(unCompData[i], inBuf[i]);
        }
        double blockError = double(diffSum) / 64.0;
        if(blockError > 50.0) {
          fprintf(stderr, "WARNING: Block error very high (%.2f)\n", blockError);
        }
#endif

        outBuf += 16;
        inBuf += 64;
      }
    }
  }

  void CompressImageBC7Stats(
    const unsigned char *inBuf, 
    unsigned char *outBuf, 
    unsigned int width, 
    unsigned int height,
    BlockStatManager &statManager
  ) {
    uint32 block[16];
    BC7CompressionMode::ResetNumUses();
    BC7CompressionMode::MaxAnnealingIterations = min(BC7CompressionMode::kMaxAnnealingIterations, GetQualityLevel());

    for(int j = 0; j < height; j += 4)
    {
      for(int i = 0; i < width; i += 4)
      {
        // ExtractBlock(inBuf + i * 4, width, block);
        CompressBC7Block((const uint32 *)inBuf, outBuf, &statManager);
        BC7CompressionMode::NumUses[gBestMode]++;

#ifndef NDEBUG
        uint8 *block = (uint8 *)outBuf;
        uint32 unComp[16];
        DecompressBC7Block(block, unComp);
        uint8* unCompData = (uint8 *)unComp;

        int diffSum = 0;
        for(int i = 0; i < 64; i++) {
          diffSum += sad(unCompData[i], inBuf[i]);
        }
        double blockError = double(diffSum) / 64.0;
        if(blockError > 50.0) {
          fprintf(stderr, "WARNING: Block error very high (%.2f)\n", blockError);
        }
#endif

        outBuf += 16;
        inBuf += 64;
      }
    }
  }

  // Extract a 4 by 4 block of pixels from inPtr and store it in colorBlock. The width parameter
  // specifies the size of the image in pixels.
  static void ExtractBlock(const uint8* inPtr, int width, uint32* colorBlock)
  {
    for(int j = 0; j < 4; j++)
    {
      memcpy(&colorBlock[j * 4], inPtr, 4 * 4);
      inPtr += width * 4;
    }
  }

  static double CompressTwoClusters(int shapeIdx, const RGBACluster *clusters, uint8 *outBuf, bool opaque) {

    uint8 tempBuf1[16];
    BitStream tmpStream1(tempBuf1, 128, 0);
    BC7CompressionMode compressor1(1, opaque);
      
    double bestError = compressor1.Compress(tmpStream1, shapeIdx, clusters);
    memcpy(outBuf, tempBuf1, 16);
    gModeChosen = 1;
    if(bestError == 0.0) {
      return 0.0;
    }

    uint8 tempBuf3[16];
    BitStream tmpStream3(tempBuf3, 128, 0);
    BC7CompressionMode compressor3(3, opaque);

    double error;
    if((error = compressor3.Compress(tmpStream3, shapeIdx, clusters)) < bestError) {
      gModeChosen = 3;
      bestError = error;
      memcpy(outBuf, tempBuf3, 16);
      if(bestError == 0.0) {
        return 0.0;
      }
    }
    
    // Mode 3 offers more precision for RGB data. Mode 7 is really only if we have alpha.
    if(!opaque) 
    {
      uint8 tempBuf7[16];
      BitStream tmpStream7(tempBuf7, 128, 0);
      BC7CompressionMode compressor7(7, opaque);    
      if((error = compressor7.Compress(tmpStream7, shapeIdx, clusters)) < bestError) {
        gModeChosen = 7;
        memcpy(outBuf, tempBuf7, 16);
        return error;
      }
    }

    return bestError;
  }

  static double CompressThreeClusters(int shapeIdx, const RGBACluster *clusters, uint8 *outBuf, bool opaque) {

    uint8 tempBuf0[16];
    BitStream tmpStream0(tempBuf0, 128, 0);

    uint8 tempBuf2[16];
    BitStream tmpStream2(tempBuf2, 128, 0);

    BC7CompressionMode compressor0(0, opaque);
    BC7CompressionMode compressor2(2, opaque);
      
    double error, bestError = (shapeIdx < 16)? compressor0.Compress(tmpStream0, shapeIdx, clusters) : DBL_MAX;
    gModeChosen = 0;
    memcpy(outBuf, tempBuf0, 16);
    if(bestError == 0.0) {
      return 0.0;
    }

    if((error = compressor2.Compress(tmpStream2, shapeIdx, clusters)) < bestError) {
      gModeChosen = 2;
      memcpy(outBuf, tempBuf2, 16);
      return error;
    }

    return bestError;
  }

  static void PopulateTwoClustersForShape(const RGBACluster &points, int shapeIdx, RGBACluster *clusters) {
    const uint16 shape = kShapeMask2[shapeIdx]; 
    for(int pt = 0; pt < kMaxNumDataPoints; pt++) {

      const RGBAVector &p = points.GetPoint(pt);

      if((1 << pt) & shape)
        clusters[1].AddPoint(p);
      else
        clusters[0].AddPoint(p);
    }

    assert(!(clusters[0].GetPointBitString() & clusters[1].GetPointBitString()));
    assert((clusters[0].GetPointBitString() ^ clusters[1].GetPointBitString()) == 0xFFFF);
    assert((shape & clusters[1].GetPointBitString()) == shape);
  }

  static void PopulateThreeClustersForShape(const RGBACluster &points, int shapeIdx, RGBACluster *clusters) {
    for(int pt = 0; pt < kMaxNumDataPoints; pt++) {

      const RGBAVector &p = points.GetPoint(pt);

      if((1 << pt) & kShapeMask3[shapeIdx][0]) {
        if((1 << pt) & kShapeMask3[shapeIdx][1])
          clusters[2].AddPoint(p);
        else
          clusters[1].AddPoint(p);
      }
      else
        clusters[0].AddPoint(p);
    }

    assert(!(clusters[0].GetPointBitString() & clusters[1].GetPointBitString()));
    assert(!(clusters[2].GetPointBitString() & clusters[1].GetPointBitString()));
    assert(!(clusters[0].GetPointBitString() & clusters[2].GetPointBitString()));
  }

  static double EstimateTwoClusterError(RGBACluster &c) {
    RGBAVector Min, Max, v;
    c.GetBoundingBox(Min, Max);
    v = Max - Min;
    if(v * v == 0) {
      return 0.0;
    }

    const float *w = BC7C::GetErrorMetric();
    return 0.0001 + c.QuantizedError(Min, Max, 8, 0xFFFFFFFF, RGBAVector(w[0], w[1], w[2], w[3]));
  }

  static double EstimateThreeClusterError(RGBACluster &c) {
    RGBAVector Min, Max, v;
    c.GetBoundingBox(Min, Max);
    v = Max - Min;
    if(v * v == 0) {
      return 0.0;
    }

    const float *w = BC7C::GetErrorMetric();
    return 0.0001 + c.QuantizedError(Min, Max, 4, 0xFFFFFFFF, RGBAVector(w[0], w[1], w[2], w[3]));
  }

  // Compress a single block.
  static void CompressBC7Block(const uint32 *block, uint8 *outBuf, BlockStatManager *statManager) {

    uint32 blockIdx = 0;
    if(statManager) {
      blockIdx = statManager->BeginBlock();

      for(int i = 0; i < kNumBlockStats; i++) {
        statManager->AddStat(blockIdx, BlockStat(kBlockStatString[i], 0));
      }
    }

    // All a single color?
    if(AllOneColor(block)) {
      BitStream bStrm(outBuf, 128, 0);
      CompressOptimalColorBC7(*block, bStrm);
      gBestMode = 5;
      
      if(statManager) {
        BlockStat s = BlockStat(kBlockStatString[eBlockStat_Path], 0);
        statManager->AddStat(blockIdx, s);

        s = BlockStat(kBlockStatString[eBlockStat_Mode], 5);
        statManager->AddStat(blockIdx, s);
      }

      return;
    }

    RGBACluster blockCluster;
    bool opaque = true;
    bool transparent = true;

    for(int i = 0; i < kMaxNumDataPoints; i++) {
      RGBAVector p = RGBAVector(i, block[i]);
      blockCluster.AddPoint(p);
      if(fabs(p.a - 255.0f) > 1e-10)
        opaque = false;

      if(p.a > 0.0f)
        transparent = false;
    }

    // The whole block is transparent?
    if(transparent) {
      BitStream bStrm(outBuf, 128, 0);
      WriteTransparentBlock(bStrm);
      gBestMode = 6;

      if(statManager) {
        BlockStat s = BlockStat(kBlockStatString[eBlockStat_Path], 1);
        statManager->AddStat(blockIdx, s);
        
        s = BlockStat(kBlockStatString[eBlockStat_Mode], gBestMode);
        statManager->AddStat(blockIdx, s);
      }

      return;
    }

    // First we must figure out which shape to use. To do this, simply
    // see which shape has the smallest sum of minimum bounding spheres.
    double bestError[2] = { DBL_MAX, DBL_MAX };
    int bestShapeIdx[2] = { -1, -1 };
    RGBACluster bestClusters[2][3];

    for(int i = 0; i < kNumShapes2; i++) 
    {
      RGBACluster clusters[2];
      PopulateTwoClustersForShape(blockCluster, i, clusters);

      double err = 0.0;
      for(int ci = 0; ci < 2; ci++) {
        err += EstimateTwoClusterError(clusters[ci]);
      }

      // If it's small, we'll take it!
      if(err < 1e-9) {
        CompressTwoClusters(i, clusters, outBuf, opaque);
        gBestMode = gModeChosen;

        if(statManager) {
          BlockStat s = BlockStat(kBlockStatString[eBlockStat_Path], 2);
          statManager->AddStat(blockIdx, s);

          s = BlockStat(kBlockStatString[eBlockStat_Mode], gBestMode);
          statManager->AddStat(blockIdx, s);
        }
        return;
      }
      
      if(err < bestError[0]) {
        bestError[0] = err;
        bestShapeIdx[0] = i;
        bestClusters[0][0] = clusters[0];
        bestClusters[0][1] = clusters[1];
      }
    }

    // There are not 3 subset blocks that support alpha, so only check these
    // if the entire block is opaque.
    if(opaque) {
      for(int i = 0; i < kNumShapes3; i++) {

        RGBACluster clusters[3];
        PopulateThreeClustersForShape(blockCluster, i, clusters);

        double err = 0.0;
        for(int ci = 0; ci < 3; ci++) {
          err += EstimateThreeClusterError(clusters[ci]);
        }

        // If it's small, we'll take it!
        if(err < 1e-9) {
          CompressThreeClusters(i, clusters, outBuf, opaque);
          gBestMode = gModeChosen;

          if(statManager) {
            BlockStat s = BlockStat(kBlockStatString[eBlockStat_Path], 2);
            statManager->AddStat(blockIdx, s);

            s = BlockStat(kBlockStatString[eBlockStat_Mode], gBestMode);
            statManager->AddStat(blockIdx, s);
          }

          return;
        }

        if(err < bestError[1]) {
          bestError[1] = err;
          bestShapeIdx[1] = i;
          bestClusters[1][0] = clusters[0];
          bestClusters[1][1] = clusters[1];
          bestClusters[1][2] = clusters[2];
        }
      }
    }
                
    if(statManager) {
      BlockStat s = BlockStat(kBlockStatString[eBlockStat_Path], 3);
      statManager->AddStat(blockIdx, s);
    }

    uint8 tempBuf1[16], tempBuf2[16];

    BitStream tempStream1 (tempBuf1, 128, 0);
    BC7CompressionMode compressor(6, opaque);
    double best = compressor.Compress(tempStream1, 0, &blockCluster);
    gBestMode = 6;
    if(best == 0.0f) {

      if(statManager) {
        BlockStat s = BlockStat(kBlockStatString[eBlockStat_Mode], gBestMode);
        statManager->AddStat(blockIdx, s);
      }

      memcpy(outBuf, tempBuf1, 16);
      return;
    }

    // Check modes 4 and 5 if the block isn't opaque...
    if(!opaque) {
      for(int mode = 4; mode <= 5; mode++) {

        BitStream tempStream2(tempBuf2, 128, 0);
        BC7CompressionMode compressorTry(mode, opaque);

        double error = compressorTry.Compress(tempStream2, 0, &blockCluster);
        if(error < best) {

          gBestMode = mode;
          best = error;

          if(best == 0.0f) {
            memcpy(outBuf, tempBuf2, 16);
            return;
          }
          else {
            memcpy(tempBuf1, tempBuf2, 16);
          }
        }
      }
    }

    double error = CompressTwoClusters(bestShapeIdx[0], bestClusters[0], tempBuf2, opaque);
    if(error < best) {

      gBestMode = gModeChosen;
      best = error;
      
      if(error == 0.0f) {
        memcpy(outBuf, tempBuf2, 16);

        if(statManager) {
          BlockStat s = BlockStat(kBlockStatString[eBlockStat_Mode], gBestMode);
          statManager->AddStat(blockIdx, s);
        }

        return;
      }
      else {
        memcpy(tempBuf1, tempBuf2, 16);
      }
    }

    if(opaque) {
      if(CompressThreeClusters(bestShapeIdx[1], bestClusters[1], tempBuf2, opaque) < best) {

        gBestMode = gModeChosen;
        memcpy(outBuf, tempBuf2, 16);

        if(statManager) {
          BlockStat s = BlockStat(kBlockStatString[eBlockStat_Mode], gBestMode);
          statManager->AddStat(blockIdx, s);
        }

        return;
      }
    }

    memcpy(outBuf, tempBuf1, 16);

    if(statManager) {
      BlockStat s = BlockStat(kBlockStatString[eBlockStat_Mode], gBestMode);
      statManager->AddStat(blockIdx, s);
    }
  }

  static void DecompressBC7Block(const uint8 block[16], uint32 outBuf[16]) {

    BitStreamReadOnly strm(block);
    
    uint32 mode = 0;
    while(!strm.ReadBit()) {
      mode++;
    }

    const BC7CompressionMode::Attributes *attrs = BC7CompressionMode::GetAttributesForMode(mode);
    const uint32 nSubsets = attrs->numSubsets;

    uint32 idxMode = 0;
    uint32 rotMode = 0;
    uint32 shapeIdx = 0;
    if ( nSubsets > 1 ) {
      shapeIdx = strm.ReadBits(mode == 0? 4 : 6);
    }
    else if( attrs->hasRotation ) {
      rotMode = strm.ReadBits(2);
      if( attrs->hasIdxMode )
        idxMode = strm.ReadBit();
    }

    assert(idxMode < 2);
    assert(rotMode < 4);
    assert(shapeIdx < ((mode == 0)? 16 : 64));

    uint32 cp = attrs->colorChannelPrecision;
    const uint32 shift = 8 - cp;

    uint8 eps[3][2][4];
    for(uint32 ch = 0; ch < 3; ch++)
    for(uint32 i = 0; i < nSubsets; i++)
    for(uint32 ep = 0; ep < 2; ep++) 
      eps[i][ep][ch] = strm.ReadBits(cp) << shift;

    uint32 ap = attrs->alphaChannelPrecision;
    const uint32 ash = 8 - ap;

    for(uint32 i = 0; i < nSubsets; i++)
    for(uint32 ep = 0; ep < 2; ep++) 
      eps[i][ep][3] = strm.ReadBits(ap) << ash;

    // Handle pbits
    switch(attrs->pbitType) {
      case BC7CompressionMode::ePBitType_None:
        // Do nothing.
      break;

      case BC7CompressionMode::ePBitType_Shared:

        cp += 1;
        ap += 1;

        for(uint32 i = 0; i < nSubsets; i++) {

          uint32 pbit = strm.ReadBit();

          for(uint32 j = 0; j < 2; j++)
          for(uint32 ch = 0; ch < kNumColorChannels; ch++) {
            const uint32 prec = ch == 3? ap : cp;
            eps[i][j][ch] |= pbit << (8-prec);
          }
        }
      break;

      case BC7CompressionMode::ePBitType_NotShared:
        
        cp += 1;
        ap += 1;
      
        for(uint32 i = 0; i < nSubsets; i++)
        for(uint32 j = 0; j < 2; j++) {

          uint32 pbit = strm.ReadBit();

          for(uint32 ch = 0; ch < kNumColorChannels; ch++) {
            const uint32 prec = ch == 3? ap : cp;
            eps[i][j][ch] |= pbit << (8-prec);
          }
        }
      break;
    }

    // Quantize endpoints...
    for(uint32 i = 0; i < nSubsets; i++)
    for(uint32 j = 0; j < 2; j++)
    for(uint32 ch = 0; ch < kNumColorChannels; ch++) {
      const uint32 prec = ch == 3? ap : cp;
      eps[i][j][ch] |= eps[i][j][ch] >> prec;    
    }

    // Figure out indices...
    uint32 alphaIndices[kMaxNumDataPoints];
    uint32 colorIndices[kMaxNumDataPoints];

    int nBitsPerAlpha = attrs->numBitsPerAlpha;
    int nBitsPerColor = attrs->numBitsPerIndex;

    uint32 idxPrec = attrs->numBitsPerIndex;
    for(int i = 0; i < kMaxNumDataPoints; i++) {
      uint32 subset = GetSubsetForIndex(i, shapeIdx, nSubsets);

      int idx = 0;
      if(GetAnchorIndexForSubset(subset, shapeIdx, nSubsets) == i) {
        idx = strm.ReadBits(idxPrec - 1);
      }
      else {
        idx = strm.ReadBits(idxPrec);
      }
      colorIndices[i] = idx;
    }

    idxPrec = attrs->numBitsPerAlpha;
    if(idxPrec == 0) {
      memcpy(alphaIndices, colorIndices, sizeof(alphaIndices));
    }
    else {
      for(int i = 0; i < kMaxNumDataPoints; i++) {
        uint32 subset = GetSubsetForIndex(i, shapeIdx, nSubsets);

        int idx = 0;
        if(GetAnchorIndexForSubset(subset, shapeIdx, nSubsets) == i) {
          idx = strm.ReadBits(idxPrec - 1);
        }
        else {
          idx = strm.ReadBits(idxPrec);
        }
        alphaIndices[i] = idx;
      }

      if(idxMode) {
        for(int i = 0; i < kMaxNumDataPoints; i++) {
          swap(alphaIndices[i], colorIndices[i]);
        }

        swap(nBitsPerAlpha, nBitsPerColor);
      }
    }

    assert(strm.GetBitsRead() == 128);

    // Get final colors by interpolating...
    for(int i = 0; i < kMaxNumDataPoints; i++) {

      const uint32 subset = GetSubsetForIndex(i, shapeIdx, nSubsets);
      uint32 &pixel = outBuf[i];
      
      pixel = 0;
      for(int ch = 0; ch < 4; ch++) {
        uint32 i0 = kBC7InterpolationValues[nBitsPerColor - 1][colorIndices[i]][0];
        uint32 i1 = kBC7InterpolationValues[nBitsPerColor - 1][colorIndices[i]][1];

        const uint8 ip = (((uint32(eps[subset][0][ch]) * i0) + (uint32(eps[subset][1][ch]) * i1) + 32) >> 6) & 0xFF;
        pixel |= ip << (8*ch);
      }

      if(attrs->alphaChannelPrecision > 0) {
        uint32 i0 = kBC7InterpolationValues[nBitsPerAlpha - 1][alphaIndices[i]][0];
        uint32 i1 = kBC7InterpolationValues[nBitsPerAlpha - 1][alphaIndices[i]][1];

        const uint8 ip = (((uint32(eps[subset][0][3]) * i0) + (uint32(eps[subset][1][3]) * i1) + 32) >> 6) & 0xFF;
        pixel |= ip << 24;
      }
      else {
        pixel |= 0xFF000000;
      }
      
      // Swap colors if necessary...
      uint8 *pb = (uint8 *)&pixel;
      switch(rotMode) {
        default:
        case 0:
          // Do nothing
          break;

        case 1:
          swap(pb[0], pb[3]);
          break;

        case 2:
          swap(pb[1], pb[3]);
          break;

        case 3:
          swap(pb[2], pb[3]);
          break;
      }
    }
  }

  // Convert the image from a BC7 buffer to a RGBA8 buffer
  void DecompressImageBC7(const uint8 *inBuf, uint8* outBuf, unsigned int width, unsigned int height) {

    unsigned int blockIdx = 0;
    //    for(unsigned int j = 0; j < height; j += 4, outBuf += width * 3 * 4)
    for(unsigned int j = 0; j < height; j += 4)
    {
      for(unsigned int i = 0; i < width; i += 4)
      {
        uint32 pixels[16];
        DecompressBC7Block(inBuf + (16*(blockIdx++)), pixels);

        memcpy(outBuf, pixels, 16 * sizeof(uint32));
        //memcpy(outBuf + (width * 4), pixels + 4, 4 * sizeof(uint32));
        //memcpy(outBuf + 2*(width * 4), pixels + 8, 4 * sizeof(uint32));
        //memcpy(outBuf + 3*(width * 4), pixels + 12, 4 * sizeof(uint32));
        //outBuf += 16;
        outBuf += 64;
      }
    }
  }
}
