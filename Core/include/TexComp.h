#ifndef _TEX_COMP_H_
#define _TEX_COMP_H_

#include "ImageFile.h"
#include "CompressedImage.h"

struct SCompressionSettings {
  SCompressionSettings(); // defaults
  ECompressionFormat format; 
  bool bUseSIMD;
  int iNumThreads;
  int iQuality;
  int iNumCompressions;
};

extern CompressedImage * CompressImage(
  const ImageFile &, 
  const SCompressionSettings &settings
);

typedef void (* CompressionFunc)(
  const unsigned char *inData,
  unsigned char *outData,
  unsigned int width,
  unsigned int height
);

extern double ComputePSNR(const CompressedImage &, const ImageFile &);

#endif //_TEX_COMP_H_
