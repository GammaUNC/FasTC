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

#include "BC7Config.h"
#include "RGBAEndpoints.h"
#include "BC7Compressor.h"
#include "BC7CompressionMode.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cfloat>

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

static const double kPi = 3.141592653589793238462643383279502884197;
static const float kFloatConversion[256] = {
	0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 
	16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 
	32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f, 
	48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f, 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f, 
	64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 72.0f, 73.0f, 74.0f, 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 
	80.0f, 81.0f, 82.0f, 83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f, 91.0f, 92.0f, 93.0f, 94.0f, 95.0f, 
	96.0f, 97.0f, 98.0f, 99.0f, 100.0f, 101.0f, 102.0f, 103.0f, 104.0f, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f, 111.0f, 
	112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f, 121.0f, 122.0f, 123.0f, 124.0f, 125.0f, 126.0f, 127.0f, 
	128.0f, 129.0f, 130.0f, 131.0f, 132.0f, 133.0f, 134.0f, 135.0f, 136.0f, 137.0f, 138.0f, 139.0f, 140.0f, 141.0f, 142.0f, 143.0f, 
	144.0f, 145.0f, 146.0f, 147.0f, 148.0f, 149.0f, 150.0f, 151.0f, 152.0f, 153.0f, 154.0f, 155.0f, 156.0f, 157.0f, 158.0f, 159.0f, 
	160.0f, 161.0f, 162.0f, 163.0f, 164.0f, 165.0f, 166.0f, 167.0f, 168.0f, 169.0f, 170.0f, 171.0f, 172.0f, 173.0f, 174.0f, 175.0f, 
	176.0f, 177.0f, 178.0f, 179.0f, 180.0f, 181.0f, 182.0f, 183.0f, 184.0f, 185.0f, 186.0f, 187.0f, 188.0f, 189.0f, 190.0f, 191.0f, 
	192.0f, 193.0f, 194.0f, 195.0f, 196.0f, 197.0f, 198.0f, 199.0f, 200.0f, 201.0f, 202.0f, 203.0f, 204.0f, 205.0f, 206.0f, 207.0f, 
	208.0f, 209.0f, 210.0f, 211.0f, 212.0f, 213.0f, 214.0f, 215.0f, 216.0f, 217.0f, 218.0f, 219.0f, 220.0f, 221.0f, 222.0f, 223.0f, 
	224.0f, 225.0f, 226.0f, 227.0f, 228.0f, 229.0f, 230.0f, 231.0f, 232.0f, 233.0f, 234.0f, 235.0f, 236.0f, 237.0f, 238.0f, 239.0f, 
	240.0f, 241.0f, 242.0f, 243.0f, 244.0f, 245.0f, 246.0f, 247.0f, 248.0f, 249.0f, 250.0f, 251.0f, 252.0f, 253.0f, 254.0f, 255.0f
};

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
static uint8 sad(uint8 a, uint8 b) {
#if 0
	__asm
	{
		movzx eax, a
		movzx ecx, b
		sub eax, ecx
		jns done
		neg eax
done:
	}
#else
	//const INT d = a - b;
	//const INT mask = d >> 31;
	//return (d ^ mask) - mask;

	// return abs(a - b);

	return (a > b)? a - b : b - a;

#endif
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

	if(sad(val, lval) < sad(val, hval))
		return lval;
	else
		return hval;
}

uint32 RGBAVector::ToPixel(const uint32 channelMask, const int pBit) const {

	const uint8 pRet0 = QuantizeChannel(uint32(r + 0.5) & 0xFF, channelMask & 0xFF, pBit);
	const uint8 pRet1 = QuantizeChannel(uint32(g + 0.5) & 0xFF, (channelMask >> 8) & 0xFF, pBit);
	const uint8 pRet2 = QuantizeChannel(uint32(b + 0.5) & 0xFF, (channelMask >> 16) & 0xFF, pBit);
	const uint8 pRet3 = QuantizeChannel(uint32(a + 0.5) & 0xFF, (channelMask >> 24) & 0xFF, pBit);

	const uint32 ret = pRet0 | (pRet1 << 8) | (pRet2 << 16) | (pRet3 << 24);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
//
// RGBAMatrix implementation
//
///////////////////////////////////////////////////////////////////////////////

RGBAMatrix &RGBAMatrix::operator *=(const RGBAMatrix &mat) {
	*this = ((*this) * mat);
	return (*this);
}

RGBAMatrix RGBAMatrix::operator *(const RGBAMatrix &mat) const {

	RGBAMatrix result;

	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {

			result(i, j) = 0.0f;
			for(int k = 0; k < 4; k++) {
				result(i, j) += m[i*4 + k] * mat.m[k*4 + j];
			}
		}
	}

	return result;
}

RGBAVector RGBAMatrix::operator *(const RGBAVector &p) const {
	return RGBAVector (
		p.x * m1 + p.y * m2 + p.z * m3 + p.w * m4,
		p.x * m5 + p.y * m6 + p.z * m7 + p.w * m8,
		p.x * m9 + p.y * m10 + p.z * m11 + p.w * m12,
		p.x * m13 + p.y * m14 + p.z * m15 + p.w * m16
	);
}

RGBAMatrix RGBAMatrix::RotateX(float rad) {
	RGBAMatrix result;
	result.m6 = result.m11 = cos(rad);
	result.m10 = sin(rad);
	result.m7 = -result.m10;
	return result;
}

RGBAMatrix RGBAMatrix::RotateY(float rad) {
	RGBAMatrix result;
	result.m1 = result.m11 = cos(rad);
	result.m3 = sin(rad);
	result.m9 = -result.m3;
	return result;
}

RGBAMatrix RGBAMatrix::RotateZ(float rad) {
	RGBAMatrix result;
	result.m1 = result.m6 = cos(rad);
	result.m5 = sin(rad);
	result.m2 = -result.m5;
	return result;
}

RGBAMatrix RGBAMatrix::Translate(const RGBAVector &t) {
	RGBAMatrix result;
	result.m4 = t.x;
	result.m8 = t.y;
	result.m12 = t.z;
	result.m16 = t.w;
	return result;
}

bool RGBAMatrix::Identity() {
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {

			if(i == j) {
				if(fabs(m[i*4 + j] - 1.0f) > 1e-5)
					return false;
			}
			else {
				if(fabs(m[i*4 + j]) > 1e-5)
					return false;
			}
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// Cluster implementation
//
///////////////////////////////////////////////////////////////////////////////

RGBACluster::RGBACluster(const RGBACluster &left, const RGBACluster &right) {
	*this = left;
	for(int i = 0; i < right.m_NumPoints; i++) {
		const RGBAVector &p = right.m_DataPoints[i];
		AddPoint(p);
	}

	m_PrincipalAxisCached = false;
}	

void RGBACluster::AddPoint(const RGBAVector &p) {
	assert(m_NumPoints < kMaxNumDataPoints);
	m_Total += p;
	m_DataPoints[m_NumPoints++] = p;
	m_PointBitString |= 1 << p.GetIdx();

	for(int i = 0; i < kNumColorChannels; i++) {
		m_Min.c[i] = min(p.c[i], m_Min.c[i]);
		m_Max.c[i] = max(p.c[i], m_Max.c[i]);
	}
}

void RGBACluster::GetPrincipalAxis(RGBADir &axis) {

	if(m_PrincipalAxisCached) {
		axis = m_PrincipalAxis;
		return;
	}

	RGBAVector avg = m_Total / float(m_NumPoints);
	m_PowerMethodIterations = ::GetPrincipalAxis(
	    m_NumPoints, 
	    m_DataPoints, 
	    m_PrincipalAxis, 
	    m_PrincipalEigenvalue, 
	    &m_SecondEigenvalue
	);

	m_PrincipalAxisCached = true;

	GetPrincipalAxis(axis);
}

double RGBACluster::GetPrincipalEigenvalue() {

  if(!m_PrincipalAxisCached) {
    RGBADir dummy;
    GetPrincipalAxis(dummy);
  }
  
  assert(m_PrincipalAxisCached);
  return m_PrincipalEigenvalue;
}

double RGBACluster::GetSecondEigenvalue() {

  if(!m_PrincipalAxisCached) {
    RGBADir dummy;
    GetPrincipalAxis(dummy);
  }
  
  assert(m_PrincipalAxisCached);
  return m_SecondEigenvalue;
}

uint32 RGBACluster::GetPowerMethodIterations() {

  if(!m_PrincipalAxisCached) {
    RGBADir dummy;
    GetPrincipalAxis(dummy);
  }
  
  assert(m_PrincipalAxisCached);
  return m_PowerMethodIterations;
}

double RGBACluster::QuantizedError(const RGBAVector &p1, const RGBAVector &p2, uint8 nBuckets, uint32 bitMask, const RGBAVector &errorMetricVec, const int pbits[2], int *indices) const {

	// nBuckets should be a power of two.
	assert(nBuckets == 3 || !(nBuckets & (nBuckets - 1)));

	const uint8 indexPrec = (nBuckets == 3)? 3 : 8-CountBitsInMask(~(nBuckets - 1));
	
	typedef uint32 tInterpPair[2];
	typedef tInterpPair tInterpLevel[16];
	const tInterpLevel *interpVals = (nBuckets == 3)? kBC7InterpolationValues : kBC7InterpolationValues + (indexPrec - 1);

	assert(indexPrec >= 2 && indexPrec <= 4);

	uint32 qp1, qp2;
	if(pbits) {
		qp1 = p1.ToPixel(bitMask, pbits[0]);
		qp2 = p2.ToPixel(bitMask, pbits[1]);
	}
	else {
		qp1 = p1.ToPixel(bitMask);
		qp2 = p2.ToPixel(bitMask);
	}

	uint8 *pqp1 = (uint8 *)&qp1;
	uint8 *pqp2 = (uint8 *)&qp2;

	const RGBAVector metric = errorMetricVec;

	float totalError = 0.0;
	for(int i = 0; i < m_NumPoints; i++) {

		const uint32 pixel = m_DataPoints[i].ToPixel();
		const uint8 *pb = (const uint8 *)(&pixel);

		float minError = FLT_MAX;
		int bestBucket = -1;
		for(int j = 0; j < nBuckets; j++) {

			uint32 interp0 = (*interpVals)[j][0];
			uint32 interp1 = (*interpVals)[j][1];

			RGBAVector errorVec (0.0f);
			for(int k = 0; k < kNumColorChannels; k++) {
				const uint8 ip = (((uint32(pqp1[k]) * interp0) + (uint32(pqp2[k]) * interp1) + 32) >> 6) & 0xFF;
				const uint8 dist = sad(pb[k], ip);
				errorVec.c[k] = kFloatConversion[dist] * metric.c[k];
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
		}

		totalError += minError;

		assert(bestBucket >= 0);
		if(indices) indices[i] = bestBucket;
	}

	return totalError;
}

///////////////////////////////////////////////////////////////////////////////
//
// Utility function implementation
//
///////////////////////////////////////////////////////////////////////////////

void ClampEndpoints(RGBAVector &p1, RGBAVector &p2) {
	clamp(p1.r, 0.0f, 255.0f);
	clamp(p1.g, 0.0f, 255.0f);
	clamp(p1.b, 0.0f, 255.0f);
	clamp(p1.a, 0.0f, 255.0f);

	clamp(p2.r, 0.0f, 255.0f);
	clamp(p2.g, 0.0f, 255.0f);
	clamp(p2.b, 0.0f, 255.0f);
	clamp(p2.a, 0.0f, 255.0f);
}

static uint32 PowerIteration(const RGBAMatrix &mat, RGBADir &eigVec, double &eigVal) {

	int numIterations = 0;
	const int kMaxNumIterations = 200;

	for(int nTries = 0; nTries < 3; nTries++) {
	// !SPEED! Find eigenvectors by using the power method. This is good because the
	// matrix is only 4x4, which allows us to use SIMD...
  RGBAVector b = RGBAVector(rand());
	assert(b.Length() > 0);
	b /= b.Length();

	bool fixed = false;
	numIterations = 0;
	while(!fixed && ++numIterations < kMaxNumIterations) {

		RGBAVector newB = mat * b;

		// !HACK! If the principal eigenvector of the covariance matrix
		// converges to zero, that means that the points lie equally 
		// spaced on a sphere in this space. In this (extremely rare)
		// situation, just choose a point and use it as the principal 
		// direction.
		const float newBlen = newB.Length();
		if(newBlen < 1e-10) {
			eigVec = b;
			eigVal = 0.0;
			return numIterations;
		}

		eigVal = newB.Length();
		newB /= eigVal;

		if(fabs(1.0f - (b * newB)) < 1e-5)
			fixed = true;

		b = newB;
	}

	eigVec = b;  
	if(numIterations < kMaxNumIterations) {
	  break;
	}
  }

	if(numIterations == kMaxNumIterations) {
	  eigVal = 0.0;
	}
	return numIterations;
}

uint32 GetPrincipalAxis(int nPts, const RGBAVector *pts, RGBADir &axis, double &eigOne, double *eigTwo) {

	assert(nPts > 0);
	assert(nPts <= kMaxNumDataPoints);

	RGBAVector avg (0.0f);
	for(int i = 0; i < nPts; i++) {
		avg += pts[i];
	}
	avg /= float(nPts);

	// We use these vectors for calculating the covariance matrix...
	RGBAVector toPts[kMaxNumDataPoints];
	RGBAVector toPtsMax(-FLT_MAX);
	for(int i = 0; i < nPts; i++) {
		toPts[i] = pts[i] - avg;

		for(int j = 0; j < kNumColorChannels; j++) {
			toPtsMax.c[j] = max(toPtsMax.c[j], toPts[i].c[j]);
		}
	}

	// Generate a list of unique points...
	RGBAVector upts[kMaxNumDataPoints];
	int uptsIdx = 0;
	for(int i = 0; i < nPts; i++) {
		
		bool hasPt = false;
		for(int j = 0; j < uptsIdx; j++) {
			if(upts[j] == pts[i])
				hasPt = true;
		}

		if(!hasPt) {
			upts[uptsIdx++] = pts[i];
		}
	}

	assert(uptsIdx > 0);

	if(uptsIdx == 1) {
		axis.r = axis.g = axis.b = axis.a = 0.0f;
		return 0;
	}
	// Collinear?
	else {

		RGBADir dir (upts[1] - upts[0]);
		bool collinear = true;
		for(int i = 2; i < nPts; i++) {
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
	for(int i = 0; i < kNumColorChannels; i++) {
		for(int j = 0; j <= i; j++) {

			float sum = 0.0;
			for(int k = 0; k < nPts; k++) {
				sum += toPts[k].c[i] * toPts[k].c[j];
			}

			covMatrix(i, j) = sum / kFloatConversion[kNumColorChannels - 1];
			covMatrix(j, i) = covMatrix(i, j);
		}
	}
	
	uint32 iters = PowerIteration(covMatrix, axis, eigOne);

	if(NULL != eigTwo) {
	  if(eigOne != 0.0) {
	    RGBAMatrix reduced = covMatrix - eigOne * RGBAMatrix(
	      axis.c[0] * axis.c[0], axis.c[0] * axis.c[1], axis.c[0] * axis.c[2], axis.c[0] * axis.c[3], 
	      axis.c[1] * axis.c[0], axis.c[1] * axis.c[1], axis.c[1] * axis.c[2], axis.c[1] * axis.c[3],
	      axis.c[2] * axis.c[0], axis.c[2] * axis.c[1], axis.c[2] * axis.c[2], axis.c[2] * axis.c[3],
	      axis.c[3] * axis.c[0], axis.c[3] * axis.c[1], axis.c[3] * axis.c[2], axis.c[3] * axis.c[3]
	    );
	    
	    bool allZero = true;
	    for(int i = 0; i < 16; i++) {
	      if(fabs(reduced[i]) > 0.0005) {
		allZero = false;
	      }
	    }

	    if(allZero) {
	      *eigTwo = 0.0;
	    }
	    else {
	      RGBADir dummyDir;
	      iters += PowerIteration(reduced, dummyDir, *eigTwo);
	    }
	  }
	  else {
	    *eigTwo = 0.0;
	  }
        }

	return iters;
}
