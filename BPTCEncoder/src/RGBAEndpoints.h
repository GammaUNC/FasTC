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

#ifndef __RGBA_ENDPOINTS_H__
#define __RGBA_ENDPOINTS_H__

#include "FasTC/TexCompTypes.h"
#include "FasTC/Vector4.h"
#include "FasTC/Matrix4x4.h"

#include "FasTC/Shapes.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cfloat>
#include <cstring>
#include <limits>

static const uint32 kNumColorChannels = 4;
static const uint32 kMaxNumDataPoints = 16;

class RGBAVector : public FasTC::Vector4<float> {
  typedef FasTC::Vector4<float> BaseVector;
 public:
  uint32 GetIdx() const { return  m_Idx; }
  RGBAVector() : BaseVector(-1.0, -1.0, -1.0, -1.0), m_Idx(0) { }
  RGBAVector(uint32 idx, uint32 pixel) : 
   BaseVector(
    static_cast<float>(pixel & 0xFF),
    static_cast<float>((pixel >> 8) & 0xFF),
    static_cast<float>((pixel >> 16) & 0xFF),
    static_cast<float>((pixel >> 24) & 0xFF)
   )
   , m_Idx(idx)
  { }

  RGBAVector(float _r, float _g, float _b, float _a)
  : BaseVector(_r, _g, _b, _a), m_Idx(0) { }

  explicit RGBAVector(float cc) : BaseVector(cc, cc, cc, cc), m_Idx(0) { }

  const float &R() const { return vec[0]; }
  float &R() { return vec[0]; }
  const float &G() const { return vec[1]; }
  float &G() { return vec[1]; }
  const float &B() const { return vec[2]; }
  float &B() { return vec[2]; }
  const float &A() const { return vec[3]; }
  float &A() { return vec[3]; }

  // Quantize this point.
  uint32 ToPixel(const uint32 channelMask = 0xFFFFFFFF, const int pBit = -1) const;

private:
  uint32 m_Idx;
};
typedef FasTC::Matrix4x4<float> RGBAMatrix;

class RGBADir : public RGBAVector {
 public:
  RGBADir() : RGBAVector() { }
  RGBADir(const RGBAVector &p) : RGBAVector(p) {
    this->Normalize();
  }
};

class RGBACluster {
  // We really don't ever need to do these
  RGBACluster &operator=(const RGBACluster &) { return *this; }
public:
  explicit RGBACluster(const uint32 pixels[16])
    : m_NumPoints(0)
    , m_Avg(0)
    , m_Min(std::numeric_limits<float>::max())
    , m_Max(-std::numeric_limits<float>::max())
  {
    for(uint32 i = 0; i < 16; i++) {
      RGBAVector p = RGBAVector(i, pixels[i]);
      m_Avg += p;
      m_PointMap[m_NumPoints] = i;
      m_DataPixels[m_NumPoints] = p.ToPixel();
      m_DataPoints[m_NumPoints++] = p;

      for(uint32 i = 0; i < kNumColorChannels; i++) {
        m_Min[i] = std::min(p[i], m_Min[i]);
        m_Max[i] = std::max(p[i], m_Max[i]);
      }
    }
    m_Avg /= static_cast<float>(m_NumPoints);
  }

  RGBAVector &Point(int idx) { return m_DataPoints[m_PointMap[idx]]; }
  const RGBAVector &GetPoint(int idx) const {
    return m_DataPoints[m_PointMap[idx]];
  }

  const uint32 &GetPixel(int idx) const {
    return m_DataPixels[m_PointMap[idx]];
  }

  uint32 GetNumPoints() const { return m_NumPoints; }
  RGBAVector GetAvg() const { return m_Avg; }

  void GetBoundingBox(RGBAVector &Min, RGBAVector &Max) const {
    Min = m_Min, Max = m_Max;
  }

  // Returns the error if we were to quantize the colors right now with the
  // given number of buckets and bit mask.
  double QuantizedError(
    const RGBAVector &p1, const RGBAVector &p2,
    uint32 nBuckets, uint32 bitMask, const RGBAVector &errorMetricVec,
    const int pbits[2] = NULL, uint8 *indices = NULL) const {
    switch(nBuckets) {
      case 4: return QuantizedError<4>(p1, p2, bitMask, errorMetricVec, pbits, indices);
      case 8: return QuantizedError<8>(p1, p2, bitMask, errorMetricVec, pbits, indices);
      case 16: return QuantizedError<16>(p1, p2, bitMask, errorMetricVec, pbits, indices);
    }
    assert(!"Unsupported num buckets");
    return std::numeric_limits<double>::max();
  }

  bool AllSamePoint() const { return m_Max == m_Min; }

  // Returns the principal axis for this point cluster.
  uint32 GetPrincipalAxis(RGBADir &axis, float *eigOne, float *eigTwo) const;

  void SetShapeIndex(uint32 shapeIdx, uint32 nPartitions) {
    m_NumPartitions = nPartitions;
    m_ShapeIdx = shapeIdx;
  }

  void SetShapeIndex(uint32 shapeIdx) {
    SetShapeIndex(shapeIdx, m_NumPartitions);
  }

  void SetPartition(uint32 part) {
    m_SelectedPartition = part;
    Recalculate();
  }

  bool IsPointValid(uint32 idx) const {
    return m_SelectedPartition ==
      BPTCC::GetSubsetForIndex(idx, m_ShapeIdx, m_NumPartitions);
  }

 private:
  // The number of points in the cluster.
  uint32 m_NumPoints;
  uint32 m_NumPartitions;
  uint32 m_SelectedPartition;
  uint32 m_ShapeIdx;

  RGBAVector m_Avg;

  // The points in the cluster.
  RGBAVector m_DataPoints[kMaxNumDataPoints];
  uint32 m_DataPixels[kMaxNumDataPoints];
  uint8 m_PointMap[kMaxNumDataPoints];
  RGBAVector m_Min, m_Max;

  template<const uint8 nBuckets>
  double QuantizedError(
    const RGBAVector &p1, const RGBAVector &p2,
    uint32 bitMask, const RGBAVector &errorMetricVec,
    const int pbits[2] = NULL, uint8 *indices = NULL) const;

  void Recalculate() {
    m_NumPoints = 0;
    m_Avg = RGBAVector(0.0f);
    m_Min = RGBAVector(std::numeric_limits<float>::max());
    m_Max = RGBAVector(-std::numeric_limits<float>::max());

    uint32 map = 0;
    for(uint32 idx = 0; idx < 16; idx++) {
      if(!IsPointValid(idx)) continue;

      m_NumPoints++;
      m_Avg += m_DataPoints[idx];
      m_PointMap[map++] = idx;

      for(uint32 i = 0; i < kNumColorChannels; i++) {
        m_Min[i] = std::min(m_DataPoints[idx][i], m_Min[i]);
        m_Max[i] = std::max(m_DataPoints[idx][i], m_Max[i]);
      }
    }

    m_Avg /= static_cast<float>(m_NumPoints);
  }
};

// Makes sure that the values of the endpoints lie between 0 and 1.
extern void ClampEndpoints(RGBAVector &p1, RGBAVector &p2);
extern uint8 QuantizeChannel(const uint8 val, const uint8 mask, const int pBit = -1);

namespace FasTC {
  REGISTER_VECTOR_TYPE(RGBAVector);
  REGISTER_VECTOR_TYPE(RGBADir);
}

#endif //__RGBA_ENDPOINTS_H__
