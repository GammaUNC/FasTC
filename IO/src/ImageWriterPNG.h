#ifndef _IMAGE_WRITER_PNG_H_
#define _IMAGE_WRITER_PNG_H_

#include "ImageWriter.h"

// Forward Declare
class Image;
class ImageWriterPNG : public ImageWriter {
 public:
  ImageWriterPNG(const Image &);
  virtual ~ImageWriterPNG() { }

  virtual bool WriteImage();
 private:
	uint32 m_StreamPosition;
	uint32 m_TotalBytesWritten;
	friend class PNGStreamWriter;
};

#endif // _IMAGE_LOADER_H_
