/* FasTC
 * Copyright (c) 2012 University of North Carolina at Chapel Hill. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its documentation for educational, 
 * research, and non-profit purposes, without fee, and without a written agreement is hereby granted, 
 * provided that the above copyright notice, this paragraph, and the following four paragraphs appear 
 * in all copies.
 *
 * Permission to incorporate this software into commercial products may be obtained by contacting the 
 * authors or the Office of Technology Development at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of North Carolina at Chapel Hill. 
 * The software program and documentation are supplied "as is," without any accompanying services from the 
 * University of North Carolina at Chapel Hill or the authors. The University of North Carolina at Chapel Hill 
 * and the authors do not warrant that the operation of the program will be uninterrupted or error-free. The 
 * end-user understands that the program was developed for research purposes and is advised not to rely 
 * exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE AUTHORS BE LIABLE TO ANY PARTY FOR 
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE 
 * USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE 
 * AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, 
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY 
 * OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
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

#include "TexCompTypes.h"
#include "Vector4.h"
#include "Matrix4x4.h"

#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstring>
#include <limits>

#include "Shapes.h"

static const uint32 kNumColorChannels = 4;
static const uint32 kMaxNumDataPoints = 16;

class RGBAVector : public FasTC::Vector4<float> {
  typedef FasTC::Vector4<float> BaseVector;
 public:
  uint32 GetIdx() const { return  m_Idx; }
  RGBAVector() : BaseVector(-1.0, -1.0, -1.0, -1.0) { }
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

  uint32 GetNumPoints() const { return m_NumPoints; }
  RGBAVector GetAvg() const { return m_Avg; }

  void GetBoundingBox(RGBAVector &Min, RGBAVector &Max) const {
    Min = m_Min, Max = m_Max;
  }

  // Returns the error if we were to quantize the colors right now with the
  // given number of buckets and bit mask.
  double QuantizedError(
    const RGBAVector &p1, const RGBAVector &p2,
    uint8 nBuckets, uint32 bitMask, const RGBAVector &errorMetricVec,
    const int pbits[2] = NULL, uint8 *indices = NULL) const;

  bool AllSamePoint() const { return m_Max == m_Min; }

  // Returns the principal axis for this point cluster.
  uint32 GetPrincipalAxis(RGBADir &axis, float *eigOne, float *eigTwo) const;

  void SetShapeIndex(uint32 shapeIdx, uint32 nPartitions) {
    m_NumPartitions = nPartitions;
    m_ShapeIdx = shapeIdx;
    // Recalculate();
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
  uint8 m_PointMap[kMaxNumDataPoints];
  RGBAVector m_Min, m_Max;

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
