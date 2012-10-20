#include "ImageWriterPNG.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Image.h"

#include <png.h>

class PNGStreamWriter {
public:
  static void WriteDataToStream(
    png_structp png_ptr, 
    png_bytep outBytes, 
    png_size_t byteCountToWrite
  ) {
    png_voidp io_ptr = png_get_io_ptr( png_ptr );
    if( io_ptr == NULL ) {
      fprintf(stderr, "Write callback had invalid io pointer.\n");
      return;
    }

    ImageWriterPNG &writer = *(ImageWriterPNG *)(io_ptr);

		while(writer.m_StreamPosition + byteCountToWrite > writer.m_RawFileDataSz) {
			uint8 *newData = new uint8[writer.m_RawFileDataSz << 1];
			memcpy(newData, writer.m_RawFileData, writer.m_RawFileDataSz);
			writer.m_RawFileDataSz <<= 1;
			delete writer.m_RawFileData;
			writer.m_RawFileData = newData;
		}

		unsigned char *stream = &(writer.m_RawFileData[writer.m_StreamPosition]);
    memcpy(stream, outBytes, byteCountToWrite);

    writer.m_StreamPosition += byteCountToWrite;
  }

	static void FlushStream(png_structp png_ptr) { /* Do nothing... */ }

};

ImageWriterPNG::ImageWriterPNG(const Image &im)
	: ImageWriter(im.GetWidth(), im.GetHeight(), im.RawData())
  , m_StreamPosition(0)
	, m_TotalBytesWritten(0)
{
}

bool ImageWriterPNG::WriteImage() {

	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_byte ** row_pointers = NULL;
	int pixel_size = 4;
	int depth = 8;
    
	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		return false;
	}
    
	info_ptr = png_create_info_struct (png_ptr);
	if (info_ptr == NULL) {
		png_destroy_write_struct (&png_ptr, &info_ptr);
		return false;
	}
    
	/* Set image attributes. */

	png_set_IHDR (png_ptr,
								info_ptr,
								m_Width,
								m_Height,
								depth,
								PNG_COLOR_TYPE_RGBA,
								PNG_INTERLACE_NONE,
								PNG_COMPRESSION_TYPE_DEFAULT,
								PNG_FILTER_TYPE_DEFAULT);
    
	/* Initialize rows of PNG. */

	row_pointers = (png_byte **)png_malloc (png_ptr, m_Height * sizeof (png_byte *));
	for (int y = 0; y < m_Height; ++y) {
		png_byte *row = (png_byte *)png_malloc (png_ptr, sizeof (uint8) * m_Width * pixel_size);

		row_pointers[y] = row;

		for (int x = 0; x < m_Width; ++x) {
			for(int ch = 0; ch < 4; ch++) {
				*row++ = GetChannelForPixel(x, y, ch);
			}
    }
	}
    
	png_set_write_fn(png_ptr, this, PNGStreamWriter::WriteDataToStream, PNGStreamWriter::FlushStream);
	png_set_rows (png_ptr, info_ptr, row_pointers);
	png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	for (int y = 0; y < m_Height; y++) {
		png_free (png_ptr, row_pointers[y]);
	}
	png_free (png_ptr, row_pointers);
    
	png_destroy_write_struct (&png_ptr, &info_ptr);

	m_RawFileDataSz = m_StreamPosition;
	return true;
}
