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

#ifndef BPTCENCODER_SRC_BC7COMPRESSIONMODESIMD_H_
#define BPTCENCODER_SRC_BC7COMPRESSIONMODESIMD_H_

#include "TexCompTypes.h"
#include "RGBAEndpointsSIMD.h"

// Forward Declarations
class BitStream;

static const int kPBits[4][2] = {
  { 0, 0 },
  { 0, 1 },
  { 1, 0 },
  { 1, 1 }
};

class BC7CompressionModeSIMD {
 public:

  static const int kMaxNumSubsets = 3;
  static const int kNumModes = 8;

  enum EPBitType {
    ePBitType_Shared,
    ePBitType_NotShared,
    ePBitType_None
  };

  BC7CompressionModeSIMD(int mode, double err)
    : m_EstimatedError(err)
    , m_Attributes(&(kModeAttributes[mode]))
  { }
  ~BC7CompressionModeSIMD() { }

  static int NumUses[8];
  static void ResetNumUses() { memset(NumUses, 0, sizeof(NumUses)); }

  double Compress(BitStream &stream, const int shapeIdx,
                  const RGBAClusterSIMD *clusters) const;

  // This switch controls the quality of the simulated annealing optimizer. We
  // will not make more than this many steps regardless of how bad the error is.
  // Higher values will produce better quality results but will run slower.
  // Default is 50.
  static int MaxAnnealingIterations;  // This is a setting

 private:

  static struct Attributes {
    int modeNumber;
    int numPartitionBits;
    int numSubsets;
    int numBitsPerIndex;
    int redChannelPrecision;
    int greenChannelPrecision;
    int blueChannelPrecision;
    int alphaChannelPrecision;
    EPBitType pbitType;
  } kModeAttributes[kNumModes];

 protected:
  const Attributes *const m_Attributes;

  int GetModeNumber() const { return m_Attributes->modeNumber; }
  int GetNumberOfPartitionBits() const {
    return m_Attributes->numPartitionBits;
  }
  int GetNumberOfSubsets() const { return m_Attributes->numSubsets; }
  int GetNumberOfBitsPerIndex() const { return m_Attributes->numBitsPerIndex; }

  int GetRedChannelPrecision() const {
    return m_Attributes->redChannelPrecision;
  }

  int GetGreenChannelPrecision() const {
    return m_Attributes->greenChannelPrecision;
  }

  int GetBlueChannelPrecision() const {
    return m_Attributes->blueChannelPrecision;
  }

  int GetAlphaChannelPrecision() const {
    return m_Attributes->alphaChannelPrecision;
  }

  EPBitType GetPBitType() const { return m_Attributes->pbitType; }

  // !SPEED! Add this to the attributes lookup table
  void GetQuantizationMask(__m128i &mask) const {
    const int maskSeed = 0x80000000;
    const uint32 abits = 24 + GetAlphaChannelPrecision() - 1;
    const uint32 rbits = 24 + GetRedChannelPrecision() - 1;
    const uint32 gbits = 24 + GetGreenChannelPrecision() - 1;
    const uint32 bbits = 24 + GetBlueChannelPrecision() - 1;
    mask = _mm_set_epi32(
      (GetAlphaChannelPrecision() > 0)? (maskSeed >> abits & 0xFF) : 0xFF,
      (maskSeed >> rbits & 0xFF),
      (maskSeed >> gbits & 0xFF),
      (maskSeed >> bbits & 0xFF)
    );
  }

  int GetNumPbitCombos() const {
    switch(GetPBitType()) {
      case ePBitType_Shared: return 2;
      case ePBitType_NotShared: return 4;
      default:
      case ePBitType_None: return 1;
    }
  }

  const int *GetPBitCombo(int idx) const {
    switch(GetPBitType()) {
      case ePBitType_Shared: return (idx)? kPBits[3] : kPBits[0];
      case ePBitType_NotShared: return kPBits[idx % 4];
      default:
      case ePBitType_None: return kPBits[0];
    }
  }

  double OptimizeEndpointsForCluster(const RGBAClusterSIMD &cluster,
                                     RGBAVectorSIMD &p1, RGBAVectorSIMD &p2,
                                     __m128i *bestIndices,
                                     int &bestPbitCombo) const;

  struct VisitedState {
    RGBAVectorSIMD p1;
    RGBAVectorSIMD p2;
    int pBitCombo;
  };

  void PickBestNeighboringEndpoints(
    const RGBAClusterSIMD &cluster,
    const RGBAVectorSIMD &p1, const RGBAVectorSIMD &p2,
    const int curPbitCombo,
    RGBAVectorSIMD &np1, RGBAVectorSIMD &np2,
    int &nPbitCombo,
    const __m128 &stepVec
  ) const;

  bool AcceptNewEndpointError(float newError, float oldError, float temp) const;

  double CompressSingleColor(const RGBAVectorSIMD &p,
                             RGBAVectorSIMD &p1, RGBAVectorSIMD &p2,
                             int &bestPbitCombo) const;
  double CompressCluster(const RGBAClusterSIMD &cluster,
                         RGBAVectorSIMD &p1, RGBAVectorSIMD &p2,
                         __m128i *bestIndices,
                         int &bestPbitCombo) const;

  void ClampEndpointsToGrid(RGBAVectorSIMD &p1, RGBAVectorSIMD &p2,
                            int &bestPBitCombo) const;

  int GetSubsetForIndex(int idx, const int shapeIdx) const;
  int GetAnchorIndexForSubset(int subset, const int shapeIdx) const;

  double GetEstimatedError() const { return m_EstimatedError; }
  const double m_EstimatedError;
};

extern const __m128i kBC7InterpolationValuesSIMD[4][16][2];
extern const uint32 kBC7InterpolationValuesScalar[4][16][2];

#endif  // BPTCENCODER_SRC_BC7COMPRESSIONMODESIMD_H_
