#include "ImageLoader.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

template <typename T>
static inline T min(const T &a, const T &b) {
  return (a > b)? b : a;
}

template <typename T>
static inline T abs(const T &a) {
  return (a > 0)? a : -a;
}

void ReportError(const char *str) {
  fprintf(stderr, "ImageLoader.cpp -- ERROR: %s\n", str);
}

unsigned int ImageLoader::GetChannelForPixel(uint32 x, uint32 y, uint32 ch) {
  uint32 prec;
  const uint8 *data;

  switch(ch) {
  case 0:
    prec = GetRedChannelPrecision();
    data = GetRedPixelData();
    break;

  case 1:
    prec = GetGreenChannelPrecision();
    data = GetGreenPixelData();
    break;

  case 2:
    prec = GetBlueChannelPrecision();
    data = GetBluePixelData();
    break;

  case 3:
    prec = GetAlphaChannelPrecision();
    data = GetAlphaPixelData();
    break;

  default:
    ReportError("Unspecified channel");
    return INT_MAX;
  }

  if(0 == prec)
    return 0;

  assert(x < GetWidth());
  assert(y < GetHeight());

  uint32 pixelIdx = y * GetWidth() + x;
  const uint32 val = data[pixelIdx];
  
  if(prec < 8) {
    uint32 ret = 0;
    for(uint32 precLeft = 8; precLeft > 0; precLeft -= min(prec, abs(prec - precLeft))) {
      
      if(prec > precLeft) {
	const int toShift = prec - precLeft;
	ret = ret << precLeft;
	ret |= val >> toShift;
      }
      else {
	ret = ret << prec;
	ret |= val;
      }

    }

    return ret;
  }
  else if(prec > 8) {
    const int toShift = prec - 8;
    return val >> toShift;
  }

  return val;
}

bool ImageLoader::LoadImage() {

  // Do we already have pixel data?
  if(m_PixelData)
    return true;

  // Read the image data!
  if(!ReadData())
    return false;

  m_Width = GetWidth();
  m_Height = GetHeight();

  // Create RGBA buffer 
  const unsigned int dataSz = 4 * m_Width * m_Height;

  m_PixelData = new unsigned char[dataSz];

  // Populate buffer in block stream order... make 
  // sure that width and height are aligned to multiples of four.
  const unsigned int aw = ((m_Width + 3) >> 2) << 2;
  const unsigned int ah = ((m_Height + 3) >> 2) << 2;

#ifndef NDEBUG
  if(aw != m_Width || ah != m_Height)
    fprintf(stderr, "Warning: Image dimension not multiple of four. Space will be filled with black.\n");
#endif

  int byteIdx = 0;
  for(int i = 0; i < ah; i+=4) {
    for(int j = 0; j < aw; j+= 4) {

      // For each block, visit the pixels in sequential order
      for(int y = i; y < i+4; y++) {
	for(int x = j; x < j+4; x++) {

	  if(y >= m_Height || x >= m_Width) {
	    m_PixelData[byteIdx++] = 0; // r
	    m_PixelData[byteIdx++] = 0; // g
	    m_PixelData[byteIdx++] = 0; // b
	    m_PixelData[byteIdx++] = 0; // a
	    continue;
	  }

	  unsigned int redVal = GetChannelForPixel(x, y, 0);
	  if(redVal == INT_MAX)
	    return false;

	  unsigned int greenVal = redVal;
	  unsigned int blueVal = redVal;

	  if(GetGreenChannelPrecision() > 0) {
	    greenVal = GetChannelForPixel(x, y, 1);
	    if(greenVal == INT_MAX)
	      return false;
	  }

	  if(GetBlueChannelPrecision() > 0) {
	    blueVal = GetChannelForPixel(x, y, 2);
	    if(blueVal == INT_MAX)
	      return false;
	  }

	  unsigned int alphaVal = 0xFF;
	  if(GetAlphaChannelPrecision() > 0) {
	    alphaVal = GetChannelForPixel(x, y, 3);
	    if(alphaVal == INT_MAX)
	      return false;
	  }

	  // Red channel
	  m_PixelData[byteIdx++] = redVal & 0xFF;

	  // Green channel
	  m_PixelData[byteIdx++] = greenVal & 0xFF;

	  // Blue channel
	  m_PixelData[byteIdx++] = blueVal & 0xFF;

	  // Alpha channel
	  m_PixelData[byteIdx++] = alphaVal & 0xFF;
	}
      }
    }
  }

  return true;
}
