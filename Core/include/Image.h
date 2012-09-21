#ifndef __TEXCOMP_IMAGE_H__
#define __TEXCOMP_IMAGE_H__

#include "TexCompTypes.h"
#include "TexComp.h"

// Forward declarations
class ImageLoader;

// Class definition
class Image {

 public:
  Image(const ImageLoader &);
  const uint8 *RawData() const { return m_PixelData; }

  CompressedImage *Compress(const SCompressionSettings &settings) const;
  double ComputePSNR(const CompressedImage &ci) const;

  uint32 GetWidth() const { return m_Width; }
  uint32 GetHeight() const { return m_Height; }

 private:
  uint32 m_Width;
  uint32 m_Height;

  uint8 *m_PixelData;
};

#endif // __TEXCOMP_IMAGE_H__
