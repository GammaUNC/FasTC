#include "ImageLoaderPNG.h"

#include <png.h>

ImageLoaderPNG::ImageLoaderPNG(const unsigned char *rawData) 
  : ImageLoader(rawData)
{
}

ImageLoaderPNG::~ImageLoaderPNG() {
}

void ImageLoaderPNG::ReadData() {
}
