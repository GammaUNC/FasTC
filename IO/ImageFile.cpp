#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ImageFile.h"

#ifdef PNG_FOUND
#  include "ImageLoaderPNG.h"
#endif

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

  return false;
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
