#include "BC7Compressor.h"
#include "TexComp.h"
#include "ThreadGroup.h"

#include <stdlib.h>
#include <stdio.h>

SCompressionSettings:: SCompressionSettings()
  : format(eCompressionFormat_BPTC)
  , bUseSIMD(false)
  , iNumThreads(1)
{}

static CompressionFunc ChooseFuncFromSettings(const SCompressionSettings &s) {
  switch(s.format) {
    case eCompressionFormat_BPTC:
    {
      if(s.bUseSIMD) {
	return BC7C::CompressImageBC7SIMD;
      }
      else {
	return BC7C::CompressImageBC7;
      }
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

    StopWatch stopWatch = StopWatch();

    if(settings.iNumThreads > 1) {

      ThreadGroup tgrp (settings.iNumThreads, img, f, cmpData);

      tgrp.Start();
      tgrp.Join();

      stopWatch = tgrp.GetStopWatch();
    }
    else {
      stopWatch.Start();
      (*f)(img.GetPixels(), cmpData, img.GetWidth(), img.GetHeight());
      stopWatch.Stop();
      outImg = new CompressedImage(img.GetWidth(), img.GetHeight(), settings.format, cmpData);
    }

    // Report compression time
    fprintf(stdout, "Compression time: %0.3f ms\n", stopWatch.TimeInMilliseconds());
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
