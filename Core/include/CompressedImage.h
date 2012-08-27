#ifndef _COMPRESSED_IMAGE_H_
#define _COMPRESSED_IMAGE_H_

enum ECompressionFormat {
  eCompressionFormat_DXT1,
  eCompressionFormat_DXT5,
  eCompressionFormat_BPTC,

  kNumCompressionFormats
};

class CompressedImage {

 private:
  unsigned char *m_Data;
  unsigned int m_DataSz;
  unsigned int m_Width;
  unsigned int m_Height;
  ECompressionFormat m_Format;

  void InitData(const unsigned char *withData);
 public:
  CompressedImage(
    const unsigned int width, 
    const unsigned int height, 
    const ECompressionFormat format, 
    const unsigned char *data
  );

  CompressedImage( const CompressedImage &other );
  ~CompressedImage();
};

#endif // _COMPRESSED_IMAGE_H_
