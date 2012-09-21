#ifndef _IMAGE_FILE_H_ 
#define _IMAGE_FILE_H_ 

#include "ImageFileFormat.h"

// Forward declare
class CompressedImage;

// Class definition
class ImageFile {

public:

  ImageFile(const char *filename);
  ImageFile(const char *filename, EImageFileFormat format);
  ~ImageFile();

  unsigned int GetWidth() const { return m_Width; }
  unsigned int GetHeight() const { return m_Height; }
  CompressedImage *Compress(const SCompressionSettings &) const;

  const unsigned char *RawData() const { return m_PixelData; }

 private:
  unsigned int m_Handle;
  unsigned int m_Width;
  unsigned int m_Height;
  unsigned char *m_PixelData;
  const EImageFileFormat m_FileFormat;

  static unsigned char *ReadFileData(const char *filename);
  static EImageFileFormat DetectFileFormat(const char *filename);

  bool LoadImage(const unsigned char *rawImageData);
};
#endif // _IMAGE_FILE_H_ 
