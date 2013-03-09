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
#include "CompressionJob.h"

class BlockStatManager;

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
	void Compress(const CompressionJob &);

  // Perform a compression while recording all of the choices the compressor made into a 
  // list of statistics. We can use this to see whether or not certain heuristics are working, such as
  // whether or not certain modes are being chosen more often than others, etc.
  void CompressWithStats(const CompressionJob &, BlockStatManager &statManager);

#ifdef HAS_SSE_41
	// Compress the image given as RGBA data to BC7 format using an algorithm optimized for SIMD
	// enabled platforms. Width and Height are the dimensions of the image in pixels.
	void CompressImageBC7SIMD(const unsigned char* inBuf, unsigned char* outBuf, unsigned int width, unsigned int height);
#endif

#ifdef HAS_ATOMICS
  // This is a threadsafe version of the compression function that is designed to compress a list of 
  // textures. If this function is called with the same argument from multiple threads, they will work
  // together to compress all of the images in the list. 
  void CompressAtomic(CompressionJobList &);
#endif

	// Decompress the image given as BC7 data to R8G8B8A8 format. Width and Height are the dimensions of the image in pixels.
	void Decompress(const DecompressionJob &);
}
