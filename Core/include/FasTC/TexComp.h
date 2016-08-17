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

#ifndef _TEX_COMP_H_
#define _TEX_COMP_H_

#include "FasTC/CompressedImage.h"
#include "FasTC/CompressionJob.h"

#include <iosfwd>
#include "FasTC/ImageFwd.h"

// Forward declarations
class ImageFile;

struct SCompressionSettings {
  SCompressionSettings(); // defaults

  // The compression format for the image.
  FasTC::ECompressionFormat format; 

  // The flag that requests us to use SIMD, if it is available
  bool bUseSIMD;

  // The number of threads to spawn in order to process the data
  int iNumThreads;

  // Some compression formats take a measurement of quality when
  // compressing an image. If the format supports it, this value 
  // will be used for quality purposes.
  int iQuality;

  // The number of compressions to perform. The program will compress
  // the image this many times, and then take the average of the timing.
  int iNumCompressions;

  // This setting measures the number of blocks that a thread
  // will process at any given time. If this value is zero, 
  // which is the default, the work will be divided by the
  // number of threads, and each thread will do it's job and 
  // exit.
  int iJobSize; 

  // This flags instructs the compression routine to be launched in succession
  // with many threads at once. Atomic expressions based on the availability
  // in the platform and compiler will provide synchronization.
  bool bUseAtomics;

  // This flag instructs the infrastructure to use the compression routine from
  // PVRTexLib. If no such lib is found during configuration then this flag is
  // ignored. The quality being used is the fastest compression quality.
  bool bUsePVRTexLib;

  // This flag instructs the infrastructure to use the compression routine from
  // NVidia Texture Tools. If no such lib is found during configuration then this
  // flag is ignored.
  bool bUseNVTT;

  // This is the output stream with which we should output the logs for the
  // compression functions.
  std::ostream *logStream;
};

template<typename PixelType>
extern CompressedImage *CompressImage(FasTC::Image<PixelType> *img, const SCompressionSettings &settings);

extern bool CompressImageData(
  const unsigned char *data,
  const unsigned int width,
  const unsigned int height,
  unsigned char *cmpData,
  const unsigned int cmpDataSz,
  const SCompressionSettings &settings
);

// This function computes the Peak Signal to Noise Ratio between a 
// compressed image and a raw image.
extern double ComputePSNR(const CompressedImage &ci, const ImageFile &file);

// This is a multi-platform yield function that preempts the current thread
// based on the threading library that we're using.
extern void YieldThread();

#endif //_TEX_COMP_H_
