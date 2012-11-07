#ifndef _IMAGE_FILE_H_ 
#define _IMAGE_FILE_H_ 

#include "TexCompTypes.h"
#include "ImageFileFormat.h"

// Forward declare
class Image;
class CompressedImage;
struct SCompressionSettings;

// Class definition
class ImageFile {

public:

  ImageFile(const char *filename);
  ImageFile(const char *filename, EImageFileFormat format);
	ImageFile(const char *filename, EImageFileFormat format, const Image &);
  ~ImageFile();

  unsigned int GetWidth() const { return m_Width; }
  unsigned int GetHeight() const { return m_Height; }
  CompressedImage *Compress(const SCompressionSettings &) const;
  Image *GetImage() const { return m_Image; }

	bool Load();
	bool Write();

 private:
	static const unsigned int kMaxFilenameSz = 256;
	char m_Filename[kMaxFilenameSz];
  unsigned int m_Handle;
  unsigned int m_Width;
  unsigned int m_Height;

  Image *m_Image;
  
  const EImageFileFormat m_FileFormat;

  static unsigned char *ReadFileData(const CHAR *filename);
  static bool WriteImageDataToFile(const uint8 *data, const uint32 dataSz, const CHAR *filename);
  static EImageFileFormat DetectFileFormat(const CHAR *filename);

  Image *LoadImage(const unsigned char *rawImageData) const;
};
#endif // _IMAGE_FILE_H_ 
