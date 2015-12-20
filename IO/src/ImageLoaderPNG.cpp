/* FasTC
 * Copyright (c) 2012 University of North Carolina at Chapel Hill. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its documentation for educational, 
 * research, and non-profit purposes, without fee, and without a written agreement is hereby granted, 
 * provided that the above copyright notice, this paragraph, and the following four paragraphs appear 
 * in all copies.
 *
 * Permission to incorporate this software into commercial products may be obtained by contacting the 
 * authors or the Office of Technology Development at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of North Carolina at Chapel Hill. 
 * The software program and documentation are supplied "as is," without any accompanying services from the 
 * University of North Carolina at Chapel Hill or the authors. The University of North Carolina at Chapel Hill 
 * and the authors do not warrant that the operation of the program will be uninterrupted or error-free. The 
 * end-user understands that the program was developed for research purposes and is advised not to rely 
 * exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE AUTHORS BE LIABLE TO ANY PARTY FOR 
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE 
 * USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE 
 * AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, 
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY 
 * OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
 * ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Please send all BUG REPORTS to <pavel@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Pavel Krajcevski
 * Dept of Computer Science
 * 201 S Columbia St
 * Frederick P. Brooks, Jr. Computer Science Bldg
 * Chapel Hill, NC 27599-3175
 * USA
 * 
 * <http://gamma.cs.unc.edu/FasTC/>
 */

#include "ImageLoaderPNG.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>

#include <png.h>

static void ReportError(const char *msg) {
  fprintf(stderr, "ERROR: ImageLoaderPNG -- %s\n", msg);
}

class PNGStreamReader {
public:
  static void ReadDataFromStream(png_structp png_ptr, 
           png_bytep outBytes, png_size_t byteCountToRead
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
  png_size_t bpr = png_get_rowbytes(png_ptr, info_ptr);
  png_bytep rowData = new png_byte[bpr];

  switch(colorType) {
    default:
      ReportError("PNG color type unsupported");
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      delete [] rowData;
      return false;

    case PNG_COLOR_TYPE_PALETTE:
    {
      m_RedChannelPrecision = bitDepth;
      m_RedData = new unsigned char[numPixels];
      m_GreenChannelPrecision = bitDepth;
      m_GreenData = new unsigned char[numPixels];
      m_BlueChannelPrecision = bitDepth;
      m_BlueData = new unsigned char[numPixels];

      png_colorp palette;
      int nPaletteEntries;
      png_uint_32 ret = png_get_PLTE(png_ptr, info_ptr, &palette, &nPaletteEntries);
      if(ret != PNG_INFO_PLTE) {
        memset(m_BlueData, 0, numPixels);
        memset(m_RedData, 0, numPixels);
        memset(m_GreenData, 0, numPixels);
        assert(!"Couldn't find PLTE chunk");
        break;
      }

      for(uint32 i = 0; i < m_Height; i++) {
        png_read_row(png_ptr, rowData, NULL);
        unsigned int rowOffset = i * m_Width;
        for(uint32 j = 0; j < m_Width; j++) {
          assert(rowData[j] < nPaletteEntries);
          const png_color &c = palette[::std::min<unsigned char>(rowData[j], nPaletteEntries - 1)];
          m_RedData[rowOffset + j] = c.red;
          m_GreenData[rowOffset + j] = c.green;
          m_BlueData[rowOffset + j] = c.blue;
        }
      }
    }
    break;

    case PNG_COLOR_TYPE_GRAY: {
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
          m_RedData[rowOffset + j] = rowData[byteIdx];
          m_GreenData[rowOffset + j] = rowData[byteIdx];
          m_BlueData[rowOffset + j] = rowData[byteIdx];
          byteIdx++;
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
