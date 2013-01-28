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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "BC7Compressor.h"
#include "WorkerQueue.h"
#include "ThreadGroup.h"

#include "ImageFile.h"
#include "Image.h"


template <typename T>
static T min(const T &a, const T &b) {
  return (a < b)? a : b;
}

template <typename T>
static T max(const T &a, const T &b) {
  return (a > b)? a : b;
}

template <typename T>
static void clamp(T &x, const T &minX, const T &maxX) {
  x = max(min(maxX, x), minX);
}

template <typename T>
static inline T sad(const T &a, const T &b) {
  return (a > b)? a - b : b - a;
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
       return BC7C::CompressImageBC7Stats;
    }
    break;

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
      else {
#endif
        return BC7C::CompressImageBC7;
#ifdef HAS_SSE_41
      }
#endif
    }
    break;

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
  const unsigned char *imgData,
  const unsigned int imgDataSz,
  const SCompressionSettings &settings,
  unsigned char *outBuf
) {
  CompressionFunc f = ChooseFuncFromSettings(settings);
  CompressionFuncWithStats fStats = ChooseFuncFromSettingsWithStats(settings);

  double cmpTimeTotal = 0.0;

  for(int i = 0; i < settings.iNumCompressions; i++) {

    StopWatch stopWatch = StopWatch();
    stopWatch.Reset();
    stopWatch.Start();

    // !FIXME! We're assuming that we have 4x4 blocks here...
    if(fStats && settings.pStatManager) {
      (*fStats)(imgData, outBuf, imgDataSz / 16, 4, *(settings.pStatManager));
    }
    else {
      (*f)(imgData, outBuf, imgDataSz / 16, 4);
    }

    stopWatch.Stop();

    cmpTimeTotal += stopWatch.TimeInMilliseconds();
  }

  double cmpTime = cmpTimeTotal / double(settings.iNumCompressions);
  return cmpTime;
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
  if(fStats && settings.pStatManager) {
    ThreadGroup tgrp (settings.iNumThreads, imgData, imgDataSz, fStats, *(settings.pStatManager), outBuf);
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
  if(fStats && settings.pStatManager) {
    WorkerQueue wq (
      settings.iNumCompressions,
      settings.iNumThreads,
      settings.iJobSize,
      imgData,
      imgDataSz,
      fStats,
      *(settings.pStatManager),
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

bool CompressImageData(
  const unsigned char *data, 
  const unsigned int dataSz,
  unsigned char *cmpData,
  const unsigned int cmpDataSz,
  const SCompressionSettings &settings
) { 

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

  // Allocate data based on the compression method
  uint32 cmpDataSzNeeded = 0;
  switch(settings.format) {
    default: assert(!"Not implemented!"); // Fall through V
    case eCompressionFormat_DXT1: cmpDataSzNeeded = dataSz / 8; break;
    case eCompressionFormat_DXT5: cmpDataSzNeeded = dataSz / 4; break;
    case eCompressionFormat_BPTC: cmpDataSzNeeded = dataSz / 4; break;
  }

  if(cmpDataSzNeeded == 0) {
    ReportError("Unknown compression format");
    return false;
  }
  else if(cmpDataSzNeeded > cmpDataSz) {
    ReportError("Not enough space for compressed data!");
    return false;
  }

  CompressionFunc f = ChooseFuncFromSettings(settings);
  if(f) {

    double cmpMSTime = 0.0;

    if(settings.iNumThreads > 1) {
      if(settings.iJobSize > 0)
	cmpMSTime = CompressImageWithWorkerQueue(data, dataSz, settings, cmpData);
      else
	cmpMSTime = CompressImageWithThreads(data, dataSz, settings, cmpData);
    }
    else {
      cmpMSTime = CompressImageInSerial(data, dataSz, settings, cmpData);
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
