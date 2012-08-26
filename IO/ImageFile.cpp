#include "ImageFile.h"

#ifdef _MSC_VER
ImageFile::ImageFile(const char *filename) {
}

ImageFile::~ImageFile() {
}

void ImageFile::GetPixels() const {

}

#else
#include <stdlib.h>

ImageFile::ImageFile(const char *filename) {
}

ImageFile::~ImageFile() {
}

void ImageFile::GetPixels() const {

}

#endif

