#include "BC7Compressor.h"
#include "TexComp.h"
#include "ThreadGroup.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

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

CompressedImage * CompressImage(
  const ImageFile &img, 
  const SCompressionSettings &settings
) { 

  // Make sure that platform supports SSE if they chose this
  // option...
  #ifndef HAS_SSE_41
  if(settings.bUseSIMD) {
    ReportError("Platform does not support SIMD!\n");
    return NULL;
  }
  #endif

  const unsigned int dataSz = img.GetWidth() * img.GetHeight() * 4;

  // Allocate data based on the compression method
  int cmpDataSz = 0;
  switch(settings.format) {
    case eCompressionFormat_DXT1: cmpDataSz = dataSz / 8;
    case eCompressionFormat_DXT5: cmpDataSz = dataSz / 4;
    case eCompressionFormat_BPTC: cmpDataSz = dataSz / 4;
  }

  if(cmpDataSz == 0) {
    ReportError("Unknown compression format");
    return NULL;
  }

  CompressedImage *outImg = NULL;
  unsigned char *cmpData = new unsigned char[cmpDataSz];

  CompressionFunc f = ChooseFuncFromSettings(settings);
  if(f) {

    double cmpMSTime = 0.0;

    if(settings.iNumThreads > 1) {

      ThreadGroup tgrp (settings.iNumThreads, img, f, cmpData);
      if(!(tgrp.PrepareThreads())) {
        assert(!"Thread group failed to prepare threads?!");
        return NULL;
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

      cmpMSTime = cmpTimeTotal / double(settings.iNumCompressions);

      tgrp.CleanUpThreads();
    }
    else {
      double cmpTimeTotal = 0.0;
      for(int i = 0; i < settings.iNumCompressions; i++) {

        StopWatch stopWatch = StopWatch();
        stopWatch.Reset();
        stopWatch.Start();

        (*f)(img.GetPixels(), cmpData, img.GetWidth(), img.GetHeight());
        stopWatch.Stop();

        cmpTimeTotal += stopWatch.TimeInMilliseconds();
      }

      cmpMSTime = cmpTimeTotal / double(settings.iNumCompressions);
      outImg = new CompressedImage(img.GetWidth(), img.GetHeight(), settings.format, cmpData);
    }

    // Report compression time
    fprintf(stdout, "Compression time: %0.3f ms\n", cmpMSTime);
  }
  else {
    ReportError("Could not find adequate compression function for specified settings");
    delete [] cmpData;
    return NULL;
  }

  // Cleanup
  delete [] cmpData;

  return outImg;
} 
