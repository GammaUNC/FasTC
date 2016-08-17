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

#ifndef __RGBA_SIMD_ENDPOINTS_H__
#define __RGBA_SIMD_ENDPOINTS_H__

#include "TexCompTypes.h"

#include <cmath>
#include <cfloat>
#include <cstring>

#include <smmintrin.h>

static const int kNumColorChannels = 4;
static const int kMaxNumDataPoints = 16;
static const __m128 kEpsilonSIMD = _mm_set1_ps(1e-8f);

class RGBAVectorSIMD {

public:
  union {
    struct { float r, g, b, a; };
    struct { float x, y, z, w; };
    float c[4];
    __m128 vec;
  };

  RGBAVectorSIMD() : r(-1.0), g(-1.0), b(-1.0), a(-1.0) { }
  RGBAVectorSIMD(uint32 pixel) : 
    r(float(pixel & 0xFF)), 
    g(float((pixel >> 8) & 0xFF)), 
    b(float((pixel >> 16) & 0xFF)), 
    a(float((pixel >> 24) & 0xFF))
  { }

  explicit RGBAVectorSIMD(float _r, float _g, float _b, float _a) :
    r(_r), g(_g), b(_b), a(_a) { }

  explicit RGBAVectorSIMD(float cc) : r(cc), g(cc), b(cc), a(cc) { }

  RGBAVectorSIMD (const __m128 &newVec) : vec(newVec) { }
  RGBAVectorSIMD (const RGBAVectorSIMD &other) : vec(other.vec) { }

  RGBAVectorSIMD operator +(const RGBAVectorSIMD &p) const {
    return RGBAVectorSIMD( _mm_add_ps(this->vec, p.vec) );
  }

  RGBAVectorSIMD &operator +=(const RGBAVectorSIMD &p) {
    this->vec = _mm_add_ps(this->vec, p.vec);
    return *this;
  }

  RGBAVectorSIMD operator -(const RGBAVectorSIMD &p) const {
    return RGBAVectorSIMD( _mm_sub_ps(this->vec, p.vec) );
  }

  RGBAVectorSIMD &operator -=(const RGBAVectorSIMD &p) {
    this->vec = _mm_sub_ps(this->vec, p.vec);
    return *this;
  }

  RGBAVectorSIMD operator /(const float s) const {
    return RGBAVectorSIMD( _mm_div_ps(this->vec, _mm_set1_ps(s) ) );
  }

  RGBAVectorSIMD &operator /=(const float s) {
    this->vec = _mm_div_ps(this->vec, _mm_set1_ps(s) );
    return *this;
  }

  float operator *(const RGBAVectorSIMD &p) const {
    __m128 mul = _mm_mul_ps(this->vec, p.vec);
    mul = _mm_hadd_ps(mul, mul);
    mul = _mm_hadd_ps(mul, mul);
    return ((float *)(&mul))[0];
  }

  void Normalize() {
    __m128 rsqrt = _mm_rsqrt_ps( _mm_set1_ps( (*this) * (*this) ) );
    vec = _mm_mul_ps( vec, rsqrt );
  }

  float Length() const {
    return sqrt((*this) * (*this));
  }

  RGBAVectorSIMD &operator *=(const RGBAVectorSIMD &v) {
    this->vec = _mm_mul_ps(this->vec, v.vec);
    return *this;
  }

  RGBAVectorSIMD operator *(const float s) const {
    return RGBAVectorSIMD( _mm_mul_ps( this->vec, _mm_set1_ps(s) ) );
  }

  friend RGBAVectorSIMD operator *(const float s, const RGBAVectorSIMD &p) {
    return RGBAVectorSIMD( _mm_mul_ps( p.vec, _mm_set1_ps(s) ) );
  }

  RGBAVectorSIMD &operator *=(const float s) {
    this->vec = _mm_mul_ps( this->vec, _mm_set1_ps(s) );
    return *this;
  }

  float &operator [](const int i) {
    return c[i];
  }

  friend bool operator ==(const RGBAVectorSIMD &rhs, const RGBAVectorSIMD &lhs) {
    __m128 d = _mm_sub_ps(rhs.vec, lhs.vec);
    d = _mm_mul_ps(d, d);
    __m128 cmp = _mm_cmpgt_ps(d, kEpsilonSIMD);
    cmp = _mm_hadd_ps(cmp, cmp);
    cmp = _mm_hadd_ps(cmp, cmp);
    return ((float *)(&cmp))[0] == 0.0f;
  }

  friend bool operator !=(const RGBAVectorSIMD &rhs, const RGBAVectorSIMD &lhs) {
    return !(rhs == lhs);
  }

  operator float *() {
    return c;
  }

  // Quantize this point.
  __m128i ToPixel(const __m128i &channelMask, const int pBit) const;
  __m128i ToPixel(const __m128i &channelMask) const;
};

class RGBAMatrixSIMD {
private:
  union {
    float m[kNumColorChannels*kNumColorChannels];
    struct {
      float m1, m5, m9, m13;
      float m2, m6, m10, m14;
      float m3, m7, m11, m15;
      float m4, m8, m12, m16;
    };
    __m128 col[kNumColorChannels];
  };

  RGBAMatrixSIMD(const float *arr) {
    memcpy(m, arr, sizeof(m));
  }

  RGBAMatrixSIMD(const __m128 newcol[kNumColorChannels]) {
    for(int i = 0; i < kNumColorChannels; i++) 
      col[i] = newcol[i];
  }

public:
  
  RGBAMatrixSIMD() : 
    m1(1.0f), m2(0.0f), m3(0.0f), m4(0.0f),
    m5(0.0f), m6(1.0f), m7(0.0f), m8(0.0f),
    m9(0.0f), m10(0.0f), m11(1.0f), m12(0.0f),
    m13(0.0f), m14(0.0f), m15(0.0f), m16(1.0f)
  { }

  RGBAMatrixSIMD &operator =(const RGBAMatrixSIMD &other) {
    memcpy(m, other.m, sizeof(m));
    return (*this);
  }

  RGBAMatrixSIMD operator +(const RGBAMatrixSIMD &p) const {
    RGBAMatrixSIMD newm;
    for(int i = 0; i < kNumColorChannels; i++) {
      newm.col[i] = _mm_add_ps(col[i], p.col[i]);
    }
    return newm;
  }

  RGBAMatrixSIMD &operator +=(const RGBAMatrixSIMD &p) {
    for(int i = 0; i < kNumColorChannels; i++) {
      col[i] = _mm_add_ps( col[i], p.col[i] );
    }
    return *this;
  }

  RGBAMatrixSIMD operator -(const RGBAMatrixSIMD &p) const {
    RGBAMatrixSIMD newm;
    for(int i = 0; i < kNumColorChannels; i++) {
      newm.col[i] = _mm_sub_ps( col[i], p.col[i] );
    }
    return newm;
  }

  RGBAMatrixSIMD &operator -=(const RGBAMatrixSIMD &p) {
    for(int i = 0; i < kNumColorChannels; i++) {
      col[i] = _mm_sub_ps( col[i], p.col[i] );
    }
    return *this;
  }

  RGBAMatrixSIMD operator /(const float s) const {
    __m128 f = _mm_set1_ps(s);
    RGBAMatrixSIMD newm;

    for(int i = 0; i < kNumColorChannels; i++) {
      newm.col[i] = _mm_div_ps( col[i], f );
    }

    return newm;
  }

  RGBAMatrixSIMD &operator /=(const float s) {

    __m128 f = _mm_set1_ps(s);

    for(int i = 0; i < kNumColorChannels; i++) {
      col[i] = _mm_div_ps(col[i], f);
    }

    return *this;
  }

  RGBAMatrixSIMD operator *(const float s) const {
    __m128 f = _mm_set1_ps(s);

    RGBAMatrixSIMD newm;
    for(int i = 0; i < kNumColorChannels; i++) {
      newm.col[i] = _mm_mul_ps( col[i], f );
    }
    return newm;
  }

  friend RGBAMatrixSIMD operator *(const float s, const RGBAMatrixSIMD &p) {
    __m128 f = _mm_set1_ps(s);
    RGBAMatrixSIMD newm;

    for(int i = 0; i < kNumColorChannels; i++) {
      newm.col[i] = _mm_mul_ps( p.col[i], f );
    }
    return newm;
  }

  RGBAMatrixSIMD &operator *=(const float s) {
    __m128 f = _mm_set1_ps(s);
    for(int i = 0; i < kNumColorChannels; i++) 
      col[i] = _mm_mul_ps(col[i], f);
    return *this;
  }

  float &operator ()(const int i, const int j) {
    return (*this)[j*4 + i];
  }

  float &operator [](const int i) {
    return m[i];
  }

  friend bool operator ==(const RGBAMatrixSIMD &rhs, const RGBAMatrixSIMD &lhs) {
    
    __m128 sum = _mm_set1_ps(0.0f);
    for(int i = 0; i < kNumColorChannels; i++) {
      __m128 d = _mm_sub_ps(rhs.col[i], lhs.col[i]);
      d = _mm_mul_ps(d, d);
      __m128 cmp = _mm_cmpgt_ps(d, kEpsilonSIMD);
      cmp = _mm_hadd_ps(cmp, cmp);
      cmp = _mm_hadd_ps(cmp, cmp);
      sum = _mm_add_ps(sum, cmp);
    }

    if(((float *)(&sum))[0] != 0)
      return false;
    else
      return true;
  }

  operator float *() {
    return m;
  }

  RGBAVectorSIMD operator *(const RGBAVectorSIMD &p) const;
};

class RGBADirSIMD : public RGBAVectorSIMD {
public:
  RGBADirSIMD() : RGBAVectorSIMD() { }
  RGBADirSIMD(const RGBAVectorSIMD &p) : RGBAVectorSIMD(p) {
    this->Normalize();
  }
};

// Makes sure that the values of the endpoints lie between 0 and 1.
extern void ClampEndpoints(RGBAVectorSIMD &p1, RGBAVectorSIMD &p2);

class RGBAClusterSIMD {
public:

  RGBAClusterSIMD() : 
    m_NumPoints(0), m_Total(0.0f), 
    m_PointBitString(0),
    m_Min(FLT_MAX),
    m_Max(-FLT_MAX),
    m_PrincipalAxisCached(false)
  { } 

  RGBAClusterSIMD(const RGBAClusterSIMD &c) : 
    m_NumPoints(c.m_NumPoints),
    m_Total(c.m_Total),
    m_PointBitString(c.m_PointBitString), 
    m_Min(c.m_Min),
    m_Max(c.m_Max),
    m_PrincipalAxisCached(false)
  { 
    memcpy(this->m_DataPoints, c.m_DataPoints, m_NumPoints * sizeof(RGBAVectorSIMD));
  }

  RGBAClusterSIMD(const RGBAClusterSIMD &left, const RGBAClusterSIMD &right);
  RGBAClusterSIMD(const RGBAVectorSIMD &p, int idx) : 
    m_NumPoints(1),
    m_Total(p),
    m_PointBitString(0),
    m_Min(p), m_Max(p),
    m_PrincipalAxisCached(false)
  { 
    m_DataPoints[0] = p;
    m_PointBitString |= (1 << idx);
  }
      
  RGBAVectorSIMD GetTotal() const { return m_Total; }
  const RGBAVectorSIMD &GetPoint(int idx) const { return m_DataPoints[idx]; }
  int GetNumPoints() const { return m_NumPoints; }
  RGBAVectorSIMD GetAvg() const { return m_Total / float(m_NumPoints); }

  void AddPoint(const RGBAVectorSIMD &p, int idx);

  void GetBoundingBox(RGBAVectorSIMD &Min, RGBAVectorSIMD &Max) const {
    Min = m_Min, Max = m_Max;
  }

  // Returns the error if we were to quantize the colors right now with the given number of buckets and bit mask.
  float QuantizedError(const RGBAVectorSIMD &p1, const RGBAVectorSIMD &p2, const uint8 nBuckets, const __m128i &bitMask, const int pbits[2] = NULL, __m128i *indices = NULL) const;

  bool AllSamePoint() const { return m_Max == m_Min; }
  int GetPointBitString() const { return m_PointBitString; }

private:

  // The number of points in the cluster.
  int m_NumPoints;

  RGBAVectorSIMD m_Total;

  // The points in the cluster.
  RGBAVectorSIMD m_DataPoints[kMaxNumDataPoints];

  RGBAVectorSIMD m_Min, m_Max;
  int m_PointBitString;

  RGBADirSIMD m_PrincipalAxis;
  bool m_PrincipalAxisCached;
};

extern void GetPrincipalAxis(const RGBAClusterSIMD &c, RGBADirSIMD &axis);

#endif //__RGBA_SIMD_ENDPOINTS_H__
