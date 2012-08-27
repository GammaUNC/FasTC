#ifndef _IMAGE_LOADER_PNG_H_
#define _IMAGE_LOADER_PNG_H_

#include "ImageLoader.h"

class ImageLoaderPNG : public ImageLoader {
 public:
  ImageLoaderPNG(const unsigned char *rawData);
  virtual ~ImageLoaderPNG();

  virtual void ReadData();
};

#endif // _IMAGE_LOADER_H_
