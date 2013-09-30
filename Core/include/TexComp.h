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

#ifndef _TEX_COMP_H_
#define _TEX_COMP_H_

#include "CompressedImage.h"
#include "CompressionJob.h"

#include <iosfwd>

// Forward declarations
class Image;
class ImageFile;

struct SCompressionSettings {
  SCompressionSettings(); // defaults

  // The compression format for the image.
  ECompressionFormat format; 

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

  // This is the output stream with which we should output the logs for the
  // compression functions.
  std::ostream *logStream;
};

extern CompressedImage *CompressImage(Image *img, const SCompressionSettings &settings);

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
