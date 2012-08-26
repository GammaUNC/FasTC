#include "ImageFile.h"

ImageFile::ImageFile(const char *filename) : 
  m_PixelData(0)
{
  unsigned char *rawData = ReadFileData(filename);
  DetectFileFormat(filename);
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

void ImageFile::GetPixels() const {

}

EImageFileFormat ImageFile::DetectFileFormat() {
}

void ImageFile::LoadImage(const unsigend char *rawImageData) {
}

void ImageFile::LoadPNGImage(const unsigned char *rawImageData) {
}

#ifdef _MSC_VER
unsigned char *ImageFile::ReadFileData(const char *filename) {

}
#else
unsigned char *ImageFile::ReadFileData(const char *filename) {

}
#endif
