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

namespace BC7C
{
	// This is the error metric that is applied to our error measurement algorithm
	// in order to bias calculation towards results that are more in-line with 
	// how the Human Visual System works. Uniform error means that each color 
	// channel is treated equally. For a while, the widely accepted non-uniform metric
	// has been to give red 30%, green 59% and blue 11% weight when computing the error
	// between two pixels.
	enum ErrorMetric
	{
		eErrorMetric_Uniform, // Treats r, g, and b channels equally
		eErrorMetric_Nonuniform, // { 0.3, 0.59, 0.11 }
		
		kNumErrorMetrics
	};

	// Sets the error metric to be the one specified.
	void SetErrorMetric(ErrorMetric e);

	// Retreives a float4 pointer for the r, g, b, a weights for each color channel, in
	// that order, based on the current error metric.
	const float *GetErrorMetric();

	// Returns the enumeration for the current error metric.
	ErrorMetric GetErrorMetricEnum();

	// Sets the number of steps that we use to perform simulated annealing. In general, a
	// larger number produces better results. The default is set to 50. This metric works
	// on a logarithmic scale -- twice the value will double the compute time, but only
	// decrease the error by two times a factor.
	void SetQualityLevel(int q);
	int GetQualityLevel();

	// Compress the image given as RGBA data to BC7 format. Width and Height are the dimensions of
	// the image in pixels.
	void CompressImageBC7(const unsigned char *inBuf, unsigned char *outBuf, unsigned int width, unsigned int height);

	// Compress the image given as RGBA data to BC7 format using an algorithm optimized for SIMD
	// enabled platforms. Width and Height are the dimensions of the image in pixels.
	void CompressImageBC7SIMD(const unsigned char* inBuf, unsigned char* outBuf, unsigned int width, unsigned int height);

	// Decompress the image given as BC7 data to R8G8B8A8 format. Width and Height are the dimensions of the image in pixels.
	void DecompressImageBC7SIMD(const unsigned char* inBuf, unsigned char* outBuf, unsigned int width, unsigned int height);
}
