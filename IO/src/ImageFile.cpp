#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

#include "TexComp.h"
#include "ImageFile.h"
#include "ImageLoader.h"
#include "CompressedImage.h"
#include "Image.h"

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

//////////////////////////////////////////////////////////////////////////////////////////
//
// ImageFile implementation
//
//////////////////////////////////////////////////////////////////////////////////////////

ImageFile::ImageFile(const char *filename)
  : m_FileFormat(  DetectFileFormat(filename) )
  , m_Image(NULL)
{
  unsigned char *rawData = ReadFileData(filename);
  if(rawData) {
    m_Image = LoadImage(rawData);
    delete [] rawData;
  }
}

ImageFile::ImageFile(const char *filename, EImageFileFormat format)
  : m_FileFormat(format)
  , m_Image(NULL)
{
  unsigned char *rawData = ReadFileData(filename);
  if(rawData) {
    m_Image = LoadImage(rawData);
    delete [] rawData;
  }
}

ImageFile::~ImageFile() { 
  if(m_Image) {
    delete m_Image;
    m_Image = NULL;
  }
}

Image *ImageFile::LoadImage(const unsigned char *rawImageData) const {

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
      return NULL;
  }

  if(!loader)
    return NULL;

  if(!(loader->LoadImage())) {
    fprintf(stderr, "Unable to load image!\n");
    delete loader;
    return NULL;
  }

  Image *i = new Image(*loader);
  delete loader;

  return i;
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
