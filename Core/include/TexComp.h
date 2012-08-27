#ifndef _TEX_COMP_H_
#define _TEX_COMP_H_

#include "ImageFile.h"
#include "CompressedImage.h"

struct SCompressionSettings {
  SCompressionSettings(); // defaults
  ECompressionFormat format; 
  bool bUseSIMD;
  int iNumThreads;
};

extern void CompressImage(
  const ImageFile &, 
  CompressedImage &, 
  const SCompressionSettings &settings
);

typedef void (* CompressionFunc)(
  const unsigned char *inData,
  unsigned char *outData,
  unsigned int width,
  unsigned int height
);

#endif //_TEX_COMP_H_
