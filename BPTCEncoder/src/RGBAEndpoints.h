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

#include <cmath>
#include <cfloat>
#include <cstring>

static const uint32 kNumColorChannels = 4;
static const uint32 kMaxNumDataPoints = 16;

class RGBAVector {

public:
	union {
		struct { float r, g, b, a; };
		struct { float x, y, z, w; };
		float c[4];
	};

	uint32 GetIdx() const { return  idx; }

	RGBAVector() : r(-1.0), g(-1.0), b(-1.0), a(-1.0) { }
	RGBAVector(uint32 _idx, uint32 pixel) : 
		r(float(pixel & 0xFF)), 
		g(float((pixel >> 8) & 0xFF)), 
		b(float((pixel >> 16) & 0xFF)), 
		a(float((pixel >> 24) & 0xFF)),
		idx(_idx)
	{ }

	RGBAVector(float _r, float _g, float _b, float _a) :
		r(_r), g(_g), b(_b), a(_a) { }

	explicit RGBAVector(float cc) : r(cc), g(cc), b(cc), a(cc) { }

	RGBAVector &operator =(const RGBAVector &other) {
		this->idx = other.idx;
		memcpy(c, other.c, sizeof(c));
		return (*this);
	}

	RGBAVector operator +(const RGBAVector &p) const {
		return RGBAVector(r + p.r, g + p.g, b + p.b, a + p.a);
	}

	RGBAVector &operator +=(const RGBAVector &p) {
		r += p.r; g += p.g; b += p.b; a += p.a;
		return *this;
	}

	RGBAVector operator -(const RGBAVector &p) const {
		return RGBAVector(r - p.r, g - p.g, b - p.b, a - p.a);
	}

	RGBAVector &operator -=(const RGBAVector &p) {
		r -= p.r; g -= p.g; b -= p.b; a -= p.a;
		return *this;
	}

	RGBAVector operator /(const float s) const {
		return RGBAVector(r / s, g / s, b / s, a / s);
	}

	RGBAVector &operator /=(const float s) {
		r /= s; g /= s; b /= s; a /= s;
		return *this;
	}

	float operator *(const RGBAVector &p) const {
		return r * p.r + g * p.g + b * p.b + a * p.a;
	}

	float Length() const {
		return sqrt((*this) * (*this));
	}

	RGBAVector &operator *=(const RGBAVector &v) {
		r *= v.r; g *= v.g; b *= v.b; a *= v.a;
		return *this;
	}

	RGBAVector operator *(const float s) const {
		return RGBAVector(r * s, g * s, b * s, a * s);
	}

	friend RGBAVector operator *(const float s, const RGBAVector &p) {
		return RGBAVector(p.r * s, p.g * s, p.b * s, p.a * s);
	}

	RGBAVector &operator *=(const float s) {
		r *= s; g *= s; b *= s; a *= s;
		return *this;
	}

	float &operator [](const int i) {
		return c[i];
	}

	friend bool operator ==(const RGBAVector &rhs, const RGBAVector &lhs) {
		const RGBAVector d = rhs - lhs;
		return fabs(d.r) < 1e-7 && fabs(d.g) < 1e-7 && fabs(d.b) < 1e-7 && fabs(d.a) < 1e-7;
	}

	friend bool operator !=(const RGBAVector &rhs, const RGBAVector &lhs) {
		return !(rhs == lhs);
	}

	operator float *() {
		return c;
	}

	RGBAVector Cross(const RGBAVector &rhs) {
		return RGBAVector(
			rhs.y * z - y * rhs.z,
			rhs.z * x - z * rhs.x,
			rhs.x * y - x * rhs.y,
			1.0f
		);
	}

	// Quantize this point.
	uint32 ToPixel(const uint32 channelMask = 0xFFFFFFFF, const int pBit = -1) const;

private:
	uint32 idx;
};

class RGBAMatrix {
private:
	union {
		float m[kNumColorChannels*kNumColorChannels];
		struct {
			float m1, m2, m3, m4;
			float m5, m6, m7, m8;
			float m9, m10, m11, m12;
			float m13, m14, m15, m16;
		};
	};

	RGBAMatrix(const float *arr) {
		memcpy(m, arr, sizeof(m));
	}

public:

	RGBAMatrix(
	  float _m1, float _m2, float _m3, float _m4,
	  float _m5, float _m6, float _m7, float _m8,
	  float _m9, float _m10, float _m11, float _m12,
	  float _m13, float _m14, float _m15, float _m16
        ) :
	  m1(_m1), m2(_m2), m3(_m3), m4(_m4),
	  m5(_m5), m6(_m6), m7(_m7), m8(_m8),
	  m9(_m9), m10(_m10), m11(_m11), m12(_m12),
	  m13(_m13), m14(_m14), m15(_m15), m16(_m16)
	{ }
	
	RGBAMatrix() : 
		m1(1.0f), m2(0.0f), m3(0.0f), m4(0.0f),
		m5(0.0f), m6(1.0f), m7(0.0f), m8(0.0f),
		m9(0.0f), m10(0.0f), m11(1.0f), m12(0.0f),
		m13(0.0f), m14(0.0f), m15(0.0f), m16(1.0f)
	{ }

	RGBAMatrix &operator =(const RGBAMatrix &other) {
		memcpy(m, other.m, sizeof(m));
		return (*this);
	}

	RGBAMatrix operator +(const RGBAMatrix &p) const {
		float newm[kNumColorChannels*kNumColorChannels];
		for(uint32 i = 0; i < kNumColorChannels*kNumColorChannels; i++) newm[i] = m[i] + p.m[i];
		return RGBAMatrix(newm);
	}

	RGBAMatrix &operator +=(const RGBAMatrix &p) {
		for(uint32 i = 0; i < kNumColorChannels*kNumColorChannels; i++) m[i] += p.m[i];
		return *this;
	}

	RGBAMatrix operator -(const RGBAMatrix &p) const {
		float newm[kNumColorChannels*kNumColorChannels];
		for(uint32 i = 0; i < kNumColorChannels*kNumColorChannels; i++) newm[i] = m[i] - p.m[i];
		return RGBAMatrix(newm);
	}

	RGBAMatrix &operator -=(const RGBAMatrix &p) {
		for(uint32 i = 0; i < kNumColorChannels*kNumColorChannels; i++) m[i] -= p.m[i];
		return *this;
	}

	RGBAMatrix operator /(const float s) const {
		float newm[kNumColorChannels*kNumColorChannels];
		for(uint32 i = 0; i < kNumColorChannels*kNumColorChannels; i++) newm[i] = m[i] / s;
		return RGBAMatrix(newm);
	}

	RGBAMatrix &operator /=(const float s) {
		for(uint32 i = 0; i < kNumColorChannels*kNumColorChannels; i++) m[i] /= s;
		return *this;
	}

	RGBAMatrix operator *(const float s) const {
		float newm[kNumColorChannels*kNumColorChannels];
		for(uint32 i = 0; i < kNumColorChannels*kNumColorChannels; i++) newm[i] = m[i] * s;
		return RGBAMatrix(newm);
	}

	RGBAMatrix operator *(const double s) const {
		float newm[kNumColorChannels*kNumColorChannels];
		for(uint32 i = 0; i < kNumColorChannels*kNumColorChannels; i++) newm[i] = float(double(m[i]) * s);
		return RGBAMatrix(newm);
	}

	friend RGBAMatrix operator *(const float s, const RGBAMatrix &p) {
		float newm[kNumColorChannels*kNumColorChannels];
		for(uint32 i = 0; i < kNumColorChannels*kNumColorChannels; i++) newm[i] = p.m[i] * s;
		return RGBAMatrix(newm);	
	}

	friend RGBAMatrix operator *(const double s, const RGBAMatrix &p) {
		float newm[kNumColorChannels*kNumColorChannels];
		for(uint32 i = 0; i < kNumColorChannels*kNumColorChannels; i++) newm[i] = float(double(p.m[i]) * s);
		return RGBAMatrix(newm);	
	}

	RGBAMatrix &operator *=(const float s) {
		for(uint32 i = 0; i < kNumColorChannels*kNumColorChannels; i++) m[i] *= s;
		return *this;
	}

	float &operator ()(const int i, const int j) {
		return (*this)[i*4 + j];
	}

	float &operator [](const int i) {
		return m[i];
	}

	friend bool operator ==(const RGBAMatrix &rhs, const RGBAMatrix &lhs) {
		const RGBAMatrix d = rhs - lhs;
		for(uint32 i = 0; i < kNumColorChannels*kNumColorChannels; i++)
			if(d.m[i] > 1e-10)
				return false;
		return true;
	}

	operator float *() {
		return m;
	}

	RGBAVector operator *(const RGBAVector &p) const;
	RGBAMatrix operator *(const RGBAMatrix &mat) const;
	RGBAMatrix &operator *=(const RGBAMatrix &mat);
	static RGBAMatrix RotateX(float rad);
	static RGBAMatrix RotateY(float rad);
	static RGBAMatrix RotateZ(float rad);
	static RGBAMatrix Translate(const RGBAVector &t);
	bool Identity();
};

class RGBADir : public RGBAVector {
public:
	RGBADir() : RGBAVector() { }
	RGBADir(const RGBAVector &p) : RGBAVector(p) {
		*this /= Length();
	}
};

// Makes sure that the values of the endpoints lie between 0 and 1.
extern void ClampEndpoints(RGBAVector &p1, RGBAVector &p2);

class RGBACluster {
public:

	RGBACluster() : 
	  m_NumPoints(0), m_Total(0), 
	  m_PointBitString(0),
	  m_Min(FLT_MAX),
	  m_Max(-FLT_MAX),
	  m_PrincipalAxisCached(false)
	{ } 

	RGBACluster(const RGBACluster &c) : 
		m_NumPoints(c.m_NumPoints),
		m_Total(c.m_Total),
		m_PointBitString(c.m_PointBitString), 
		m_Min(c.m_Min),
		m_Max(c.m_Max),
		m_PrincipalAxisCached(c.m_PrincipalAxisCached),
		m_PrincipalEigenvalue(c.m_PrincipalEigenvalue),
		m_SecondEigenvalue(c.m_SecondEigenvalue),
		m_PowerMethodIterations(c.m_PowerMethodIterations),
		m_PrincipalAxis(c.m_PrincipalAxis)
	{ 
		memcpy(this->m_DataPoints, c.m_DataPoints, m_NumPoints * sizeof(RGBAVector));
	}

	RGBACluster(const RGBACluster &left, const RGBACluster &right);
	RGBACluster(const RGBAVector &p) : 
		m_NumPoints(1),
		m_Total(p),
		m_PointBitString(0),
		m_Min(p), m_Max(p),
		m_PrincipalAxisCached(false)
	{ 
		m_DataPoints[0] = p;
		m_PointBitString |= (1 << p.GetIdx());
	}
			
	RGBAVector GetTotal() const { return m_Total; }
	const RGBAVector &GetPoint(int idx) const { return m_DataPoints[idx]; }
	int GetNumPoints() const { return m_NumPoints; }
	RGBAVector GetAvg() const { return m_Total / float(m_NumPoints); }
	const RGBAVector *GetPoints() const { return m_DataPoints; }

	void AddPoint(const RGBAVector &p);

	void GetBoundingBox(RGBAVector &Min, RGBAVector &Max) const {
		Min = m_Min, Max = m_Max;
	}

	// Returns the error if we were to quantize the colors right now with the given number of buckets and bit mask.
	double QuantizedError(const RGBAVector &p1, const RGBAVector &p2, uint8 nBuckets, uint32 bitMask, const RGBAVector &errorMetricVec, const int pbits[2] = NULL, int *indices = NULL) const;

	// Returns the principal axis for this point cluster.
	double GetPrincipalEigenvalue();
	double GetSecondEigenvalue();
	uint32 GetPowerMethodIterations();
	void GetPrincipalAxis(RGBADir &axis);

	bool AllSamePoint() const { return m_Max == m_Min; }
	int GetPointBitString() const { return m_PointBitString; }

private:

	// The number of points in the cluster.
	int m_NumPoints;

	RGBAVector m_Total;

	// The points in the cluster.
	RGBAVector m_DataPoints[kMaxNumDataPoints];

	int m_PointBitString;
	RGBAVector m_Min, m_Max;

	bool m_PrincipalAxisCached;
	double m_PrincipalEigenvalue;
	double m_SecondEigenvalue;
	uint32 m_PowerMethodIterations;
	RGBADir m_PrincipalAxis;
};

extern uint8 QuantizeChannel(const uint8 val, const uint8 mask, const int pBit = -1);
extern uint32 GetPrincipalAxis(int nPts, const RGBAVector *pts, RGBADir &axis, double &eigOne, double *eigTwo);

#endif //__RGBA_ENDPOINTS_H__
