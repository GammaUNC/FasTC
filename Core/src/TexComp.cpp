#include "TexComp.h"

#include <stdlib.h>
#include <stdio.h>

SCompressionSettings:: SCompressionSettings()
  : format(eCompressionFormat_BPTC)
  , bUseSIMD(false)
  , iNumThreads(1)
{}

static CompressionFunc ChooseFuncFromSettings(const SCompressionSettings &s) {
  return NULL;
}

static void ReportError(const char *msg) {
  fprintf(stderr, "TexComp -- %s\n", msg);
}

void CompressImage(
  const ImageFile &img, 
  CompressedImage &outImg, 
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
    return;
  }

  unsigned char *cmpData = new unsigned char[cmpDataSz];

  CompressionFunc f = ChooseFuncFromSettings(settings);
  if(f) {
    (*f)(img.GetPixels(), cmpData, img.GetWidth(), img.GetHeight());
    outImg = CompressedImage(img.GetWidth(), img.GetHeight(), settings.format, cmpData);
  }
  else {
    ReportError("Could not find adequate compression function for specified settings");
    // return
  }

  // Cleanup
  delete [] cmpData;
} 
