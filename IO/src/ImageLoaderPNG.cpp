#include "ImageLoaderPNG.h"

#include <stdio.h>
#include <assert.h>

#include <png.h>

static void ReportError(const char *msg) {
  fprintf(stderr, "ERROR: ImageLoaderPNG -- %s\n", msg);
}

class PNGStreamReader {
public:
  static void ReadDataFromStream(
    png_structp png_ptr, 
    png_bytep outBytes, 
    png_size_t byteCountToRead
  ) {
    png_voidp io_ptr = png_get_io_ptr( png_ptr );
    if( io_ptr == NULL ) {
      ReportError("Read callback had invalid io pointer.\n");
      return;
    }

    ImageLoaderPNG &loader = *(ImageLoaderPNG *)(io_ptr);

    const unsigned char *stream = &(loader.m_RawData[loader.m_StreamPosition]);
    memcpy(outBytes, stream, byteCountToRead);

    loader.m_StreamPosition += byteCountToRead;
  }
};

ImageLoaderPNG::ImageLoaderPNG(const unsigned char *rawData) 
  : ImageLoader(rawData)
  , m_StreamPosition(8) // We start at position 8 because of PNG header.
{
}

ImageLoaderPNG::~ImageLoaderPNG() {
}

bool ImageLoaderPNG::ReadData() {
  
  const int kNumSigBytesToRead = 8;
  uint8 pngSigBuf[kNumSigBytesToRead];
  memcpy(pngSigBuf, m_RawData, kNumSigBytesToRead);

  const int numSigNoMatch = png_sig_cmp(pngSigBuf, 0, kNumSigBytesToRead);
  if(numSigNoMatch) {
    ReportError("Incorrect PNG signature");
    return false;
  }

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!png_ptr) {
    ReportError("Could not create read struct");
    return false;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if(!info_ptr) {
    ReportError("Could not create info struct");
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return false;
  }

  // Read from our buffer, not a file pointer...
  png_set_read_fn(png_ptr, this, PNGStreamReader::ReadDataFromStream);

  // Make sure to tell libpng how many bytes we've read...
  png_set_sig_bytes(png_ptr, kNumSigBytesToRead);

  png_read_info(png_ptr, info_ptr);

  int bitDepth = 0;
  int colorType = -1;

  if( 1 != png_get_IHDR(png_ptr, info_ptr, 
      (png_uint_32 *)(&m_Width), (png_uint_32 *)(&m_Height), 
      &bitDepth, &colorType, 
      NULL, NULL, NULL) 
  ) {
    ReportError("Could not read PNG header");
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return false;
  }

  if(bitDepth != 8) {
    ReportError("Only 8-bit images currently supported.");
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return false;
  }

  const int numPixels = m_Width * m_Height;
  png_uint_32 bpr = png_get_rowbytes(png_ptr, info_ptr);
  png_bytep rowData = new png_byte[bpr];

  switch(colorType) {
  default:
  case PNG_COLOR_TYPE_PALETTE:
    ReportError("PNG color type unsupported");
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return false;

  case PNG_COLOR_TYPE_GRAY: {
      m_RedChannelPrecision = bitDepth;
      m_RedData = new unsigned char[numPixels];

      for(uint32 i = 0; i < m_Height; i++) {
	
	png_read_row(png_ptr, rowData, NULL);

	unsigned int rowOffset = i * m_Width;
	
	unsigned int byteIdx = 0;
	for(uint32 j = 0; j < m_Width; j++) {
 	  m_RedData[rowOffset + j] = rowData[byteIdx++];
	}

	assert(byteIdx == bpr);
      }
    }
    break;

  case PNG_COLOR_TYPE_RGB:
      m_RedChannelPrecision = bitDepth;
      m_RedData = new unsigned char[numPixels];
      m_GreenChannelPrecision = bitDepth;
      m_GreenData = new unsigned char[numPixels];
      m_BlueChannelPrecision = bitDepth;
      m_BlueData = new unsigned char[numPixels];

      for(uint32 i = 0; i < m_Height; i++) {
	
	png_read_row(png_ptr, rowData, NULL);

	unsigned int rowOffset = i * m_Width;
	
	unsigned int byteIdx = 0;
	for(uint32 j = 0; j < m_Width; j++) {
 	  m_RedData[rowOffset + j] = rowData[byteIdx++];
	  m_GreenData[rowOffset + j] = rowData[byteIdx++];
	  m_BlueData[rowOffset + j] = rowData[byteIdx++];
	}

	assert(byteIdx == bpr);
      }
    break;

  case PNG_COLOR_TYPE_RGB_ALPHA:
      m_RedChannelPrecision = bitDepth;
      m_RedData = new unsigned char[numPixels];
      m_GreenChannelPrecision = bitDepth;
      m_GreenData = new unsigned char[numPixels];
      m_BlueChannelPrecision = bitDepth;
      m_BlueData = new unsigned char[numPixels];
      m_AlphaChannelPrecision = bitDepth;
      m_AlphaData = new unsigned char[numPixels];

      for(uint32 i = 0; i < m_Height; i++) {
	
	png_read_row(png_ptr, rowData, NULL);

	unsigned int rowOffset = i * m_Width;
	
	unsigned int byteIdx = 0;
	for(uint32 j = 0; j < m_Width; j++) {
 	  m_RedData[rowOffset + j] = rowData[byteIdx++];
	  m_GreenData[rowOffset + j] = rowData[byteIdx++];
	  m_BlueData[rowOffset + j] = rowData[byteIdx++];
	  m_AlphaData[rowOffset + j] = rowData[byteIdx++];
	}

	assert(byteIdx == bpr);
      }
    break;

  case PNG_COLOR_TYPE_GRAY_ALPHA:
      m_RedChannelPrecision = bitDepth;
      m_RedData = new unsigned char[numPixels];
      m_AlphaChannelPrecision = bitDepth;
      m_AlphaData = new unsigned char[numPixels];

      for(uint32 i = 0; i < m_Height; i++) {
	
	png_read_row(png_ptr, rowData, NULL);

	unsigned int rowOffset = i * m_Width;
	
	unsigned int byteIdx = 0;
	for(uint32 j = 0; j < m_Width; j++) {
 	  m_RedData[rowOffset + j] = rowData[byteIdx++];
	  m_AlphaData[rowOffset + j] = rowData[byteIdx++];
	}

	assert(byteIdx == bpr);
      }
    break;
  }

  // Cleanup
  delete [] rowData;
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  return true;
}
