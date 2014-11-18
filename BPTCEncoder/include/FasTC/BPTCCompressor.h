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

#ifndef BPTCENCODER_INCLUDE_BPTCCOMPRESSOR_H_
#define BPTCENCODER_INCLUDE_BPTCCOMPRESSOR_H_

#include "FasTC/CompressionJob.h"

#include "FasTC/BPTCConfig.h"

#include <iosfwd>

namespace BPTCC {
  // The various available block modes that a BPTC compressor can choose from.
  // The enum is specialized to be power-of-two values so that an EBlockMode
  // variable can be used as a bit mask.
  enum EBlockMode {
    eBlockMode_Zero = 1,
    eBlockMode_One = 2,
    eBlockMode_Two = 4,
    eBlockMode_Three = 8,
    eBlockMode_Four = 16,
    eBlockMode_Five = 32,
    eBlockMode_Six = 64,
    eBlockMode_Seven = 128
  };

  // This is the error metric that is applied to our error measurement algorithm
  // in order to bias calculation towards results that are more in-line with
  // how the Human Visual System works. Uniform error means that each color
  // channel is treated equally. For a while, the widely accepted non-uniform
  // metric has been to give red 30%, green 59% and blue 11% weight when
  // computing the error between two pixels.
  enum ErrorMetric {
    eErrorMetric_Uniform,     // Treats r, g, and b channels equally
    eErrorMetric_Nonuniform,  // { 0.3, 0.59, 0.11 }

    kNumErrorMetrics
  };

  // A shape consists of an index into the table of shapes and the number
  // of partitions that the index corresponds to. Different BPTC modes
  // interpret the shape differently and some are even illegal (such as
  // having an index >= 16 on mode 0). Hence, each shape corresponds to
  // these two variables.
  struct Shape {
    uint32 m_NumPartitions;
    uint32 m_Index;
  };

  // A shape selection can influence the results of the compressor by choosing
  // different modes to compress or not compress. The shape index is a value
  // between zero and sixty-four that corresponds to one of the available
  // partitioning schemes defined by the BPTC format.
  struct ShapeSelection {
    // This is the number of indices from which to select the appropriate
    // shapes. I.e. the compressor will try the first m_NumIndices shapes
    uint32 m_NumIndices;

    // These are the shape indices to use when evaluating two-partition shapes.
    Shape m_Shapes[10];

    // This is the additional mask to prevent modes once shape selection
    // is done. This value is &-ed with m_BlockModes from CompressionSettings
    // to determine what the final considered blocks are.
    uint32 m_SelectedModes;

    // Defaults
    ShapeSelection()
    : m_NumIndices(0)
    , m_SelectedModes(static_cast<EBlockMode>(0xFF))
    { }
  };

  // A shape selection function is one that selects a BPTC shape from a given
  // block position and pixel array.
  typedef ShapeSelection (*ShapeSelectionFn)
    (uint32 x, uint32 y, const uint32 pixels[16], const void *userData);

  // Compression parameters used to control the BPTC compressor. Each of the
  // values has a default, so this is not strictly required to perform
  // compression, but some aspects of the compressor can be user-defined or
  // overridden.
  struct CompressionSettings {
    // The shape selection function to use during compression. The default (when
    // this variable is set to NULL) is to use the diagonal of the axis-aligned
    // bounding box of every partition to estimate the error using that
    // partition would accrue. The shape with the least error is then chosen.
    // This procedure is done for both two and three partition shapes, and then
    // every block mode is still available.
    ShapeSelectionFn m_ShapeSelectionFn;

    // The user data passed to the shape selection function.
    const void *m_ShapeSelectionUserData;

    // The block modes that the compressor will consider during compression.
    // This variable is a bit mask of EBlockMode values and by default contains
    // every mode. This setting can be used to further restrict the search space
    // and increase compression times.
    uint32 m_BlockModes;

    // See the description for ErrorMetric.
    ErrorMetric m_ErrorMetric;

    // The number of simulated annealing steps to perform per refinement
    // iteration. In general, a larger number produces better results. The
    // default is set to 50. This metric works on a logarithmic scale -- twice
    // the value will double the compute time, but only decrease the error by
    // two times a factor.
    uint32 m_NumSimulatedAnnealingSteps;

    CompressionSettings()
    : m_ShapeSelectionFn(NULL)
    , m_ShapeSelectionUserData(NULL)
    , m_BlockModes(static_cast<EBlockMode>(0xFF))
    , m_ErrorMetric(eErrorMetric_Uniform)
    , m_NumSimulatedAnnealingSteps(50)
    { }
  };

  // Retreives a float4 pointer for the r, g, b, a weights for each color
  // channel, in that order.
  const float *GetErrorMetric(ErrorMetric e);

  // Compress the image given as RGBA data to BPTC format. Width and Height are
  // the dimensions of the image in pixels.
  void Compress(const FasTC::CompressionJob &,
                CompressionSettings settings = CompressionSettings());

  // Perform a compression while recording all of the choices the compressor
  // made into a list of statistics. We can use this to see whether or not
  // certain heuristics are working, such as whether or not certain modes are
  // being chosen more often than others, etc.
  void CompressWithStats(const FasTC::CompressionJob &, std::ostream *logStream,
                         CompressionSettings settings = CompressionSettings());

#ifdef HAS_SSE_41
  // Compress the image given as RGBA data to BPTC format using an algorithm
  // optimized for SIMD enabled platforms. Width and Height are the dimensions
  // of the image in pixels.
  void CompressImageBPTCSIMD(const unsigned char* inBuf, unsigned char* outBuf,
                            unsigned int width, unsigned int height);
#endif

#ifdef HAS_ATOMICS
  // This is a threadsafe version of the compression function that is designed
  // to compress a list of textures. If this function is called with the same
  // argument from multiple threads, they will work together to compress all of
  // the images in the list.
  void CompressAtomic(FasTC::CompressionJobList &);
#endif

#ifdef FOUND_NVTT_BPTC_EXPORT
  // These functions take the same arguments as Compress and CompressWithStats,
  // but they use the NVTT compressor if it was supplied to CMake.
  void CompressNVTT(const FasTC::CompressionJob &);
  void CompressNVTTWithStats(const FasTC::CompressionJob &,
                             std::ostream *logStream);
#endif

  // Decompress the image given as BPTC data to R8G8B8A8 format.
  void Decompress(const FasTC::DecompressionJob &);
}  // namespace BPTCC

#endif  // BPTCENCODER_INCLUDE_BPTCCOMPRESSOR_H_
