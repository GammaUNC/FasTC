#include "TexComp.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "BC7Compressor.h"
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
  const CompressionFunc f,
  unsigned char *outBuf
) {
  double cmpTimeTotal = 0.0;

  for(int i = 0; i < settings.iNumCompressions; i++) {

    StopWatch stopWatch = StopWatch();
    stopWatch.Reset();
    stopWatch.Start();

    // !FIXME! We're assuming that we have 4x4 blocks here...
    (*f)(imgData, outBuf, imgDataSz / 16, 4);

    stopWatch.Stop();

    cmpTimeTotal += stopWatch.TimeInMilliseconds();
  }

  double cmpTime = cmpTimeTotal / double(settings.iNumCompressions);
  return cmpTime;
}

static double CompressImageWithThreads(
  const unsigned char *imgData,
  const unsigned int imgDataSz,
  const SCompressionSettings &settings,
  const CompressionFunc f,
  unsigned char *outBuf
) {

  ThreadGroup tgrp (settings.iNumThreads, imgData, imgDataSz, f, outBuf);
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

  double cmpTime = cmpTimeTotal / double(settings.iNumCompressions);
  return cmpTime;
}

static double CompressImageWithWorkerQueue(
  const ImageFile &img,
  const SCompressionSettings &settings,
  const CompressionFunc f,
  unsigned char *outBuf
) {
  return 0.0;
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
  int cmpDataSzNeeded = 0;
  switch(settings.format) {
    case eCompressionFormat_DXT1: cmpDataSzNeeded = dataSz / 8;
    case eCompressionFormat_DXT5: cmpDataSzNeeded = dataSz / 4;
    case eCompressionFormat_BPTC: cmpDataSzNeeded = dataSz / 4;
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
      cmpMSTime = CompressImageWithThreads(data, dataSz, settings, f, cmpData);
    }
    else {
      cmpMSTime = CompressImageInSerial(data, dataSz, settings, f, cmpData);
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
