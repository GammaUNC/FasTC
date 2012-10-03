#include "ImageFile.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

#include "ImageLoader.h"
#include "CompressedImage.h"
#include "Image.h"
#include "FileStream.h"

#ifdef PNG_FOUND
#  include "ImageLoaderPNG.h"
#endif

//////////////////////////////////////////////////////////////////////////////////////////
//
// Static helper functions
//
//////////////////////////////////////////////////////////////////////////////////////////

static inline void ReportError(const CHAR *msg) {
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

ImageFile::ImageFile(const CHAR *filename)
  : m_FileFormat(  DetectFileFormat(filename) )
  , m_Image(NULL)
{
  unsigned char *rawData = ReadFileData(filename);
  if(rawData) {
    m_Image = LoadImage(rawData);
    delete [] rawData;
  }
}

ImageFile::ImageFile(const CHAR *filename, EImageFileFormat format)
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

EImageFileFormat ImageFile::DetectFileFormat(const CHAR *filename) {

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

  const CHAR *ext = &filename[dotPos];

  if(strcmp(ext, ".png") == 0) {
    return eFileFormat_PNG;
  }
  return kNumImageFileFormats;
}

unsigned char *ImageFile::ReadFileData(const CHAR *filename) {
  FileStream fstr (filename, eFileMode_ReadBinary);
  if(fstr.Tell() < 0) {
    fprintf(stderr, "Error opening file for reading: %s\n", filename);
    return 0;
  }

  // Figure out the filesize.
  fstr.Seek(0, FileStream::eSeekPosition_End);
  uint64 fileSize = fstr.Tell();

  // Allocate data for file contents
  unsigned char *rawData = new unsigned char[fileSize];

  // Return stream to beginning of file
  fstr.Seek(0, FileStream::eSeekPosition_Beginning);
  assert(fstr.Tell() == 0);

  // Read all of the data
  int32 bytesRead = fstr.Read(rawData, fileSize);
  if(bytesRead != fileSize) {
    assert(!"We didn't read as much data as we thought we had!");
    fprintf(stderr, "Internal error: Incorrect file size assumption\n");
    return 0;
  }

  // Return the data..
  return rawData;
}

