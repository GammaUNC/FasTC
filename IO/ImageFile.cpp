#include "ImageFile.h"
#include <string.h>

ImageFile::ImageFile(const char *filename) : 
  m_PixelData(0),
  m_FileFormat(  DetectFileFormat(filename) )
{
  unsigned char *rawData = ReadFileData(filename);
  LoadImage(rawData);
  delete [] rawData;
}

ImageFile::ImageFile(const char *filename, EImageFileFormat format) :
  m_FileFormat(format),
  m_PixelData(0)
{
  unsigned char *rawData = ReadFileData(filename);
  LoadImage(rawData);
  delete [] rawData;
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
  return false;
}

#ifdef _MSC_VER
unsigned char *ImageFile::ReadFileData(const char *filename) {

}
#else
unsigned char *ImageFile::ReadFileData(const char *filename) {

}
#endif
