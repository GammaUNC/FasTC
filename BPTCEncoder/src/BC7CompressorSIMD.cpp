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

#include "BC7IntTypes.h"
#include "BC7Compressor.h"
#include "BC7CompressionModeSIMD.h"
#include "RGBAEndpointsSIMD.h"
#include "BCLookupTables.h"
#include "BitStream.h"

#ifdef _MSC_VER
#define ALIGN_SSE __declspec( align(16) )
#else
#define ALIGN_SSE __attribute__((aligned(16)))
#endif

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

const uint32 kBC7InterpolationValuesScalar[4][16][2] = {
	{ {64, 0}, {33, 31}, {0, 64}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ {64, 0}, {43, 21}, {21, 43}, {0, 64}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ {64, 0}, {55, 9}, {46, 18}, {37, 27}, {27, 37}, {18, 46}, {9, 55}, {0, 64}, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ {64, 0}, {60, 4}, {55, 9}, {51, 13}, {47, 17}, {43, 21}, {38, 26}, {34, 30}, {30, 34}, {26, 38}, {21, 43}, {17, 47}, {13, 51}, {9, 55}, {4, 60}, {0, 64} }
};

static const ALIGN_SSE uint32 kZeroVector[4] = { 0, 0, 0, 0 };
const __m128i kBC7InterpolationValuesSIMD[4][16][2] = {
	{ 
		{ _mm_set1_epi32(64), *((const __m128i *)kZeroVector)}, 
		{ _mm_set1_epi32(33), _mm_set1_epi32(31) }, 
		{ *((const __m128i *)kZeroVector), _mm_set1_epi32(64) }, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
	},
	{ 
		{ _mm_set1_epi32(64), *((const __m128i *)kZeroVector)}, 
		{ _mm_set1_epi32(43), _mm_set1_epi32(21)}, 
		{ _mm_set1_epi32(21), _mm_set1_epi32(43)}, 
		{ *((const __m128i *)kZeroVector), _mm_set1_epi32(64)}, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
	},
	{ 
		{ _mm_set1_epi32(64), *((const __m128i *)kZeroVector) }, 
		{ _mm_set1_epi32(55), _mm_set1_epi32(9) }, 
		{ _mm_set1_epi32(46), _mm_set1_epi32(18)}, 
		{ _mm_set1_epi32(37), _mm_set1_epi32(27)}, 
		{ _mm_set1_epi32(27), _mm_set1_epi32(37)}, 
		{ _mm_set1_epi32(18), _mm_set1_epi32(46)}, 
		{ _mm_set1_epi32(9), _mm_set1_epi32(55)}, 
		{ *((const __m128i *)kZeroVector), _mm_set1_epi32(64)}, 
		0, 0, 0, 0, 0, 0, 0, 0 
	},
	{ 
		{ _mm_set1_epi32(64), *((const __m128i *)kZeroVector)}, 
		{ _mm_set1_epi32(60), _mm_set1_epi32(4)}, 
		{ _mm_set1_epi32(55), _mm_set1_epi32(9)}, 
		{ _mm_set1_epi32(51), _mm_set1_epi32(13)}, 
		{ _mm_set1_epi32(47), _mm_set1_epi32(17)}, 
		{ _mm_set1_epi32(43), _mm_set1_epi32(21)}, 
		{ _mm_set1_epi32(38), _mm_set1_epi32(26)}, 
		{ _mm_set1_epi32(34), _mm_set1_epi32(30)}, 
		{ _mm_set1_epi32(30), _mm_set1_epi32(34)}, 
		{ _mm_set1_epi32(26), _mm_set1_epi32(38)}, 
		{ _mm_set1_epi32(21), _mm_set1_epi32(43)}, 
		{ _mm_set1_epi32(17), _mm_set1_epi32(47)}, 
		{ _mm_set1_epi32(13), _mm_set1_epi32(51)}, 
		{ _mm_set1_epi32(9), _mm_set1_epi32(55)}, 
		{ _mm_set1_epi32(4), _mm_set1_epi32(60)}, 
		{ *((const __m128i *)kZeroVector), _mm_set1_epi32(64)} 
	}
};

static const ALIGN_SSE uint32 kByteValMask[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
static inline __m128i sad(const __m128i &a, const __m128i &b) {
	const __m128i maxab = _mm_max_epu8(a, b);
	const __m128i minab = _mm_min_epu8(a, b);
	return _mm_and_si128( *((const __m128i *)kByteValMask), _mm_subs_epu8( maxab, minab ) );
}

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cfloat>
#include <ctime>

#ifndef max
template <typename T>
static T max(const T &a, const T &b) {
  return (a > b)? a : b; 
}
#endif

#ifndef min
template <typename T>
static T min(const T &a, const T &b) {
  return (a < b)? a : b;
}
#endif

int BC7CompressionModeSIMD::MaxAnnealingIterations = 50; // This is a setting.
int BC7CompressionModeSIMD::NumUses[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

BC7CompressionModeSIMD::Attributes BC7CompressionModeSIMD::kModeAttributes[kNumModes] = {
	{ 0, 4, 3, 3, 4, 4, 4, 0, BC7CompressionModeSIMD::ePBitType_NotShared },
	{ 1, 6, 2, 3, 6, 6, 6, 0, BC7CompressionModeSIMD::ePBitType_Shared },
	{ 2, 6, 3, 2, 5, 5, 5, 0, BC7CompressionModeSIMD::ePBitType_None },
	{ 3, 6, 2, 2, 7, 7, 7, 0, BC7CompressionModeSIMD::ePBitType_NotShared },
	{ 0 }, // Mode 4 not supported
	{ 0 }, // Mode 5 not supported
	{ 6, 0, 1, 4, 7, 7, 7, 7, BC7CompressionModeSIMD::ePBitType_NotShared },
	{ 7, 6, 2, 2, 5, 5, 5, 5, BC7CompressionModeSIMD::ePBitType_NotShared },
};

void BC7CompressionModeSIMD::ClampEndpointsToGrid(RGBAVectorSIMD &p1, RGBAVectorSIMD &p2, int &bestPBitCombo) const {
	const int nPbitCombos = GetNumPbitCombos();
	const bool hasPbits = nPbitCombos > 1;
	__m128i qmask;
	GetQuantizationMask(qmask);

	ClampEndpoints(p1, p2);

	// !SPEED! This can be faster. We're searching through all possible
	// pBit combos to find the best one. Instead, we should be seeing what
	// the pBit type is for this compression mode and finding the closest 
	// quantization.
	float minDist = FLT_MAX;
	RGBAVectorSIMD bp1, bp2;
	for(int i = 0; i < nPbitCombos; i++) {

		__m128i qp1, qp2;
		if(hasPbits) {
			qp1 = p1.ToPixel(qmask, GetPBitCombo(i)[0]);
			qp2 = p2.ToPixel(qmask, GetPBitCombo(i)[1]);
		}
		else {
			qp1 = p1.ToPixel(qmask);
			qp2 = p2.ToPixel(qmask);
		}

		RGBAVectorSIMD np1 = RGBAVectorSIMD( _mm_cvtepi32_ps( qp1 ) );
		RGBAVectorSIMD np2 = RGBAVectorSIMD( _mm_cvtepi32_ps( qp2 ) );

		RGBAVectorSIMD d1 = np1 - p1;
		RGBAVectorSIMD d2 = np2 - p2;
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

int BC7CompressionModeSIMD::GetSubsetForIndex(int idx, const int shapeIdx) const {
	int subset = 0;
	
	const int nSubsets = GetNumberOfSubsets();
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

int BC7CompressionModeSIMD::GetAnchorIndexForSubset(int subset, const int shapeIdx) const {
	
	const int nSubsets = GetNumberOfSubsets();
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

double BC7CompressionModeSIMD::CompressSingleColor(const RGBAVectorSIMD &p, RGBAVectorSIMD &p1, RGBAVectorSIMD &p2, int &bestPbitCombo) const {

	// Our pixel to compress...
	const __m128i pixel = p.ToPixel(*((const __m128i *)kByteValMask));

	uint32 bestDist = 0xFF;
	bestPbitCombo = -1;

	for(int pbi = 0; pbi < GetNumPbitCombos(); pbi++) {

		const int *pbitCombo = GetPBitCombo(pbi);
		
		uint32 dist = 0x0;
		uint32 bestValI[kNumColorChannels] = { -1, -1, -1, -1 };
		uint32 bestValJ[kNumColorChannels] = { -1, -1, -1, -1 };

		for(int ci = 0; ci < kNumColorChannels; ci++) {

			const uint8 val = ((uint8 *)(&pixel))[4*ci];
			int nBits = 0;
			switch(ci) {
				case 0: nBits = GetRedChannelPrecision(); break;
				case 1: nBits = GetGreenChannelPrecision(); break;
				case 2: nBits = GetBlueChannelPrecision(); break;
				case 3: nBits = GetAlphaChannelPrecision(); break;
			}

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

			const uint32 interpVal0 = kBC7InterpolationValuesScalar[GetNumberOfBitsPerIndex() - 1][1][0];
			const uint32 interpVal1 = kBC7InterpolationValuesScalar[GetNumberOfBitsPerIndex() - 1][1][1];

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

static const ALIGN_SSE uint32 kOneVec[4] = { 1, 1, 1, 1 };

// Fast random number generator. See more information at 
// http://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor/
static uint32 g_seed = uint32(time(NULL));
static inline uint32 fastrand() { 
	g_seed = (214013 * g_seed + 2531011); 
	return (g_seed>>16) & RAND_MAX; 
} 

static __m128i cur_seed = _mm_set1_epi32( int(time(NULL)) ); 	 
static inline __m128i rand_dir()
{
	// static const __m128i mult = _mm_set_epi32( 214013, 17405, 214013, 69069 ); 
	// static const __m128i gadd = _mm_set_epi32( 2531011, 10395331, 13737667, 1 ); 
	static const ALIGN_SSE uint32 mult[4] = { 214013, 17405, 214013, 0 }; 
	static const ALIGN_SSE uint32 gadd[4] = { 2531011, 10395331, 13737667, 0 }; 
	static const ALIGN_SSE uint32 masklo[4] = { RAND_MAX, RAND_MAX, RAND_MAX, RAND_MAX };
	
	cur_seed = _mm_mullo_epi32( *((const __m128i *)mult), cur_seed );
	cur_seed = _mm_add_epi32( *((const __m128i *)gadd), cur_seed );

	const __m128i resShift = _mm_srai_epi32( cur_seed, 16 );
	const __m128i result = _mm_and_si128( resShift, *((const __m128i *)kOneVec) );

	return result;
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

static const ALIGN_SSE uint32 kSevenVec[4] = { 7, 7, 7, 7 };
static const ALIGN_SSE uint32 kNegOneVec[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
static const ALIGN_SSE uint32 kFloatSignBit[4] = { 0x40000000, 0x40000000, 0x40000000, 0x40000000 };

static void ChangePointForDirWithoutPbitChange(RGBAVectorSIMD &v, const __m128 &stepVec) {
	
	const __m128i dirBool = rand_dir();
	const __m128i cmp = _mm_cmpeq_epi32( dirBool, *((const __m128i *)kZeroVector) );

	const __m128 negStepVec = _mm_sub_ps( _mm_castsi128_ps( *((const __m128i *)kZeroVector) ), stepVec );
	const __m128 step = _mm_blendv_ps( negStepVec, stepVec, _mm_castsi128_ps( cmp ) );
	v.vec = _mm_add_ps( v.vec, step );
}

static void ChangePointForDirWithPbitChange(RGBAVectorSIMD &v, int oldPbit, const __m128 &stepVec) {

	const __m128i pBitVec = _mm_set1_epi32( oldPbit );
	const __m128i cmpPBit = _mm_cmpeq_epi32( pBitVec, *((const __m128i *)kZeroVector) );
	const __m128i notCmpPBit = _mm_xor_si128( cmpPBit, *((const __m128i *)kNegOneVec) );

	const __m128i dirBool = rand_dir();
	const __m128i cmpDir = _mm_cmpeq_epi32( dirBool, *((const __m128i *)kOneVec) );
	const __m128i notCmpDir = _mm_xor_si128( cmpDir, *((const __m128i *)kNegOneVec) );
	
	const __m128i shouldDec = _mm_and_si128( cmpDir, cmpPBit );
	const __m128i shouldInc = _mm_and_si128( notCmpDir, notCmpPBit );

	const __m128 decStep = _mm_blendv_ps( _mm_castsi128_ps( *((const __m128i *)kZeroVector) ), stepVec, _mm_castsi128_ps( shouldDec ) );
	v.vec = _mm_sub_ps( v.vec, decStep );

	const __m128 incStep = _mm_blendv_ps( _mm_castsi128_ps( *((const __m128i *)kZeroVector) ), stepVec, _mm_castsi128_ps( shouldInc ) );
	v.vec = _mm_add_ps( v.vec, incStep );
}

void BC7CompressionModeSIMD::PickBestNeighboringEndpoints(const RGBAClusterSIMD &cluster, const RGBAVectorSIMD &p1, const RGBAVectorSIMD &p2, const int curPbitCombo, RGBAVectorSIMD &np1, RGBAVectorSIMD &np2, int &nPbitCombo, const __m128 &stepVec) const {

	np1 = p1;
	np2 = p2;

	// First, let's figure out the new pbit combo... if there's no pbit then we don't need
	// to worry about it.
	const EPBitType pBitType = GetPBitType();
	if(pBitType != ePBitType_None) {

		// If there is a pbit, then we must change it, because those will provide the closest values
		// to the current point.
		if(pBitType == ePBitType_Shared)
			nPbitCombo = (curPbitCombo + 1) % 2;
		else {
			// Not shared... p1 needs to change and p2 needs to change... which means that 
			// combo 0 gets rotated to combo 3, combo 1 gets rotated to combo 2 and vice
			// versa...
			nPbitCombo = 3 - curPbitCombo;
		}

		assert(GetPBitCombo(curPbitCombo)[0] + GetPBitCombo(nPbitCombo)[0] == 1);
		assert(GetPBitCombo(curPbitCombo)[1] + GetPBitCombo(nPbitCombo)[1] == 1);

		const int *pBitCombo = GetPBitCombo(curPbitCombo);
		ChangePointForDirWithPbitChange(np1, pBitCombo[0], stepVec);
		ChangePointForDirWithPbitChange(np2, pBitCombo[1], stepVec);
	}
	else {
		ChangePointForDirWithoutPbitChange(np1, stepVec);
		ChangePointForDirWithoutPbitChange(np2, stepVec);
	}

	ClampEndpoints(np1, np2);
}

bool BC7CompressionModeSIMD::AcceptNewEndpointError(float newError, float oldError, float temp) const {

	const float p = exp((0.15f * (oldError - newError)) / temp);
	// const double r = (double(rand()) / double(RAND_MAX));
	const float r = frand();

	return r < p;
}

double BC7CompressionModeSIMD::OptimizeEndpointsForCluster(const RGBAClusterSIMD &cluster, RGBAVectorSIMD &p1, RGBAVectorSIMD &p2, __m128i *bestIndices, int &bestPbitCombo) const {
	
	const int nBuckets = (1 << GetNumberOfBitsPerIndex());
	const int nPbitCombos = GetNumPbitCombos();
	__m128i qmask;
	GetQuantizationMask(qmask);

	// Here we use simulated annealing to traverse the space of clusters to find the best possible endpoints.
	float curError = cluster.QuantizedError(p1, p2, nBuckets, qmask, GetPBitCombo(bestPbitCombo), bestIndices);
	int curPbitCombo = bestPbitCombo;
	float bestError = curError;
	RGBAVectorSIMD bp1 = p1, bp2 = p2;

	assert(curError == cluster.QuantizedError(p1, p2, nBuckets, qmask, GetPBitCombo(bestPbitCombo)));

	__m128i precVec = _mm_setr_epi32( GetRedChannelPrecision(), GetGreenChannelPrecision(), GetBlueChannelPrecision(), GetAlphaChannelPrecision() );
	const __m128i precMask = _mm_xor_si128( _mm_cmpeq_epi32( precVec, *((const __m128i *)kZeroVector) ), *((const __m128i *)kNegOneVec) );
	precVec = _mm_sub_epi32( *((const __m128i *)kSevenVec), precVec );
	precVec = _mm_slli_epi32( precVec, 23 );
	precVec = _mm_or_si128( precVec, *((const __m128i *)kFloatSignBit) );
	
	//__m128 stepSzVec = _mm_set1_ps(1.0f);
	//__m128 stepVec = _mm_mul_ps( stepSzVec, _mm_castsi128_ps( _mm_and_si128( precMask, precVec ) ) );
	__m128 stepVec = _mm_castsi128_ps( _mm_and_si128( precMask, precVec ) );

	const int maxEnergy = MaxAnnealingIterations;
	for(int energy = 0; bestError > 0 && energy < maxEnergy; energy++) {

		float temp = float(energy) / float(maxEnergy-1);

		__m128i indices[kMaxNumDataPoints/4];
		RGBAVectorSIMD np1, np2;
		int nPbitCombo;

		PickBestNeighboringEndpoints(cluster, p1, p2, curPbitCombo, np1, np2, nPbitCombo, stepVec);

		float error = cluster.QuantizedError(np1, np2, nBuckets, qmask, GetPBitCombo(nPbitCombo), indices);
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

			// Restart...
			energy = 0;
		}
	}

	p1 = bp1;
	p2 = bp2;

	return bestError;
}

double BC7CompressionModeSIMD::CompressCluster(const RGBAClusterSIMD &cluster, RGBAVectorSIMD &p1, RGBAVectorSIMD &p2, __m128i *bestIndices, int &bestPbitCombo) const {
		
	// If all the points are the same in the cluster, then we need to figure out what the best
	// approximation to this point is....
	if(cluster.AllSamePoint()) {
		const RGBAVectorSIMD &p = cluster.GetPoint(0);
		double bestErr = CompressSingleColor(p, p1, p2, bestPbitCombo);

		// We're assuming all indices will be index 1...
		for(int i = 0; i < 4; i++) {
			bestIndices[i] = _mm_set1_epi32(1);
		}
		
		return bestErr;
	}
	
	const int nBuckets = (1 << GetNumberOfBitsPerIndex());
	const int nPbitCombos = GetNumPbitCombos();

	RGBAVectorSIMD avg = cluster.GetTotal() / float(cluster.GetNumPoints());
	RGBADirSIMD axis;
	::GetPrincipalAxis(cluster, axis);

	float mindp = FLT_MAX, maxdp = -FLT_MAX;
	for(int i = 0 ; i < cluster.GetNumPoints(); i++) {
		float dp = (cluster.GetPoint(i) - avg) * axis;
		if(dp < mindp) mindp = dp;
		if(dp > maxdp) maxdp = dp;
	}

	RGBAVectorSIMD pts[1 << 4]; // At most 4 bits per index.
	float numPts[1<<4];
	assert(nBuckets <= 1 << 4);
	
	p1 = avg + mindp * axis;
	p2 = avg + maxdp * axis;

	ClampEndpoints(p1, p2);

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
		
		RGBAVectorSIMD newPts[1 << 4];

		// Assign each of the existing points to one of the buckets...
		for(int i = 0; i < cluster.GetNumPoints(); i++) {

			int minBucket = -1;
			float minDist = FLT_MAX;
			for(int j = 0; j < nBuckets; j++) {
				RGBAVectorSIMD v = cluster.GetPoint(i) - pts[j];
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
			
			numPts[i] = 0.0f;
			newPts[i] = RGBAVectorSIMD(0.0f);
			for(int j = 0; j < cluster.GetNumPoints(); j++) {
				if(bucketIdx[j] == i) {
					numPts[i] += 1.0f;
					newPts[i] += cluster.GetPoint(j);
				}
			}

			// If there are no points in this cluster, then it should
			// remain the same as last time and avoid a divide by zero.
			if(0.0f != numPts[i])
				newPts[i] /= numPts[i];
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
		if(numPts[i] > 0.0f) {
			numBucketsFilled++;
			lastFilledBucket = i;
		}
	}

	assert(numBucketsFilled > 0);
	if(1 == numBucketsFilled) {
		const RGBAVectorSIMD &p = pts[lastFilledBucket];
		double bestErr = CompressSingleColor(p, p1, p2, bestPbitCombo);

		// We're assuming all indices will be index 1...
		for(int i = 0; i < 4; i++) {
			bestIndices[i] = _mm_set1_epi32(1);
		}
		  
		return bestErr;
	}

	// Now that we know the index of each pixel, we can assign the endpoints based on a least squares fit
	// of the clusters. For more information, take a look at this article by NVidia:
	// http://developer.download.nvidia.com/compute/cuda/1.1-Beta/x86_website/projects/dxtc/doc/cuda_dxtc.pdf
	float asq = 0.0, bsq = 0.0, ab = 0.0;
	RGBAVectorSIMD ax(0.0f), bx(0.0f);
	for(int i = 0; i < nBuckets; i++) {
		float a = float(nBuckets - 1 - i) / float(nBuckets - 1);
		float b = float(i) / float(nBuckets - 1);

		float n = numPts[i];
		RGBAVectorSIMD x = pts[i];

		asq += n * a * a;
		bsq += n * b * b;
		ab += n * a * b;

		ax += x * a * n;
		bx += x * b * n;
	}

	float f = 1.0f / (asq * bsq - ab * ab);
	p1 = f * (ax * bsq - bx * ab);
	p2 = f * (bx * asq - ax * ab);

	ClampEndpointsToGrid(p1, p2, bestPbitCombo);

	#ifdef _DEBUG
		int pBitCombo = bestPbitCombo;
		RGBAVectorSIMD tp1 = p1, tp2 = p2;
		ClampEndpointsToGrid(tp1, tp2, pBitCombo);

		assert(p1 == tp1);
		assert(p2 == tp2);
		assert(pBitCombo == bestPbitCombo);
	#endif

	assert(bestPbitCombo >= 0);

	return OptimizeEndpointsForCluster(cluster, p1, p2, bestIndices, bestPbitCombo);
}

double BC7CompressionModeSIMD::Compress(BitStream &stream, const int shapeIdx, const RGBAClusterSIMD *clusters) const {	

	const int kModeNumber = GetModeNumber();
	const int nPartitionBits = GetNumberOfPartitionBits();
	const int nSubsets = GetNumberOfSubsets();

	// Mode #
	stream.WriteBits(1 << kModeNumber, kModeNumber + 1);

	// Partition #
	assert((((1 << nPartitionBits) - 1) & shapeIdx) == shapeIdx);
	stream.WriteBits(shapeIdx, nPartitionBits);
		
	RGBAVectorSIMD p1[kMaxNumSubsets], p2[kMaxNumSubsets];
	int bestIndices[kMaxNumSubsets][kMaxNumDataPoints] = {
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
	};
	int bestPbitCombo[kMaxNumSubsets] = { -1, -1, -1 };

	double totalErr = 0.0;
	for(int cidx = 0; cidx < nSubsets; cidx++) {
		ALIGN_SSE int indices[kMaxNumDataPoints];

		// Compress this cluster
		totalErr += CompressCluster(clusters[cidx], p1[cidx], p2[cidx], (__m128i *)indices, bestPbitCombo[cidx]);

		// !SPEED! We can precompute the subsets for each index based on the shape. This
		// isn't the bottleneck for the compressor, but it could prove to be a little 
		// faster...

		// Map the indices to their proper position.
		int idx = 0;
		for(int i = 0; i < 16; i++) {
			int subs = GetSubsetForIndex(i, shapeIdx);
			if(subs == cidx) {
				bestIndices[cidx][i] = indices[idx++];
			}
		}
	}

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
	__m128i qmask;
	GetQuantizationMask(qmask);

	//Quantize the points...
	__m128i pixel1[kMaxNumSubsets], pixel2[kMaxNumSubsets];
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

		int anchorIdx = GetAnchorIndexForSubset(sidx, shapeIdx);
		assert(bestIndices[sidx][anchorIdx] != -1);

		int nIndexBits = GetNumberOfBitsPerIndex();
		if(bestIndices[sidx][anchorIdx] >> (nIndexBits - 1)) {
			__m128i t = pixel1[sidx]; pixel1[sidx] = pixel2[sidx]; pixel2[sidx] = t;

			int nIndexVals = 1 << nIndexBits;
			for(int i = 0; i < 16; i++) {
				bestIndices[sidx][i] = (nIndexVals - 1) - bestIndices[sidx][i];
			}
		}

		assert(!(bestIndices[sidx][anchorIdx] >> (nIndexBits - 1)));
	}

	// Get the quantized values...
	uint8 r1[kMaxNumSubsets], g1[kMaxNumSubsets], b1[kMaxNumSubsets], a1[kMaxNumSubsets];
	uint8 r2[kMaxNumSubsets], g2[kMaxNumSubsets], b2[kMaxNumSubsets], a2[kMaxNumSubsets];
	for(int i = 0; i < nSubsets; i++) {
		r1[i] = ((uint8 *)(&(pixel1[i])))[0];
		r2[i] = ((uint8 *)(&(pixel2[i])))[0];

		g1[i] = ((uint8 *)(&(pixel1[i])))[4];
		g2[i] = ((uint8 *)(&(pixel2[i])))[4];

		b1[i] = ((uint8 *)(&(pixel1[i])))[8];
		b2[i] = ((uint8 *)(&(pixel2[i])))[8];

		a1[i] = ((uint8 *)(&(pixel1[i])))[12];
		a2[i] = ((uint8 *)(&(pixel2[i])))[12];
	}

	// Write them out...
	const int nRedBits = GetRedChannelPrecision();
	for(int i = 0; i < nSubsets; i++) {
		stream.WriteBits(r1[i] >> (8 - nRedBits), nRedBits);
		stream.WriteBits(r2[i] >> (8 - nRedBits), nRedBits);
	}

	const int nGreenBits = GetGreenChannelPrecision();
	for(int i = 0; i < nSubsets; i++) {
		stream.WriteBits(g1[i] >> (8 - nGreenBits), nGreenBits);
		stream.WriteBits(g2[i] >> (8 - nGreenBits), nGreenBits);
	}

	const int nBlueBits = GetBlueChannelPrecision();
	for(int i = 0; i < nSubsets; i++) {
		stream.WriteBits(b1[i] >> (8 - nBlueBits), nBlueBits);
		stream.WriteBits(b2[i] >> (8 - nBlueBits), nBlueBits);
	}

	const int nAlphaBits = GetAlphaChannelPrecision();
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

	for(int i = 0; i < 16; i++) {
		const int subs = GetSubsetForIndex(i, shapeIdx);
		const int idx = bestIndices[subs][i];
		const int anchorIdx = GetAnchorIndexForSubset(subs, shapeIdx);
		const int nBitsForIdx = GetNumberOfBitsPerIndex();
		assert(idx >= 0 && idx < (1 << nBitsForIdx));
		assert(i != anchorIdx || !(idx >> (nBitsForIdx - 1)) || !"Leading bit of anchor index is not zero!");
		stream.WriteBits(idx, (i == anchorIdx)? nBitsForIdx - 1 : nBitsForIdx);
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
	static void CompressBC7Block(const uint32 *block, uint8 *outBuf);

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

	// Compress an image using BC7 compression. Use the inBuf parameter to point to an image in
	// 4-byte RGBA format. The width and height parameters specify the size of the image in pixels.
	// The buffer pointed to by outBuf should be large enough to store the compressed image. This
	// implementation has an 4:1 compression ratio.
	void CompressImageBC7SIMD(const unsigned char *inBuf, unsigned char *outBuf, unsigned int width, unsigned int height)
	{
		ALIGN_SSE uint32 block[16];

		_MM_SET_ROUNDING_MODE( _MM_ROUND_TOWARD_ZERO );
		BC7CompressionModeSIMD::ResetNumUses();

		BC7CompressionModeSIMD::MaxAnnealingIterations = GetQualityLevel();

		for(int j = 0; j < height; j += 4)
		{
			for(int i = 0; i < width; i += 4)
			{
			  CompressBC7Block((const uint32 *)inBuf, outBuf);

			  outBuf += 16;
			  inBuf += 64;
			}
		}
	}

	// Extract a 4 by 4 block of pixels from inPtr and store it in colorBlock. The width parameter
	// specifies the size of the image in pixels.
	static void ExtractBlock(const uint8* inPtr, int width, uint32* colorBlock)
	{
		// Compute the stride.
		const int stride = width * 4;

		// Copy the first row of pixels from inPtr into colorBlock.
		_mm_store_si128((__m128i*)colorBlock, _mm_load_si128((__m128i*)inPtr));
		inPtr += stride;

		// Copy the second row of pixels from inPtr into colorBlock.
		_mm_store_si128((__m128i*)(colorBlock + 4), _mm_load_si128((__m128i*)inPtr));
		inPtr += stride;

		// Copy the third row of pixels from inPtr into colorBlock.
		_mm_store_si128((__m128i*)(colorBlock + 8), _mm_load_si128((__m128i*)inPtr));
		inPtr += stride;

		// Copy the forth row of pixels from inPtr into colorBlock.
		_mm_store_si128((__m128i*)(colorBlock + 12), _mm_load_si128((__m128i*)inPtr));
	}

	static double CompressTwoClusters(int shapeIdx, const RGBAClusterSIMD *clusters, uint8 *outBuf, double estimatedError) {

		uint8 tempBuf1[16];
		BitStream tmpStream1(tempBuf1, 128, 0);
		BC7CompressionModeSIMD compressor1(1, estimatedError);
			
		double bestError = compressor1.Compress(tmpStream1, shapeIdx, clusters);
		memcpy(outBuf, tempBuf1, 16);
		if(bestError == 0.0) {
			return 0.0;
		}

		uint8 tempBuf3[16];
		BitStream tmpStream3(tempBuf3, 128, 0);
		BC7CompressionModeSIMD compressor3(3, estimatedError);

		double error;
		if((error = compressor3.Compress(tmpStream3, shapeIdx, clusters)) < bestError) {
			bestError = error;
			memcpy(outBuf, tempBuf3, 16);
			if(bestError == 0.0) {
				return 0.0;
			}
		}
		
		// Mode 3 offers more precision for RGB data. Mode 7 is really only if we have alpha.
		//uint8 tempBuf7[16];
		//BitStream tmpStream7(tempBuf7, 128, 0);
		//BC7CompressionModeSIMD compressor7(7, estimatedError);		
		//if((error = compressor7.Compress(tmpStream7, shapeIdx, clusters)) < bestError) {
		//	memcpy(outBuf, tempBuf7, 16);
		//	return error;
		//}

		return bestError;
	}

	static double CompressThreeClusters(int shapeIdx, const RGBAClusterSIMD *clusters, uint8 *outBuf, double estimatedError) {

		uint8 tempBuf0[16];
		BitStream tmpStream0(tempBuf0, 128, 0);

		uint8 tempBuf2[16];
		BitStream tmpStream2(tempBuf2, 128, 0);

		BC7CompressionModeSIMD compressor0(0, estimatedError);
		BC7CompressionModeSIMD compressor2(2, estimatedError);
			
		double error, bestError = (shapeIdx < 16)? compressor0.Compress(tmpStream0, shapeIdx, clusters) : DBL_MAX;
		memcpy(outBuf, tempBuf0, 16);
		if(bestError == 0.0) {
			return 0.0;
		}

		if((error = compressor2.Compress(tmpStream2, shapeIdx, clusters)) < bestError) {
			memcpy(outBuf, tempBuf2, 16);
			return error;
		}

		return bestError;
	}

	static void PopulateTwoClustersForShape(const RGBAClusterSIMD &points, int shapeIdx, RGBAClusterSIMD *clusters) {
		const uint16 shape = kShapeMask2[shapeIdx]; 
		for(int pt = 0; pt < kMaxNumDataPoints; pt++) {

			const RGBAVectorSIMD &p = points.GetPoint(pt);

			if((1 << pt) & shape)
				clusters[1].AddPoint(p, pt);
			else
				clusters[0].AddPoint(p, pt);
		}

		assert(!(clusters[0].GetPointBitString() & clusters[1].GetPointBitString()));
		assert((clusters[0].GetPointBitString() ^ clusters[1].GetPointBitString()) == 0xFFFF);
		assert((shape & clusters[1].GetPointBitString()) == shape);
	}

	static void PopulateThreeClustersForShape(const RGBAClusterSIMD &points, int shapeIdx, RGBAClusterSIMD *clusters) {
		for(int pt = 0; pt < kMaxNumDataPoints; pt++) {

			const RGBAVectorSIMD &p = points.GetPoint(pt);

			if((1 << pt) & kShapeMask3[shapeIdx][0]) {
				if((1 << pt) & kShapeMask3[shapeIdx][1])
					clusters[2].AddPoint(p, pt);
				else
					clusters[1].AddPoint(p, pt);
			}
			else
				clusters[0].AddPoint(p, pt);
		}

		assert(!(clusters[0].GetPointBitString() & clusters[1].GetPointBitString()));
		assert(!(clusters[2].GetPointBitString() & clusters[1].GetPointBitString()));
		assert(!(clusters[0].GetPointBitString() & clusters[2].GetPointBitString()));
	}

	static double EstimateTwoClusterError(RGBAClusterSIMD &c) {
		RGBAVectorSIMD Min, Max, v;
		c.GetBoundingBox(Min, Max);
		v = Max - Min;
		if(v * v == 0) {
			return 0.0;
		}

		return 0.0001 + c.QuantizedError(Min, Max, 8, _mm_set1_epi32(0xFF));
	}

	static double EstimateThreeClusterError(RGBAClusterSIMD &c) {
		RGBAVectorSIMD Min, Max, v;
		c.GetBoundingBox(Min, Max);
		v = Max - Min;
		if(v * v == 0) {
			return 0.0;
		}

		return 0.0001 + c.QuantizedError(Min, Max, 4, _mm_set1_epi32(0xFF));
	}

	// Compress a single block.
	void CompressBC7Block(const uint32 *block, uint8 *outBuf) {
		
		// All a single color?
		if(AllOneColor(block)) {
			BitStream bStrm(outBuf, 128, 0);
			CompressOptimalColorBC7(*((const uint32 *)block), bStrm);
			return;
		}		

		RGBAClusterSIMD blockCluster;
		bool opaque = true;
		bool transparent = true;

		for(int i = 0; i < kMaxNumDataPoints; i++) {
			RGBAVectorSIMD p = RGBAVectorSIMD(block[i]);
			blockCluster.AddPoint(p, i);
			if(fabs(p.a - 255.0f) > 1e-10)
				opaque = false;

			if(p.a > 0.0f)
				transparent = false;
		}

		// The whole block is transparent?
		if(transparent) {
			BitStream bStrm(outBuf, 128, 0);
			WriteTransparentBlock(bStrm);
			return;
		}

		// First we must figure out which shape to use. To do this, simply
		// see which shape has the smallest sum of minimum bounding spheres.
		double bestError[2] = { DBL_MAX, DBL_MAX };
		int bestShapeIdx[2] = { -1, -1 };
		RGBAClusterSIMD bestClusters[2][3];

		for(int i = 0; i < kNumShapes2; i++) 
		{
			RGBAClusterSIMD clusters[2];
			PopulateTwoClustersForShape(blockCluster, i, clusters);

			double err = 0.0;
			for(int ci = 0; ci < 2; ci++) {
				err += EstimateTwoClusterError(clusters[ci]);
			}

			// If it's small, we'll take it!
			if(err < 1e-9) {
				CompressTwoClusters(i, clusters, outBuf, err);
				return;
			}

			if(err < bestError[0]) {
				bestError[0] = err;
				bestShapeIdx[0] = i;
				bestClusters[0][0] = clusters[0];
				bestClusters[0][1] = clusters[1];
			}
		}

		// There are not 3 subset blocks that support alpha...
		if(opaque) {
			for(int i = 0; i < kNumShapes3; i++) {

				RGBAClusterSIMD clusters[3];
				PopulateThreeClustersForShape(blockCluster, i, clusters);

				double err = 0.0;
				for(int ci = 0; ci < 3; ci++) {
					err += EstimateThreeClusterError(clusters[ci]);
				}

				// If it's small, we'll take it!
				if(err < 1e-9) {
					CompressThreeClusters(i, clusters, outBuf, err);
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

		if(opaque) {

			uint8 tempBuf1[16];
			uint8 tempBuf2[16];

			BitStream tempStream1 (tempBuf1, 128, 0);
			BC7CompressionModeSIMD compressor(6, DBL_MAX);
			double best = compressor.Compress(tempStream1, 0, &blockCluster);
			if(best == 0.0f) {
				memcpy(outBuf, tempBuf1, 16);
				return;
			}

			double error = DBL_MAX;
			if((error = CompressTwoClusters(bestShapeIdx[0], bestClusters[0], tempBuf2, bestError[0])) < best) {
				best = error;
				if(error == 0.0f) {
					memcpy(outBuf, tempBuf2, 16);
					return;
				}
				else {
					memcpy(tempBuf1, tempBuf2, 16);
				}
			}

			if(CompressThreeClusters(bestShapeIdx[1], bestClusters[1], tempBuf2, bestError[1]) < best) {
				memcpy(outBuf, tempBuf2, 16);
				return;
			}

			memcpy(outBuf, tempBuf1, 16);
		}
		else {
			assert(!"Don't support alpha yet!");
		}
	}
}
