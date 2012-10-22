#include "Image.h"
#include "ImageLoader.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

template <typename T>
static inline T sad( const T &a, const T &b ) {
  return (a > b)? a - b : b - a;
}

Image::Image(const CompressedImage &ci) 
  : m_Width(ci.GetWidth())
  , m_Height(ci.GetHeight())
{
	unsigned int bufSz = ci.GetWidth() * ci.GetHeight() * 4;
	m_PixelData = new uint8[ bufSz ];
	if(!m_PixelData) { fprintf(stderr, "%s\n", "Out of memory!"); return; }

	if(!ci.DecompressImage(m_PixelData, bufSz)) {
		fprintf(stderr, "Error decompressing image!\n");
		return;
	}
}

Image::Image(const ImageLoader &loader) 
  : m_PixelData(0)
  , m_Width(loader.GetWidth())
  , m_Height(loader.GetHeight())
{
  if(loader.GetImageData()) {
    m_PixelData = new uint8[ loader.GetImageDataSz() ];
    if(!m_PixelData) { fprintf(stderr, "%s\n", "Out of memory!"); return; }
    memcpy(m_PixelData, loader.GetImageData(), loader.GetImageDataSz());
  }
  else {
    fprintf(stderr, "%s\n", "Failed to get data from image loader!");
  }
}

CompressedImage *Image::Compress(const SCompressionSettings &settings) const {
  CompressedImage *outImg = NULL;
  const unsigned int dataSz = GetWidth() * GetHeight() * 4;

  assert(dataSz > 0);

  // Allocate data based on the compression method
  int cmpDataSz = 0;
  switch(settings.format) {
    case eCompressionFormat_DXT1: cmpDataSz = dataSz / 8;
    case eCompressionFormat_DXT5: cmpDataSz = dataSz / 4;
    case eCompressionFormat_BPTC: cmpDataSz = dataSz / 4;
  }

  unsigned char *cmpData = new unsigned char[cmpDataSz];
  CompressImageData(m_PixelData, dataSz, cmpData, cmpDataSz, settings);

  outImg = new CompressedImage(GetWidth(), GetHeight(), settings.format, cmpData);
  return outImg;
}

double Image::ComputePSNR(const CompressedImage &ci) const {
  unsigned int imageSz = 4 * GetWidth() * GetHeight();
  unsigned char *unCompData = new unsigned char[imageSz];
  if(!(ci.DecompressImage(unCompData, imageSz))) {
    fprintf(stderr, "%s\n", "Failed to decompress image.");
    return -1.0f;
  }

  const double wr = 1.0;
  const double wg = 1.0;
  const double wb = 1.0;
    
  double MSE = 0.0;
  for(int i = 0; i < imageSz; i+=4) {

    const unsigned char *pixelDataRaw = m_PixelData + i;
    const unsigned char *pixelDataUncomp = unCompData + i;

    double rawAlphaScale = double(pixelDataRaw[3]) / 255.0;
    double uncompAlphaScale = double(pixelDataUncomp[3]) / 255.0;
    double dr = double(sad(rawAlphaScale * pixelDataRaw[0], uncompAlphaScale * pixelDataUncomp[0])) * wr;
    double dg = double(sad(rawAlphaScale * pixelDataRaw[1], uncompAlphaScale * pixelDataUncomp[1])) * wg;
    double db = double(sad(rawAlphaScale * pixelDataRaw[2], uncompAlphaScale * pixelDataUncomp[2])) * wb;
    
    const double pixelMSE = 
      (double(dr) * double(dr)) + 
      (double(dg) * double(dg)) + 
      (double(db) * double(db));
    
    //fprintf(stderr, "Pixel MSE: %f\n", pixelMSE);
    MSE += pixelMSE;
  }

  MSE /= (double(GetWidth()) * double(GetHeight()));

  double MAXI = 
    (255.0 * wr) * (255.0 * wr) + 
    (255.0 * wg) * (255.0 * wg) + 
    (255.0 * wb) * (255.0 * wb);

  double PSNR = 10 * log10(MAXI/MSE);

  // Cleanup
  delete unCompData;
  return PSNR;
}
