#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

#include "ImageFile.h"
#include "ImageLoader.h"

#ifdef PNG_FOUND
#  include "ImageLoaderPNG.h"
#endif

//////////////////////////////////////////////////////////////////////////////////////////
//
// Static helper functions
//
//////////////////////////////////////////////////////////////////////////////////////////

static inline void ReportError(const char *msg) {
  fprintf(stderr, "ImageFile -- %s\n", msg);
}

template <typename T>
static inline T abs(const T &a) {
  return a > 0? a : -a;
}

template <typename T>
static inline T min(const T &a, const T &b) {
  return (a < b)? a : b;
}

static unsigned int GetChannelForPixel(
  const ImageLoader *loader, 
  unsigned int x, unsigned int y,
  int ch
) {
  unsigned int prec;
  const unsigned char *data;

  switch(ch) {
  case 0:
    prec = loader->GetRedChannelPrecision();
    data = loader->GetRedPixelData();
    break;

  case 1:
    prec = loader->GetGreenChannelPrecision();
    data = loader->GetGreenPixelData();
    break;

  case 2:
    prec = loader->GetBlueChannelPrecision();
    data = loader->GetBluePixelData();
    break;

  case 3:
    prec = loader->GetAlphaChannelPrecision();
    data = loader->GetAlphaPixelData();
    break;

  default:
    ReportError("Unspecified channel");
    return INT_MAX;
  }

  if(0 == prec)
    return 0;

  assert(x < loader->GetWidth());
  assert(y < loader->GetHeight());

  int pixelIdx = y * loader->GetWidth() + x;
  const unsigned int val = data[pixelIdx];
  
  if(prec < 8) {
    unsigned int ret = 0;
    for(unsigned int precLeft = 8; precLeft > 0; precLeft -= min(prec, abs(prec - precLeft))) {
      
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

//////////////////////////////////////////////////////////////////////////////////////////
//
// ImageFile implementation
//
//////////////////////////////////////////////////////////////////////////////////////////

ImageFile::ImageFile(const char *filename) : 
  m_PixelData(0),
  m_FileFormat(  DetectFileFormat(filename) )
{
  unsigned char *rawData = ReadFileData(filename);
  if(rawData) {
    LoadImage(rawData);
    delete [] rawData;
  }
}

ImageFile::ImageFile(const char *filename, EImageFileFormat format) :
  m_FileFormat(format),
  m_PixelData(0)
{
  unsigned char *rawData = ReadFileData(filename);
  if(rawData) {
    LoadImage(rawData);
    delete [] rawData;
  }
}

ImageFile::~ImageFile() {
  if(m_PixelData) {
    delete [] m_PixelData;
  }
}

EImageFileFormat ImageFile::DetectFileFormat(const char *filename) {

  int len = strlen(filename);
  if(len >= 256) {
    // !FIXME! Report Error...
    return kNumImageFileFormats;
  }

  int dotPos = len - 1;

  while(dotPos >= 0 && filename[dotPos--] != '.');

  if(dotPos < 0) {
    // !FIXME! Report Error.....
    return kNumImageFileFormats;
  }
  
  // consume the last character...
  dotPos++;

  const char *ext = &filename[dotPos];

  if(strcmp(ext, ".png") == 0) {
    return eFileFormat_PNG;
  }
  return kNumImageFileFormats;
}

bool ImageFile::LoadImage(const unsigned char *rawImageData) {

  ImageLoader *loader = NULL;
  switch(m_FileFormat) {

#ifdef PNG_FOUND
    case eFileFormat_PNG:
      {
	loader = new ImageLoaderPNG(rawImageData);
      }
      break;
#endif // PNG_FOUND

    default:
      fprintf(stderr, "Unable to load image: unknown file format.\n");
      break;
  }

  // Read the image data!
  if(!loader->ReadData())
    return false;

  m_Width = loader->GetWidth();
  m_Height = loader->GetHeight();

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

	  unsigned int redVal = GetChannelForPixel(loader, x, y, 0);
	  if(redVal == INT_MAX)
	    return false;

	  unsigned int greenVal = redVal;
	  unsigned int blueVal = redVal;

	  if(loader->GetGreenChannelPrecision() > 0) {
	    greenVal = GetChannelForPixel(loader, x, y, 1);
	    if(greenVal == INT_MAX)
	      return false;
	  }

	  if(loader->GetBlueChannelPrecision() > 0) {
	    blueVal = GetChannelForPixel(loader, x, y, 2);
	    if(blueVal == INT_MAX)
	      return false;
	  }

	  unsigned int alphaVal = 0xFF;
	  if(loader->GetAlphaChannelPrecision() > 0) {
	    alphaVal = GetChannelForPixel(loader, x, y, 3);
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

#ifdef _MSC_VER
unsigned char *ImageFile::ReadFileData(const char *filename) {
  //!FIXME! - Actually, implement me
  assert(!"Not implemented!");
}
#else
unsigned char *ImageFile::ReadFileData(const char *filename) {
  FILE *fp = fopen(filename, "rb");
  if(!fp) {
    fprintf(stderr, "Error opening file for reading: %s\n", filename);
    return 0;
  }

  // Check filesize
  long fileSize = 0;
  fseek(fp, 0, SEEK_END);
  fileSize = ftell(fp);

  // Allocate data for file contents
  unsigned char *rawData = new unsigned char[fileSize];

  // Return stream to beginning of file
  fseek(fp, 0, SEEK_SET);
  assert(ftell(fp) == 0);

  // Read all of the data
  size_t bytesRead = fread(rawData, 1, fileSize, fp);
  if(bytesRead != fileSize) {
    assert(!"We didn't read as much data as we thought we had!");
    fprintf(stderr, "Internal error: Incorrect file size assumption\n");
    return 0;
  }

  // Close the file pointer
  fclose(fp);

  // Return the data..
  return rawData;
}
#endif
