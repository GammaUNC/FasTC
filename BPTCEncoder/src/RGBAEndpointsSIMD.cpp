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
#include "RGBAEndpointsSIMD.h"
#include "BC7Compressor.h"
#include "BC7CompressionModeSIMD.h"

#include <cassert>
#include <cfloat>

#ifndef HAS_SSE_POPCNT 
static inline uint32 popcnt32(uint32 x) {
  uint32 m1 = 0x55555555;
  uint32 m2 = 0x33333333;
  uint32 m3 = 0x0f0f0f0f;
  x -= (x>>1) & 1;
  x = (x&m2) + ((x>>2)&m2);
  x = (x+(x>>4))&m3;
  x += x>>8;
  return (x+(x>>16)) & 0x3f;
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// RGBAVectorSIMD implementation
//
///////////////////////////////////////////////////////////////////////////////

/* Original scalar implementation:

	// If the mask is all the bits, then we can just return the value.
	if(mask == 0xFF) {
		return val;
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
*/

// !TODO! AVX2 supports an instruction known as vsllv, which shifts a vector
// by the values stored in another vector. I.e. you can do something like this:
//
// __m128i shiftVals = _mm_set_epi32(1, 2, 3, 4);
// __m128i someVector = _mm_set1_epi32(1) ;
// __m128i shifted = _mm_srav_epi32 (someVector, shiftVals);
//
// and the result will be the same as __mm_Set_epi32(1, 4, 8, 16);
//
// This is useful because our color channels may have different precisions
// when we're quantizing them, such as for BC7 modes 4 and 5. Hence, we would
// want to do our quantization as accurately as possible, but currently it would
// be very hard to vectorize.

#ifdef _MSC_VER
#define ALIGN_SSE __declspec ( align(16) )
#else
#define ALIGN_SSE __attribute__((aligned(16)))
#endif

// Constants. There are two ways to specify them: either by using the _mm_set* 
// intrinsics, or by defining them as aligned arrays. You want to do the former 
// when you use them infrequently, and the latter when you use them multiple times
// in a short time frame (like in an inner loop)
static const __m128 kZero = _mm_set1_ps(0.0f);
static const __m128 kByteMax = _mm_set1_ps(255.0f);
static const __m128 kHalfVector = _mm_set1_ps(0.5f);
static const __m128i kOneVector = _mm_set1_epi32(1);
static const __m128i kZeroVector = _mm_set1_epi32(0);
static const ALIGN_SSE uint32 kThirtyTwoVector[4] = { 32, 32, 32, 32 };
static const __m128i kByteValMask = _mm_set_epi32(0xFF, 0xFF, 0xFF, 0xFF);

static inline __m128i sad(const __m128i &a, const __m128i &b) {
	const __m128i maxab = _mm_max_epu8(a, b);
	const __m128i minab = _mm_min_epu8(a, b);
	return _mm_and_si128( kByteValMask, _mm_subs_epu8( maxab, minab ) );
}

__m128i RGBAVectorSIMD::ToPixel(const __m128i &qmask) const {

	// !SPEED! We should figure out a way to get rid of these scalar operations.
#ifdef HAS_SSE_POPCNT
	const uint32 prec = _mm_popcnt_u32(((uint32 *)(&qmask))[0]);
#else
	const uint32 prec = popcnt32(((uint32 *)(&qmask))[0]);
#endif
	
	assert(r >= 0.0f && r <= 255.0f);
	assert(g >= 0.0f && g <= 255.0f);
	assert(b >= 0.0f && b <= 255.0f);
	assert(a >= 0.0f && a <= 255.0f);
	assert(((uint32 *)(&qmask))[3] == 0xFF || ((uint32 *)(&qmask))[3] == ((uint32 *)(&qmask))[0]);
	assert(((uint32 *)(&qmask))[2] == ((uint32 *)(&qmask))[1] && ((uint32 *)(&qmask))[0] == ((uint32 *)(&qmask))[1]);

	const __m128i val = _mm_cvtps_epi32( _mm_add_ps(kHalfVector, vec) );

	const __m128i step = _mm_slli_epi32( kOneVector, 8 - prec );
	const __m128i &mask = qmask;

	__m128i lval = _mm_and_si128(val, mask);
	__m128i hval = _mm_add_epi32(lval, step);

	const __m128i lvalShift = _mm_srli_epi32(lval, prec);
	const __m128i hvalShift = _mm_srli_epi32(hval, prec);

	lval = _mm_or_si128(lval, lvalShift);
	hval = _mm_or_si128(hval, hvalShift);

	const __m128i lvald = _mm_sub_epi32( val, lval );
	const __m128i hvald = _mm_sub_epi32( hval, val );

	const __m128i vd = _mm_cmplt_epi32(lvald, hvald);
	__m128i ans = _mm_blendv_epi8(hval, lval, vd);

	const __m128i chanExact = _mm_cmpeq_epi32(mask, kByteValMask);
	ans = _mm_blendv_epi8( ans, val, chanExact );
	return ans;
}

__m128i RGBAVectorSIMD::ToPixel(const __m128i &qmask, const int pBit) const {
	
	// !SPEED! We should figure out a way to get rid of these scalar operations.
#ifdef HAS_SSE_POPCNT
	const uint32 prec = _mm_popcnt_u32(((uint32 *)(&qmask))[0]);
#else
	const uint32 prec = popcnt32(((uint32 *)(&qmask))[0]);
#endif
	
	assert(r >= 0.0f && r <= 255.0f);
	assert(g >= 0.0f && g <= 255.0f);
	assert(b >= 0.0f && b <= 255.0f);
	assert(a >= 0.0f && a <= 255.0f);
	assert(((uint32 *)(&qmask))[3] == 0xFF || ((uint32 *)(&qmask))[3] == ((uint32 *)(&qmask))[0]);
	assert(((uint32 *)(&qmask))[2] == ((uint32 *)(&qmask))[1] && ((uint32 *)(&qmask))[0] == ((uint32 *)(&qmask))[1]);

	const __m128i val = _mm_cvtps_epi32( _mm_add_ps(kHalfVector, vec) );
	const __m128i pbit = _mm_set1_epi32(!!pBit);

	const __m128i &mask = qmask; // _mm_set_epi32(alphaMask, channelMask, channelMask, channelMask);
	const __m128i step = _mm_slli_epi32( kOneVector, 8 - prec );

	__m128i lval = _mm_and_si128( val, mask );
	__m128i hval = _mm_add_epi32( lval, step );

	const __m128i pBitShifted = _mm_slli_epi32(pbit, 7 - prec);
	lval = _mm_or_si128(lval, pBitShifted );
	hval = _mm_or_si128(hval, pBitShifted);

	// These next three lines we make sure that after adding the pbit that val is
	// still in between lval and hval. If it isn't, then we subtract a
	// step from both. Now, val should be larger than lval and less than
	// hval, but certain situations make this not always the case (e.g. val
	// is 0, precision is 4 bits, and pbit is 1). Hence, we add back the
	// step if it goes below zero, making it equivalent to hval and so it
	// doesn't matter which we choose.
	{
		__m128i cmp = _mm_cmpgt_epi32(lval, val);
		cmp = _mm_mullo_epi32(cmp, step);
		lval = _mm_add_epi32(lval, cmp);
		hval = _mm_add_epi32(hval, cmp);

		cmp = _mm_cmplt_epi32(lval, kZeroVector);
		cmp = _mm_mullo_epi32(cmp, step);
		lval = _mm_sub_epi32(lval, cmp);
	}

	const __m128i lvalShift = _mm_srli_epi32(lval, prec + 1);
	const __m128i hvalShift = _mm_srli_epi32(hval, prec + 1);

	lval = _mm_or_si128(lval, lvalShift);
	hval = _mm_or_si128(hval, hvalShift);

	const __m128i lvald = _mm_sub_epi32( val, lval );
	const __m128i hvald = _mm_sub_epi32( hval, val );

	const __m128i vd = _mm_cmplt_epi32(lvald, hvald);
	__m128i ans = _mm_blendv_epi8(hval, lval, vd);

	const __m128i chanExact = _mm_cmpeq_epi32(mask, kByteValMask);
	ans = _mm_blendv_epi8( ans, val, chanExact );
	return ans;
}

///////////////////////////////////////////////////////////////////////////////
//
// RGBAMatrixSIMD implementation
//
///////////////////////////////////////////////////////////////////////////////

RGBAVectorSIMD RGBAMatrixSIMD::operator *(const RGBAVectorSIMD &p) const {
	
	__m128 xVec = _mm_set1_ps( p.x );
	__m128 yVec = _mm_set1_ps( p.y );
	__m128 zVec = _mm_set1_ps( p.z );
	__m128 wVec = _mm_set1_ps( p.w );

	__m128 vec1 = _mm_mul_ps( xVec, col[0] );
	__m128 vec2 = _mm_mul_ps( yVec, col[1] );
	__m128 vec3 = _mm_mul_ps( zVec, col[2] );
	__m128 vec4 = _mm_mul_ps( wVec, col[3] );

	return RGBAVectorSIMD( _mm_add_ps( _mm_add_ps( vec1, vec2 ), _mm_add_ps( vec3, vec4 ) ) );
}

///////////////////////////////////////////////////////////////////////////////
//
// Cluster implementation
//
///////////////////////////////////////////////////////////////////////////////

RGBAClusterSIMD::RGBAClusterSIMD(const RGBAClusterSIMD &left, const RGBAClusterSIMD &right) {

	assert(!(left.m_PointBitString & right.m_PointBitString));

	*this = left;
	for(int i = 0; i < right.m_NumPoints; i++) {

		const RGBAVectorSIMD &p = right.m_DataPoints[i];

		assert(m_NumPoints < kMaxNumDataPoints);
		m_Total += p;
		m_DataPoints[m_NumPoints++] = p;

		m_Min.vec = _mm_min_ps(m_Min.vec, p.vec);
		m_Max.vec = _mm_max_ps(m_Max.vec, p.vec);
	}

	m_PointBitString = left.m_PointBitString | right.m_PointBitString;
	m_PrincipalAxisCached = false;
}	

void RGBAClusterSIMD::AddPoint(const RGBAVectorSIMD &p, int idx) {
	assert(m_NumPoints < kMaxNumDataPoints);
	m_Total += p;
	m_DataPoints[m_NumPoints++] = p;
	m_PointBitString |= 1 << idx;

	m_Min.vec = _mm_min_ps(m_Min.vec, p.vec);
	m_Max.vec = _mm_max_ps(m_Max.vec, p.vec);
}

float RGBAClusterSIMD::QuantizedError(const RGBAVectorSIMD &p1, const RGBAVectorSIMD &p2, const uint8 nBuckets, const __m128i &bitMask, const int pbits[2], __m128i *indices) const {

	// nBuckets should be a power of two.
	assert(!(nBuckets & (nBuckets - 1)));

#ifdef HAS_SSE_POPCNT
	const uint8 indexPrec = 8-_mm_popcnt_u32(~(nBuckets - 1) & 0xFF);
#else
	const uint8 indexPrec = 8-popcnt32(~(nBuckets - 1) & 0xFF);
#endif
	assert(indexPrec >= 2 && indexPrec <= 4);

	typedef __m128i tInterpPair[2];
	typedef tInterpPair tInterpLevel[16];
	const tInterpLevel *interpVals = kBC7InterpolationValuesSIMD + (indexPrec - 1);

	__m128i qp1, qp2;
	if(pbits) {
		qp1 = p1.ToPixel(bitMask, pbits[0]);
		qp2 = p2.ToPixel(bitMask, pbits[1]);
	}
	else {
		qp1 = p1.ToPixel(bitMask);
		qp2 = p2.ToPixel(bitMask);
	}

	__m128 errorMetricVec = _mm_load_ps( BC7C::GetErrorMetric() );

	__m128 totalError = kZero;
	for(int i = 0; i < m_NumPoints; i++) {

		const __m128i pixel = m_DataPoints[i].ToPixel( kByteValMask );

		__m128 minError = _mm_set1_ps(FLT_MAX);
		__m128i bestBucket = _mm_set1_epi32(-1);
		for(int j = 0; j < nBuckets; j++) {

			const __m128i jVec = _mm_set1_epi32(j);
			const __m128i interp0 = (*interpVals)[j][0];
			const __m128i interp1 = (*interpVals)[j][1];

			const __m128i ip0 = _mm_mullo_epi32( qp1, interp0 );
			const __m128i ip1 = _mm_mullo_epi32( qp2, interp1 );
			const __m128i ip = _mm_add_epi32( *((const __m128i *)kThirtyTwoVector), _mm_add_epi32( ip0, ip1 ) );
			const __m128i dist = sad( _mm_and_si128( _mm_srli_epi32( ip, 6 ), kByteValMask ), pixel );
			__m128 errorVec = _mm_cvtepi32_ps( dist );
			
			errorVec = _mm_mul_ps( errorVec, errorMetricVec );
			errorVec = _mm_mul_ps( errorVec, errorVec );
			errorVec = _mm_hadd_ps( errorVec, errorVec );
			errorVec = _mm_hadd_ps( errorVec, errorVec );

			const __m128 cmp = _mm_cmple_ps( errorVec, minError );
			minError = _mm_blendv_ps( minError, errorVec, cmp );
			bestBucket = _mm_blendv_epi8( bestBucket, jVec, _mm_castps_si128( cmp ) );

			// Conceptually, once the error starts growing, it doesn't stop growing (we're moving
			// farther away from the reference point along the line). Hence we can early out here.
			// However, quanitzation artifacts mean that this is not ALWAYS the case, so we do suffer
			// about 0.01 RMS error. 
			if(!((uint8 *)(&cmp))[0])
				break;
		}

		totalError = _mm_add_ps(totalError, minError);
		if(indices) ((uint32 *)indices)[i] = ((uint32 *)(&bestBucket))[0];
	}

	return ((float *)(&totalError))[0];
}

///////////////////////////////////////////////////////////////////////////////
//
// Utility function implementation
//
///////////////////////////////////////////////////////////////////////////////

void ClampEndpoints(RGBAVectorSIMD &p1, RGBAVectorSIMD &p2) {
	p1.vec = _mm_min_ps( kByteMax, _mm_max_ps( p1.vec, kZero ) );
	p2.vec = _mm_min_ps( kByteMax, _mm_max_ps( p2.vec, kZero ) );
}

void GetPrincipalAxis(const RGBAClusterSIMD &c, RGBADirSIMD &axis) {

	if(c.GetNumPoints() == 2) {
		axis = c.GetPoint(1) - c.GetPoint(0);
		return;
	}

	RGBAVectorSIMD avg = c.GetTotal();
	avg /= float(c.GetNumPoints());

	// We use these vectors for calculating the covariance matrix...
	RGBAVectorSIMD toPts[kMaxNumDataPoints];
	RGBAVectorSIMD toPtsMax(-FLT_MAX);
	for(int i = 0; i < c.GetNumPoints(); i++) {
		toPts[i] = c.GetPoint(i) - avg;
		toPtsMax.vec = _mm_max_ps(toPtsMax.vec, toPts[i].vec);
	}

	RGBAMatrixSIMD covMatrix;

	// Compute covariance.
	const float fNumPoints = float(c.GetNumPoints());
	for(int i = 0; i < kNumColorChannels; i++) {
		for(int j = 0; j <= i; j++) {

			float sum = 0.0;
			for(int k = 0; k < c.GetNumPoints(); k++) {
				sum += toPts[k].c[i] * toPts[k].c[j];
			}

			covMatrix(i, j) = sum / fNumPoints;
			covMatrix(j, i) = covMatrix(i, j);
		}
	}

	// !SPEED! Find eigenvectors by using the power method. This is good because the
	// matrix is only 4x4, which allows us to use SIMD...
	RGBAVectorSIMD b = toPtsMax;
	assert(b.Length() > 0);
	b /= b.Length();

	RGBAVectorSIMD newB = covMatrix * b;

	// !HACK! If the principal eigenvector of the covariance matrix
	// converges to zero, that means that the points lie equally 
	// spaced on a sphere in this space. In this (extremely rare)
	// situation, just choose a point and use it as the principal 
	// direction.
	const float newBlen = newB.Length();
	if(newBlen < 1e-10) {
		axis = toPts[0];
		return;
	}

	for(int i = 0; i < 8; i++) {
		newB = covMatrix * b;
		newB.Normalize();
		b = newB;
	}

	axis = b;
}
