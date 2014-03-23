/* FasTC
 * Copyright (c) 2014 University of North Carolina at Chapel Hill.
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

#ifndef BPTCENCODER_SRC_BPTCCOMPRESSIONMODE_H_
#define BPTCENCODER_SRC_BPTCCOMPRESSIONMODE_H_

#include "RGBAEndpoints.h"

namespace FasTC {
  class BitStream;
}  // namespace FasTC

namespace BPTCC {

// Forward Declarations
struct VisitedState;
const int kMaxEndpoints = 3;

static const int kPBits[4][2] = {
  { 0, 0 },
  { 0, 1 },
  { 1, 0 },
  { 1, 1 }
};

class CompressionMode {

 public:

  static const uint32 kMaxNumSubsets = 3;
  static const uint32 kNumModes = 8;

  // This initializes the compression variables used in order to compress a list
  // of clusters. We can increase the speed a tad by specifying whether or not
  // the block is opaque or not.
  explicit CompressionMode(int mode, const CompressionSettings &settings)
    : m_IsOpaque(mode < 4)
    , m_Attributes(&(kModeAttributes[mode]))
    , m_SASteps(settings.m_NumSimulatedAnnealingSteps)
    , m_ErrorMetric(settings.m_ErrorMetric)
    , m_RotateMode(0)
    , m_IndexMode(0)
  { }
  ~CompressionMode() { }

  // These are all of the parameters required to define the data in a compressed
  // BPTC block. The mode determines how these parameters will be translated
  // into actual bits.
  struct Params {
    RGBAVector m_P1[kMaxNumSubsets], m_P2[kMaxNumSubsets];
    uint8 m_Indices[kMaxNumSubsets][kMaxNumDataPoints];
    uint8 m_AlphaIndices[kMaxNumDataPoints];
    uint8 m_PbitCombo[kMaxNumSubsets];
    int8 m_RotationMode;
    int8 m_IndexMode;
    const uint16 m_ShapeIdx;
    explicit Params(uint32 shape)
      : m_RotationMode(-1), m_IndexMode(-1), m_ShapeIdx(shape) {
      memset(m_Indices, 0xFF, sizeof(m_Indices));
      memset(m_AlphaIndices, 0xFF, sizeof(m_AlphaIndices));
      memset(m_PbitCombo, 0xFF, sizeof(m_PbitCombo));
    }
  };

  // This outputs the parameters to the given bitstream based on the current
  // compression mode. The first argument is not const because the mode and
  // the value of the first index determines whether or not the indices need to
  // be swapped. The final output bits will always be a valid BPTC block. 
  void Pack(Params &params, FasTC::BitStream &stream) const;

  // This function compresses a group of clusters into the passed bitstream.
  double Compress(FasTC::BitStream &stream, const int shapeIdx,
                  RGBACluster &cluster);

  // This switch controls the quality of the simulated annealing optimizer. We
  // will not make more than this many steps regardless of how bad the error is.
  // Higher values will produce better quality results but will run slower.
  // Default is 20.
  static int MaxAnnealingIterations;               // This is a setting
  static const int kMaxAnnealingIterations = 256;  // This is a limit

  // P-bits are low-order bits that are shared across color channels. This enum
  // says whether or not both endpoints share a p-bit or whether or not they
  // even have a p-bit.
  enum EPBitType {
    ePBitType_Shared,
    ePBitType_NotShared,
    ePBitType_None
  };

  // These are all the per-mode attributes that can be set. They are specified
  // in a table and we access them through the private m_Attributes variable.
  static struct Attributes {
    int modeNumber;
    int numPartitionBits;
    int numSubsets;
    int numBitsPerIndex;
    int numBitsPerAlpha;
    int colorChannelPrecision;
    int alphaChannelPrecision;
    bool hasRotation;
    bool hasIdxMode;
    EPBitType pbitType;
  } kModeAttributes[kNumModes];

  // This returns the above attributes structure for the given mode.
  static const Attributes *GetAttributesForMode(int mode) {
    if(mode < 0 || mode >= 8) return NULL;
    return &kModeAttributes[mode];
  }

 private:

  const double m_IsOpaque;
  const Attributes *const m_Attributes;

  int m_SASteps;
  ErrorMetric m_ErrorMetric;
  int m_RotateMode;
  int m_IndexMode;

  void SetIndexMode(int mode) { m_IndexMode = mode; }
  void SetRotationMode(int mode) { m_RotateMode = mode; }

  int GetRotationMode() const {
    return m_Attributes->hasRotation? m_RotateMode : 0;
  }
  int GetModeNumber() const { return m_Attributes->modeNumber; }

  int GetNumberOfPartitionBits() const {
    return m_Attributes->numPartitionBits;
  }
  int GetNumberOfSubsets() const { return m_Attributes->numSubsets; }

  int GetNumberOfBitsPerIndex(int8 indexMode = -1) const {
    if(indexMode < 0) indexMode = m_IndexMode;
    if(indexMode == 0)
      return m_Attributes->numBitsPerIndex;
    else
      return m_Attributes->numBitsPerAlpha;
  }

  int GetNumberOfBitsPerAlpha(int8 indexMode = -1) const {
    if(indexMode < 0) indexMode = m_IndexMode;
    if(indexMode == 0)
      return m_Attributes->numBitsPerAlpha;
    else
      return m_Attributes->numBitsPerIndex;
  }

  // If we handle alpha separately, then we will consider the alpha channel
  // to be not used whenever we do any calculations...
  int GetAlphaChannelPrecision() const {
    return m_Attributes->alphaChannelPrecision;
  }

  // This returns the proper error metric even if we have rotation bits set
  RGBAVector GetErrorMetric() const {
    const float *w = BPTCC::GetErrorMetric(m_ErrorMetric);
    switch(GetRotationMode()) {
      default:
      case 0: return RGBAVector(w[0], w[1], w[2], w[3]);
      case 1: return RGBAVector(w[3], w[1], w[2], w[0]);
      case 2: return RGBAVector(w[0], w[3], w[2], w[1]);
      case 3: return RGBAVector(w[0], w[1], w[3], w[2]);
    }
  }

  EPBitType GetPBitType() const { return m_Attributes->pbitType; }

  // This function creates an integer that represents the maximum values in each
  // channel. We can use this to figure out the proper endpoint values for a
  // given mode.
  unsigned int GetQuantizationMask() const {
    const int maskSeed = 0x80000000;
    const uint32 alphaPrec = GetAlphaChannelPrecision();
    const uint32 cbits = m_Attributes->colorChannelPrecision - 1;
    const uint32 abits = GetAlphaChannelPrecision() - 1;
    if(alphaPrec > 0) {
      return (
        (maskSeed >> (24 + cbits) & 0xFF) |
        (maskSeed >> (16 + cbits) & 0xFF00) |
        (maskSeed >> (8 + cbits) & 0xFF0000) |
        (maskSeed >> abits & 0xFF000000)
      );
    } else {
      return (
        ((maskSeed >> (24 + cbits) & 0xFF) |
         (maskSeed >> (16 + cbits) & 0xFF00) |
         (maskSeed >> (8 + cbits) & 0xFF0000)) &
        (0x00FFFFFF)
      );
    }
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

  // This performs simulated annealing on the endpoints p1 and p2 based on the
  // current MaxAnnealingIterations. This is set by calling the function
  // SetQualityLevel
  double OptimizeEndpointsForCluster(
    const RGBACluster &cluster,
    RGBAVector &p1, RGBAVector &p2,
    uint8 *bestIndices,
    uint8 &bestPbitCombo
  ) const;

  // This function performs the heuristic to choose the "best" neighboring
  // endpoints to p1 and p2 based on the compression mode (index precision,
  // endpoint precision etc)
  void PickBestNeighboringEndpoints(
    const RGBACluster &cluster,
    const RGBAVector &p1, const RGBAVector &p2,
    const int curPbitCombo,
    RGBAVector &np1, RGBAVector &np2,
    int &nPbitCombo,
    const VisitedState *visitedStates,
    int nVisited,
    float stepSz = 1.0f
  ) const;

  // This is used by simulated annealing to determine whether or not the
  // newError (from the neighboring endpoints) is sufficient to continue the
  // annealing process from these new endpoints based on how good the oldError
  // was, and how long we've been annealing (t)
  bool AcceptNewEndpointError(double newError, double oldError, float t) const;

  // This function figures out the best compression for the single color p, and
  // places the endpoints in p1 and p2. If the compression mode supports p-bits,
  // then we choose the best p-bit combo and return it as well.
  double CompressSingleColor(const RGBAVector &p,
                             RGBAVector &p1, RGBAVector &p2,
                             uint8 &bestPbitCombo) const;

  // Compress the cluster using a generalized cluster fit. This figures out the
  // proper endpoints assuming that we have no alpha.
  double CompressCluster(const RGBACluster &cluster,
                         RGBAVector &p1, RGBAVector &p2,
                         uint8 *bestIndices, uint8 &bestPbitCombo) const;

  // Compress the non-opaque cluster using a generalized cluster fit, and place
  // the endpoints within p1 and p2. The color indices and alpha indices are
  // computed as well.
  double CompressCluster(const RGBACluster &cluster,
                         RGBAVector &p1, RGBAVector &p2,
                         uint8 *bestIndices, uint8 *alphaIndices) const;

  // This function takes two endpoints in the continuous domain (as floats) and
  // clamps them to the nearest grid points based on the compression mode (and
  // possible pbit values)
  void ClampEndpointsToGrid(RGBAVector &p1, RGBAVector &p2,
                            uint8 &bestPBitCombo) const;
};

extern const uint32 kInterpolationValues[4][16][2];

}  // namespace BPTCC {
#endif  // BPTCENCODER_SRC_BPTCCOMPRESSIONMODE_H_
