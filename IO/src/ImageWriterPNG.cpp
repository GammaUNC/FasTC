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

ImageWriterPNG::ImageWriterPNG(Image &im)
  : ImageWriter(im.GetWidth(), im.GetHeight(), im.RawData())
  , m_bBlockStreamOrder(im.GetBlockStreamOrder())
  , m_StreamPosition(0)
{
  im.ComputeRGBA();
  m_PixelData = reinterpret_cast<const uint8 *>(im.GetRGBA());
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
  for (uint32 y = 0; y < m_Height; ++y) {
    png_byte *row = (png_byte *)png_malloc (png_ptr, sizeof (uint8) * m_Width * pixel_size);

    row_pointers[y] = row;

    for (uint32 x = 0; x < m_Width; ++x) {
      if(m_bBlockStreamOrder) {
        for(uint32 ch = 0; ch < 4; ch++) {
          *row++ = GetChannelForPixel(x, y, ch);
        }
      } else {
        reinterpret_cast<uint32 *>(row)[x] =
          reinterpret_cast<const uint32 *>(m_PixelData)[y * m_Width + x];
      }
    }
  }
    
  png_set_write_fn(png_ptr, this, PNGStreamWriter::WriteDataToStream, PNGStreamWriter::FlushStream);
  png_set_rows (png_ptr, info_ptr, row_pointers);
  png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  for (uint32 y = 0; y < m_Height; y++) {
    png_free (png_ptr, row_pointers[y]);
  }
  png_free (png_ptr, row_pointers);
    
  png_destroy_write_struct (&png_ptr, &info_ptr);

  m_RawFileDataSz = m_StreamPosition;
  return true;
}
