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

#include "CompressedImage.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "TexCompTypes.h"
#include "BC7Compressor.h"
#include "PVRTCCompressor.h"

CompressedImage::CompressedImage( const CompressedImage &other )
  : Image(other)
  , m_Format(other.m_Format)
  , m_RGBAData(0)
{
  if(other.m_RGBAData) {
    m_RGBAData = new uint32[GetWidth() * GetHeight()];
    memcpy(m_RGBAData, other.m_RGBAData, sizeof(uint32) * GetWidth() * GetHeight());
  }
}

CompressedImage::CompressedImage(
  const unsigned int width,
  const unsigned int height,
  const ECompressionFormat format,
  const unsigned char *data
)
  : Image(width, height, NULL, format != eCompressionFormat_PVRTC)
  , m_Format(format)
  , m_RGBAData(0)
{
  m_DataSz = GetCompressedSize(GetWidth() * GetHeight() * 4, m_Format);
  if(m_DataSz > 0) {
    assert(!m_Data);
    m_Data = new unsigned char[m_DataSz];
    memcpy(m_Data, data, m_DataSz);
  }
}

CompressedImage &CompressedImage::operator=(const CompressedImage &other) {
  Image::operator=(other);
  m_Format = other.m_Format;
  if(other.m_RGBAData) {
    m_RGBAData = new uint32[GetWidth() * GetHeight()];
    memcpy(m_RGBAData, other.m_RGBAData, sizeof(uint32) * GetWidth() * GetHeight());
  }
  return *this;
}

CompressedImage::~CompressedImage() {
  if(m_RGBAData) {
    delete m_RGBAData;
    m_RGBAData = NULL;
  }
}

bool CompressedImage::DecompressImage(unsigned char *outBuf, unsigned int outBufSz) const {

  // First make sure that we have enough data
  uint32 dataSz = GetUncompressedSize(m_DataSz, m_Format);
  if(dataSz > outBufSz) {
    fprintf(stderr, "Not enough space to store entire decompressed image! "
                    "Got %d bytes, but need %d!\n", outBufSz, dataSz);
    assert(false);
    return false;
  }

  DecompressionJob dj (m_Data, outBuf, GetWidth(), GetHeight());
  switch(m_Format) {
    case eCompressionFormat_PVRTC:
    {
      PVRTCC::Decompress(dj);
    }
    break;

    case eCompressionFormat_BPTC: 
    { 
      BC7C::Decompress(dj);
    }
    break;

    default:
    {
      const char *errStr = "Have not implemented decompression method.";
      fprintf(stderr, "%s\n", errStr);
      assert(!errStr);
    }
    return false;
  }

  return true;
}

void CompressedImage::ComputeRGBA() {

  if(m_RGBAData) {
    delete m_RGBAData;
  }
  m_RGBAData = new uint32[GetWidth() * GetHeight()];

  uint8 *pixelData = reinterpret_cast<uint8 *>(m_RGBAData);
  DecompressImage(pixelData, GetWidth() * GetHeight() * 4);
}

uint32 CompressedImage::GetCompressedSize(uint32 uncompressedSize, ECompressionFormat format) {
  assert(uncompressedSize % 8 == 0);

  uint32 cmpDataSzNeeded = 0;
  switch(format) {
  default:
    assert(!"Not implemented!");
    // Fall through V
  case eCompressionFormat_DXT1:
  case eCompressionFormat_PVRTC:
    cmpDataSzNeeded = uncompressedSize / 8;
    break;

  case eCompressionFormat_DXT5:
  case eCompressionFormat_BPTC:
    cmpDataSzNeeded = uncompressedSize / 4;
    break;
  }

  return cmpDataSzNeeded;
}

