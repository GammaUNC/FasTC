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

#include "TexComp.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <iostream>

#include "BC7Compressor.h"
#include "CompressionFuncs.h"
#include "Image.h"
#include "ImageFile.h"
#include "Pixel.h"
#include "PVRTCCompressor.h"
#include "Thread.h"
#include "ThreadGroup.h"
#include "WorkerQueue.h"

template <typename T>
static void clamp(T &x, const T &minX, const T &maxX) {
  x = std::max(std::min(maxX, x), minX);
}

template <typename T>
static inline T sad(const T &a, const T &b) {
  return (a > b)? a - b : b - a;
}

static void CompressPVRTC(const CompressionJob &cj) {
  PVRTCC::Compress(cj);
}

SCompressionSettings:: SCompressionSettings()
  : format(eCompressionFormat_BPTC)
  , bUseSIMD(false)
  , iNumThreads(1)
  , iQuality(50)
  , iNumCompressions(1)
{
  clamp(iQuality, 0, 256);
}

static  CompressionFuncWithStats ChooseFuncFromSettingsWithStats(const SCompressionSettings &s) {
  switch(s.format) {
    case eCompressionFormat_BPTC:
    {
       return BC7C::CompressWithStats;
    }
    break;

    case eCompressionFormat_PVRTC:
    {
      // !FIXME! actually implement one of these methods...
      return NULL;
    }

    default:
    {
      assert(!"Not implemented!");
      return NULL;
    }
  }
  return NULL;
}

static CompressionFunc ChooseFuncFromSettings(const SCompressionSettings &s) {
  switch(s.format) {
    case eCompressionFormat_BPTC:
    {
      BC7C::SetQualityLevel(s.iQuality);
#ifdef HAS_SSE_41
      if(s.bUseSIMD) {
        return BC7C::CompressImageBC7SIMD;
      }
#endif
      return BC7C::Compress;
    }
    break;

    case eCompressionFormat_PVRTC:
    {
      return CompressPVRTC;
    }

    default:
    {
      assert(!"Not implemented!");
      return NULL;
    }
  }
  return NULL;
}

static void ReportError(const char *msg) {
  fprintf(stderr, "TexComp -- %s\n", msg);
}

static double CompressImageInSerial(
  const uint8 *imgData,
  const uint32 imgWidth,
  const uint32 imgHeight,
  const SCompressionSettings &settings,
  unsigned char *outBuf
) {
  CompressionFunc f = ChooseFuncFromSettings(settings);
  CompressionFuncWithStats fStats = ChooseFuncFromSettingsWithStats(settings);

  double cmpTimeTotal = 0.0;

  StopWatch stopWatch = StopWatch();
  for(int i = 0; i < settings.iNumCompressions; i++) {

    stopWatch.Reset();
    stopWatch.Start();

    // !FIXME! We're assuming that we have 4x4 blocks here...
    CompressionJob cj (imgData, outBuf, imgWidth, imgHeight);
    if(fStats && settings.logStream) {
      // !FIXME! Actually use the stat manager...
      //(*fStats)(cj, *(settings.pStatManager));
      (*fStats)(cj, settings.logStream);
    }
    else {
      (*f)(cj);
    }

    stopWatch.Stop();

    cmpTimeTotal += stopWatch.TimeInMilliseconds();
  }

  double cmpTime = cmpTimeTotal / double(settings.iNumCompressions);
  return cmpTime;
}

class AtomicThreadUnit : public TCCallable {
  CompressionJobList &m_CompressionJobList;
  TCBarrier *m_Barrier;
  CompressionFunc m_CmpFnc;

 public:
  AtomicThreadUnit(
    CompressionJobList &_cjl,
    TCBarrier *barrier,
    CompressionFunc f
  ) : TCCallable(),
    m_CompressionJobList(_cjl),
    m_Barrier(barrier),
    m_CmpFnc(f)
  { }

  virtual ~AtomicThreadUnit() { }
  virtual void operator()() {
    m_Barrier->Wait();
    if(m_CmpFnc == BC7C::Compress) {
      BC7C::CompressAtomic(m_CompressionJobList);
    }
    else {
      assert(!"I don't know what we're compressing...");
    }
  }
};

static double CompressImageWithAtomics(
  const unsigned char *imgData,
  const unsigned int width, const unsigned int height,
  const SCompressionSettings &settings,
  unsigned char *outBuf
) {
  CompressionFunc f = ChooseFuncFromSettings(settings);
  
  // Setup compression list...
  const int nTimes = settings.iNumCompressions;
  CompressionJobList cjl (nTimes);
  for(int i = 0; i < nTimes; i++) {
    if(!cjl.AddJob(CompressionJob(imgData, outBuf, height, width))) {
      assert(!"Error adding compression job to job list!");
    }
  }

  const int nThreads = settings.iNumThreads;

  // Allocate resources...
  TCBarrier barrier (nThreads+1);
  TCThread **threads = (TCThread **)malloc(nThreads * sizeof(TCThread *));
  AtomicThreadUnit **units = (AtomicThreadUnit **)malloc(nThreads * sizeof(AtomicThreadUnit *));

  // Launch threads...
  for(int i = 0; i < nThreads; i++) {
    AtomicThreadUnit *u = new AtomicThreadUnit(cjl, &barrier, f);
    threads[i] = new TCThread(*u);
    units[i] = u;
  }

  // Wait here to make sure that our timer is correct...
  barrier.Wait();

  StopWatch sw;
  sw.Start();

  // Wait for threads to finish
  for(int i = 0; i < nThreads; i++) {
    threads[i]->Join();
  }
  sw.Stop();

  // Cleanup
  for(int i = 0; i < nThreads; i++)
    delete threads[i];
  free(threads);
  for(int i = 0; i < nThreads; i++)
    delete units[i];
  free(units);

  // Compression time
  double cmpTimeTotal = sw.TimeInMilliseconds();
  return cmpTimeTotal / double(settings.iNumCompressions);
}

static double CompressThreadGroup(ThreadGroup &tgrp, const SCompressionSettings &settings) {
  if(!(tgrp.PrepareThreads())) {
    assert(!"Thread group failed to prepare threads?!");
    return -1.0f;
  }

  double cmpTimeTotal = 0.0;
  for(int i = 0; i < settings.iNumCompressions; i++) {
    if(i > 0)
      tgrp.PrepareThreads();

    tgrp.Start();
    tgrp.Join();

    StopWatch stopWatch = tgrp.GetStopWatch();
    cmpTimeTotal += tgrp.GetStopWatch().TimeInMilliseconds();
  }

  tgrp.CleanUpThreads();  
  return cmpTimeTotal;
}

static double CompressImageWithThreads(
  const unsigned char *imgData,
  const unsigned int imgDataSz,
  const SCompressionSettings &settings,
  unsigned char *outBuf
) {

  CompressionFunc f = ChooseFuncFromSettings(settings);
  CompressionFuncWithStats fStats = ChooseFuncFromSettingsWithStats(settings);

  double cmpTimeTotal = 0.0;
  if(fStats && settings.logStream) {
    ThreadGroup tgrp (settings.iNumThreads, imgData, imgDataSz, fStats, settings.logStream, outBuf);
    cmpTimeTotal = CompressThreadGroup(tgrp, settings);
  }
  else {
    ThreadGroup tgrp (settings.iNumThreads, imgData, imgDataSz, f, outBuf);
    cmpTimeTotal = CompressThreadGroup(tgrp, settings);
  }

  double cmpTime = cmpTimeTotal / double(settings.iNumCompressions);
  return cmpTime;
}

static double CompressImageWithWorkerQueue(
  const unsigned char *imgData,
  const unsigned int imgDataSz,
  const SCompressionSettings &settings,
  unsigned char *outBuf
) {
  CompressionFunc f = ChooseFuncFromSettings(settings);
  CompressionFuncWithStats fStats = ChooseFuncFromSettingsWithStats(settings);

  double cmpTimeTotal = 0.0;
  if(fStats && settings.logStream) {
    WorkerQueue wq (
      settings.iNumCompressions,
      settings.iNumThreads,
      settings.iJobSize,
      imgData,
      imgDataSz,
      fStats,
      settings.logStream,
      outBuf
    );

    wq.Run();
    cmpTimeTotal = wq.GetStopWatch().TimeInMilliseconds();
  }
  else {
    WorkerQueue wq (
      settings.iNumCompressions,
      settings.iNumThreads,
      settings.iJobSize,
      imgData,
      imgDataSz,
      f,
      outBuf
    );

    wq.Run();
    cmpTimeTotal = wq.GetStopWatch().TimeInMilliseconds();
  }

  return cmpTimeTotal / double(settings.iNumCompressions);
}

template<typename PixelType>
CompressedImage *CompressImage(
  FasTC::Image<PixelType> *img, const SCompressionSettings &settings
) {
  if(!img) return NULL;

  const uint32 w = img->GetWidth();
  const uint32 h = img->GetHeight();

  CompressedImage *outImg = NULL;
  const unsigned int dataSz = w * h * 4;
  uint32 *data = new uint32[dataSz / 4];

  assert(dataSz > 0);

  // Allocate data based on the compression method
  uint32 cmpDataSz = CompressedImage::GetCompressedSize(dataSz, settings.format);

  // Make sure that we have RGBA data...
  img->ComputePixels();
  const PixelType *pixels = img->GetPixels();
  for(uint32 i = 0; i < img->GetNumPixels(); i++) {
    data[i] = pixels[i].Pack();
  }

  unsigned char *cmpData = new unsigned char[cmpDataSz];
  CompressImageData(reinterpret_cast<uint8 *>(data), w, h, cmpData, cmpDataSz, settings);

  outImg = new CompressedImage(w, h, settings.format, cmpData);
  
  delete [] data;
  delete [] cmpData;
  return outImg;
}

// !FIXME! Ideally, we wouldn't have to do this because there would be a way to instantiate this
// function in the header or using some fancy template metaprogramming. I can't think of the way 
// at the moment.
template CompressedImage *CompressImage(FasTC::Image<FasTC::Pixel> *, const SCompressionSettings &settings);

bool CompressImageData(
  const uint8 *data, 
  const uint32 width,
  const uint32 height,
  uint8 *compressedData,
  const uint32 cmpDataSz,
  const SCompressionSettings &settings
) { 

  uint32 dataSz = width * height * 4;

  // Make sure that platform supports SSE if they chose this
  // option...
  #ifndef HAS_SSE_41
  if(settings.bUseSIMD) {
    ReportError("Platform does not support SIMD!\n");
    return false;
  }
  #endif

  if(dataSz <= 0) {
    ReportError("No data sent to compress!");
    return false;
  }

  uint32 numThreads = settings.iNumThreads;
  if(settings.format == eCompressionFormat_PVRTC &&
     (settings.iNumThreads > 1 || settings.logStream)) {
    if(settings.iNumThreads > 1) {
      ReportError("WARNING - PVRTC compressor does not support multithreading.");
      numThreads = 1;
    }

    if(settings.logStream) {
      ReportError("WARNING - PVRTC compressor does not support stat collection.");
    }
  }

  // Allocate data based on the compression method
  uint32 compressedDataSzNeeded =
    CompressedImage::GetCompressedSize(dataSz, settings.format);

  if(compressedDataSzNeeded == 0) {
    ReportError("Unknown compression format");
    return false;
  }
  else if(compressedDataSzNeeded > cmpDataSz) {
    ReportError("Not enough space for compressed data!");
    return false;
  }

  CompressionFunc f = ChooseFuncFromSettings(settings);
  if(f) {

    double cmpMSTime = 0.0;

    if(numThreads > 1) {
      if(settings.bUseAtomics) {
        cmpMSTime = CompressImageWithAtomics(data, width, height, settings, compressedData);
      } else if(settings.iJobSize > 0) {
        cmpMSTime = CompressImageWithWorkerQueue(data, dataSz, settings, compressedData);
      } else {
        cmpMSTime = CompressImageWithThreads(data, dataSz, settings, compressedData);
      }
    }
    else {
      cmpMSTime = CompressImageInSerial(data, width, height, settings, compressedData);
    }

    // Report compression time
    fprintf(stdout, "Compression time: %0.3f ms\n", cmpMSTime);
  }
  else {
    ReportError("Could not find adequate compression function for specified settings");
    return false;
  }

  return true;
}

void YieldThread() {
  TCThread::Yield();
}
