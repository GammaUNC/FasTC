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

#include "FasTC/BPTCConfig.h"
#include "FasTC/BPTCCompressor.h"

#include "RGBAEndpoints.h"
#include "CompressionMode.h"

#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <cstdio>

#ifndef min
template <typename T>
static T min(const T &a, const T &b) {
  return (a > b)? b : a;
}
#endif

#ifndef max
template <typename T>
static T max(const T &a, const T &b) {
  return (a > b)? a : b;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Static helper functions
//
///////////////////////////////////////////////////////////////////////////////
static inline uint32 CountBitsInMask(uint8 n) {

#if defined(_WIN64) || defined(__x86_64__) || defined(NO_INLINE_ASSEMBLY)
  if(!n) return 0; // no bits set
  if(!(n & (n-1))) return 1; // power of two

  uint32 c;
  for(c = 0; n; c++) {
    n &= n - 1;
  }
  return c;
#else
#ifdef _MSC_VER
  __asm {
    mov eax, 8
    movzx ecx, n
    bsf ecx, ecx
    sub eax, ecx
        }
#else
  uint32 ans;
  __asm__("movl $8, %%eax;"
    "movzbl %b1, %%ecx;"
    "bsf %%ecx, %%ecx;"
    "subl %%ecx, %%eax;"
    "movl %%eax, %0;"
    : "=Q"(ans)
    : "b"(n)
    : "%eax", "%ecx"
  );
  return ans;
#endif  
#endif
}

template <typename ty>
static inline void clamp(ty &x, const ty &min, const ty &max) {
  x = (x < min)? min : ((x > max)? max : x);
}

// absolute distance. It turns out the compiler does a much
// better job of optimizing this than we can, since we can't 
// translate the values to/from registers
template <typename ty>
static ty sad(ty a, ty b) {
  return (a > b)? a - b : b - a;
}

///////////////////////////////////////////////////////////////////////////////
//
// RGBAVector implementation
//
///////////////////////////////////////////////////////////////////////////////

uint8 QuantizeChannel(const uint8 val, const uint8 mask, const int pBit) {

  // If the mask is all the bits, then we can just return the value.
  if(mask == 0xFF) {
    return val;
  }

  // Otherwise if the mask is no bits then we'll assume that they want
  // all the bits ... this is only really relevant for alpha...
  if(mask == 0x0) {
    return 0xFF;
  }

  uint32 prec = CountBitsInMask(mask);
  const uint32 step = 1 << (8 - prec);

  assert(step-1 == uint8(~mask));

  uint32 lval = val & mask;
  uint32 hval = lval + step;

  if(pBit >= 0) {
    prec++;
    lval |= !!(pBit) << (8 - prec);
    hval |= !!(pBit) << (8 - prec);
  }

  if(lval > val) {
    lval -= step;
    hval -= step;
  }

  lval |= lval >> prec;
  hval |= hval >> prec;

  if(sad<uint8>(val, lval) < sad<uint8>(val, hval))
    return lval;
  else
    return hval;
}

uint32 RGBAVector::ToPixel(const uint32 channelMask, const int pBit) const {

  const uint8 pRet0 = QuantizeChannel(uint32(R() + 0.5) & 0xFF, channelMask & 0xFF, pBit);
  const uint8 pRet1 = QuantizeChannel(uint32(G() + 0.5) & 0xFF, (channelMask >> 8) & 0xFF, pBit);
  const uint8 pRet2 = QuantizeChannel(uint32(B() + 0.5) & 0xFF, (channelMask >> 16) & 0xFF, pBit);
  const uint8 pRet3 = QuantizeChannel(uint32(A() + 0.5) & 0xFF, (channelMask >> 24) & 0xFF, pBit);

  const uint32 ret = pRet0 | (pRet1 << 8) | (pRet2 << 16) | (pRet3 << 24);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//
// Cluster implementation
//
///////////////////////////////////////////////////////////////////////////////

template<typename T>
static inline T Clamp(const T &x, const T &a, const T &b) {
  return std::max(a, std::min(x, b));
}

template<const uint8 nBuckets>
double RGBACluster::QuantizedError(
  const RGBAVector &p1, const RGBAVector &p2,
  uint32 bitMask, const RGBAVector &errorMetricVec,
  const int pbits[2], uint8 *indices
) const {

  // nBuckets should be a power of two.
  const uint8 indexPrec = static_cast<uint8>(log(static_cast<float>(nBuckets))/log(2.0f));
  assert(!(nBuckets & (nBuckets - 1)));
  assert(indexPrec >= 2 && indexPrec <= 4);
  
  typedef uint32 tInterpPair[2];
  typedef tInterpPair tInterpLevel[16];
  const tInterpLevel *interpVals = BPTCC::kInterpolationValues + (indexPrec - 1);

  uint32 qp1, qp2;
  if(pbits) {
    qp1 = p1.ToPixel(bitMask, pbits[0]);
    qp2 = p2.ToPixel(bitMask, pbits[1]);
  } else {
    qp1 = p1.ToPixel(bitMask);
    qp2 = p2.ToPixel(bitMask);
  }

  const RGBAVector uqp1 = RGBAVector(0, qp1);
  const RGBAVector uqp2 = RGBAVector(0, qp2);
  const float uqplsq = (uqp1 - uqp2).LengthSq();
  const RGBAVector uqpdir = uqp2 - uqp1;

  const uint8 *pqp1 = reinterpret_cast<const uint8 *>(&qp1);
  const uint8 *pqp2 = reinterpret_cast<const uint8 *>(&qp2);

  const RGBAVector metric = errorMetricVec;

  float totalError = 0.0;
  if(uqplsq == 0) {

    // If both endpoints are the same then the indices don't matter...
    for(uint32 i = 0; i < GetNumPoints(); i++) {

      const uint32 pixel = GetPixel(i);
      const uint8 *pb = (const uint8 *)(&pixel);

      uint32 interp0 = (*interpVals)[0][0];
      uint32 interp1 = (*interpVals)[0][1];

      RGBAVector errorVec (0.0f);
      for(uint32 k = 0; k < 4; k++) {
        const uint32 ip = (((pqp1[k] * interp0) + (pqp2[k] * interp1) + 32) >> 6) & 0xFF;
        const uint8 dist = sad<uint8>(pb[k], ip);
        errorVec[k] = static_cast<float>(dist) * metric[k];
      }

      totalError += errorVec * errorVec;

      if(indices)
        indices[i] = 0;
    }

    return totalError;
  }

  for(uint32 i = 0; i < GetNumPoints(); i++) {

    // Project this point unto the direction denoted by uqpdir...
    const RGBAVector pt = GetPoint(i);
#if 0
    const float pct = Clamp(((pt - uqp1) * uqpdir) / uqplsq, 0.0f, 1.0f);
    const int32 j1 = static_cast<int32>(pct * static_cast<float>(nBuckets-1));
    const int32 j2 = static_cast<int32>(pct * static_cast<float>(nBuckets-1) + 0.7);
#else
    const float pct = ((pt - uqp1) * uqpdir) / uqplsq;
    int32 j1 = static_cast<int32>(floor(pct * static_cast<float>(nBuckets-1)));
    int32 j2 = static_cast<int32>(ceil(pct * static_cast<float>(nBuckets-1)));
    j1 = std::min(std::max(0, j1), nBuckets - 1);
    j2 = std::min(j2, nBuckets - 1);
#endif

    assert(j1 >= 0 && j2 <= nBuckets - 1);

    const uint32 pixel = GetPixel(i);
    const uint8 *pb = (const uint8 *)(&pixel);

    float minError = FLT_MAX;
    uint8 bestBucket = 0;
    int32 j = j1;
    do {

      uint32 interp0 = (*interpVals)[j][0];
      uint32 interp1 = (*interpVals)[j][1];

      RGBAVector errorVec (0.0f);
      for(uint32 k = 0; k < 4; k++) {
        const uint32 ip = (((pqp1[k] * interp0) + (pqp2[k] * interp1) + 32) >> 6) & 0xFF;
        const uint8 dist = sad<uint8>(pb[k], ip);
        errorVec[k] = static_cast<float>(dist) * metric[k];
      }

      float error = errorVec * errorVec;
      if(error < minError) {
        minError = error;
        bestBucket = j;
      }

      // Conceptually, once the error starts growing, it doesn't stop growing (we're moving
      // farther away from the reference point along the line). Hence we can early out here.
      // However, quanitzation artifacts mean that this is not ALWAYS the case, so we do suffer
      // about 0.01 RMS error. 
      else if(error > minError) {
        break;
      }
    } while(++j <= j2);

    totalError += minError;

    if(indices) indices[i] = bestBucket;
  }

  return totalError;
}

template double RGBACluster::QuantizedError<4>(
  const RGBAVector &p1, const RGBAVector &p2,
  uint32 bitMask, const RGBAVector &errorMetricVec,
  const int pbits[2], uint8 *indices) const;

template double RGBACluster::QuantizedError<8>(
  const RGBAVector &p1, const RGBAVector &p2,
  uint32 bitMask, const RGBAVector &errorMetricVec,
  const int pbits[2], uint8 *indices) const;

template double RGBACluster::QuantizedError<16>(
  const RGBAVector &p1, const RGBAVector &p2,
  uint32 bitMask, const RGBAVector &errorMetricVec,
  const int pbits[2], uint8 *indices) const;

uint32 RGBACluster::GetPrincipalAxis(RGBADir &axis, float *eigOne, float *eigTwo) const {

  // We use these vectors for calculating the covariance matrix...
  RGBAVector toPts[kMaxNumDataPoints];
  RGBAVector toPtsMax(-std::numeric_limits<float>::max());
  for(uint32 i = 0; i < this->GetNumPoints(); i++) {
    toPts[i] = this->GetPoint(i) - this->GetAvg();

    for(uint32 j = 0; j < kNumColorChannels; j++) {
      toPtsMax[j] = max(toPtsMax[j], toPts[i][j]);
    }
  }

  // Generate a list of unique points...
  RGBAVector upts[kMaxNumDataPoints];
  uint32 uptsIdx = 0;
  for(uint32 i = 0; i < this->GetNumPoints(); i++) {
    
    bool hasPt = false;
    for(uint32 j = 0; j < uptsIdx; j++) {
      if(upts[j] == this->GetPoint(i))
        hasPt = true;
    }

    if(!hasPt) {
      upts[uptsIdx++] = this->GetPoint(i);
    }
  }

  assert(uptsIdx > 0);

  if(uptsIdx == 1) {
    axis.R() = axis.G() = axis.B() = axis.A() = 0.0f;
    return 0;

  // Collinear?
  } else {
    RGBADir dir (upts[1] - upts[0]);
    bool collinear = true;
    for(uint32 i = 2; i < this->GetNumPoints(); i++) {
      RGBAVector v = (upts[i] - upts[0]);
      if(fabs(fabs(v*dir) - v.Length()) > 1e-7) {
        collinear = false;
        break;
      }
    }

    if(collinear) {
      axis = dir;
      return 0;
    }
  }

  RGBAMatrix covMatrix;

  // Compute covariance.
  for(uint32 i = 0; i < kNumColorChannels; i++) {
    for(uint32 j = 0; j <= i; j++) {

      float sum = 0.0;
      for(uint32 k = 0; k < this->GetNumPoints(); k++) {
        sum += toPts[k][i] * toPts[k][j];
      }

      covMatrix(i, j) = sum / static_cast<float>(kNumColorChannels - 1);
      covMatrix(j, i) = covMatrix(i, j);
    }
  }
  
  uint32 iters = covMatrix.PowerMethod(axis, eigOne);
  if(NULL != eigTwo && NULL != eigOne) {
    if(*eigOne != 0.0) {
      RGBAMatrix reduced;
      for(uint32 j = 0; j < 4; j++) {
        for(uint32 i = 0; i < 4; i++) {
          reduced(i, j) = axis[j] * axis[i];
        }
      }

      reduced = covMatrix - ((*eigOne) * reduced);
      bool allZero = true;
      for(uint32 i = 0; i < 16; i++) {
        if(fabs(reduced[i]) > 0.0005) {
          allZero = false;
        }
      }

      if(allZero) {
        *eigTwo = 0.0;
      }
      else {
        RGBADir dummyDir;
        iters += reduced.PowerMethod(dummyDir, eigTwo);
      }
    }
    else {
      *eigTwo = 0.0;
    }
  }

  return iters;
}

///////////////////////////////////////////////////////////////////////////////
//
// Utility function implementation
//
///////////////////////////////////////////////////////////////////////////////

void ClampEndpoints(RGBAVector &p1, RGBAVector &p2) {
  for(uint32 i = 0; i < 4; i++) {
    clamp(p1[i], 0.0f, 255.0f);
    clamp(p2[i], 0.0f, 255.0f);
  }
}
