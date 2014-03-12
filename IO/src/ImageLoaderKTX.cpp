/* FasTC
 * Copyright (c) 2014 University of North Carolina at Chapel Hill.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for educational, research, and non-profit purposes, without
 * fee, and without a written agreement is hereby granted, provided that the
 * above copyright notice, this paragraph, and the following four paragraphs
 * appear in all copies.
 *
 * Permission to incorporate this software into commercial products may be
 * obtained by contacting the authors or the Office of Technology Development
 * at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of
 * North Carolina at Chapel Hill. The software program and documentation are
 * supplied "as is," without any accompanying services from the University of
 * North Carolina at Chapel Hill or the authors. The University of North
 * Carolina at Chapel Hill and the authors do not warrant that the operation of
 * the program will be uninterrupted or error-free. The end-user understands
 * that the program was developed for research purposes and is advised not to
 * rely exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE
 * AUTHORS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF
 * THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA
 * AT CHAPEL HILL OR THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY
 * DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON
 * AN "AS IS" BASIS, AND THE UNIVERSITY  OF NORTH CAROLINA AT CHAPEL HILL AND
 * THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
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

#include "ImageLoaderKTX.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "TexCompTypes.h"
#include "ScopedAllocator.h"

#include "GLDefines.h"
#include "Image.h"
#include "CompressedImage.h"

class ByteReader {
 private:
  const uint8 *m_Data;
  uint32 m_BytesLeft;
 public:
  ByteReader(const uint8 *data, uint32 bytesExpected)
    : m_Data(data), m_BytesLeft(bytesExpected)
  { }

  bool Advance(uint32 nBytes) {
    if(nBytes > m_BytesLeft) {
      assert(!"Cannot read any more, unexpected bytes!");
      return false;
    }

    m_Data += nBytes;
    m_BytesLeft -= nBytes;
    return true;
  }

  operator const uint8 *() {
    return m_Data;
  }

  const uint8 *GetData() const { return m_Data; }
  uint32 GetBytesLeft() const { return m_BytesLeft; }
};

class IntLoader {
 public:
  virtual uint32 ReadInt(const uint8 *data) const = 0;
  bool LoadInt(ByteReader &data, uint32 &result) const {
    result = ReadInt(data);
    data.Advance(4);
    return true;
  }
  virtual ~IntLoader() { }
};

class BigEndianIntLoader : public IntLoader {
 public:
  BigEndianIntLoader() { }
  virtual uint32 ReadInt(const uint8 *data) const {
    uint32 ret = 0;
    ret |= data[0];
    for(uint32 i = 1; i < 4; i++) {
      ret <<= 8;
      ret |= data[i];
    }
    return ret;
  }
};
static const BigEndianIntLoader gBEldr;

class LittleEndianIntLoader : public IntLoader {
 public:
  LittleEndianIntLoader() { }
  virtual uint32 ReadInt(const uint8 *data) const {
    uint32 ret = 0;
    ret |= data[3];
    for(int32 i = 3; i >= 0; i--) {
      ret <<= 8;
      ret |= data[i];
    }
    return ret;
  }
};
static const LittleEndianIntLoader gLEldr;

ImageLoaderKTX::ImageLoaderKTX(const uint8 *rawData, const int32 rawDataSz)
  : ImageLoader(rawData, rawDataSz), m_Processor(NULL)
{ }

ImageLoaderKTX::~ImageLoaderKTX() { }

FasTC::Image<> *ImageLoaderKTX::LoadImage() {

  // Get rid of the pixel data if it exists...
  if(m_PixelData) {
    delete m_PixelData;
    m_PixelData = NULL;
  }

  if(!ReadData()) {
    return NULL;
  }

  if(!m_bIsCompressed) {
    uint32 *pixels = reinterpret_cast<uint32 *>(m_PixelData);
    return new FasTC::Image<>(m_Width, m_Height, pixels);
  }

  return new CompressedImage(m_Width, m_Height, m_Format, m_PixelData);
}

bool ImageLoaderKTX::ReadData() {

  // Default is uncompressed
  m_bIsCompressed = false;

  ByteReader rdr (m_RawData, m_NumRawDataBytes);

  // First, check to make sure that the identifier is present...
  static const uint8 kKTXID[12] = {
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
  };

  if(memcmp(rdr.GetData(), kKTXID, 12) == 0) {
    rdr.Advance(12);
  } else {
    return false;
  }

  const IntLoader *ldr = NULL;
  if(rdr.GetData()[0] == 0x04) {
    ldr = &gBEldr;
  } else {
    ldr = &gLEldr;
  }
  rdr.Advance(4);

  #define LOAD(x) uint32 x; do { if(!ldr->LoadInt(rdr, x)) { return false; } } while(0)
  LOAD(glType);
  LOAD(glTypeSize);
  LOAD(glFormat);
  LOAD(glInternalFormat);
  LOAD(glBaseInternalFormat);
  LOAD(pixelWidth);
  LOAD(pixelHeight);
  LOAD(pixelDepth);
  LOAD(numberOfArrayElements);
  LOAD(numberOfFaces);
  LOAD(numberOfMipmapLevels);
  LOAD(bytesOfKeyValueData);

  // Do we need to read the data?
  if(m_Processor) {
    const uint8 *imgData = rdr.GetData() + bytesOfKeyValueData;
    while(rdr.GetData() < imgData) {
      LOAD(keyAndValueByteSize);
      FasTC::ScopedAllocator<uint8> keyValueData =
        FasTC::ScopedAllocator<uint8>::Create(keyAndValueByteSize);
      if(!keyValueData) {
        fprintf(stderr, "KTX loader - out of memory.\n");
        return false;
      }

      // Read the bytes...
      memcpy(keyValueData, rdr.GetData(), keyAndValueByteSize);
      const char *key = reinterpret_cast<const char *>((const uint8 *)keyValueData);
      const char *value = key;
      while(value != '\0') {
        value++;
      }
      value++; // consume the null byte

      m_Processor(key, value);
      rdr.Advance((keyAndValueByteSize + 3) & ~0x3);
    }
  } else {
    rdr.Advance(bytesOfKeyValueData);
  }

  // The following code only supports a limited subset of
  // image types, the full spec (if we choose to support it)
  // is here:
  // http://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec

  // Read image data...
  if(numberOfMipmapLevels != 1) {
    fprintf(stderr, "KTX loader - unsupported mipmap levels: %d\n", numberOfMipmapLevels);
    return false;
  }

  LOAD(imageSize);
  
  if(numberOfArrayElements > 1) {
    fprintf(stderr,
            "KTX loader - unsupported number of array elements: %d\n",
            numberOfArrayElements);
    return false;
  }

  if(numberOfFaces != 1) {
    fprintf(stderr,
            "KTX loader - unsupported number of faces: %d\n"
            "This likely means that we are trying to load a cube map.\n",
            numberOfFaces);
    return false;
  }

  if(pixelDepth != 0) {
    fprintf(stderr, "KTX loader - 3D textures not supported\n");
    return false;
  }

  if(pixelHeight == 0) {
    fprintf(stderr, "KTX loader - 1D textures not supported\n");
    return false;
  }

  if(pixelWidth == 0) {
    fprintf(stderr, "KTX loader - nonzero pixel width??\n");
    return false;
  }

  m_Width = pixelWidth;
  m_Height = pixelHeight;

  if(glType == 0 && glFormat == 0) {
    switch(glInternalFormat) {
      case GL_COMPRESSED_RGBA_BPTC_UNORM:
        m_Format = FasTC::eCompressionFormat_BPTC;
        break;
      default:
        fprintf(stderr, "KTX loader - texture format (0x%x) unsupported!\n", glInternalFormat);
        return false;
    }

    uint32 dataSize = CompressedImage::GetCompressedSize(pixelWidth * pixelHeight * 4, m_Format);
    m_PixelData = new uint8[dataSize];
    memcpy(m_PixelData, rdr.GetData(), dataSize);
    rdr.Advance(dataSize);

    m_bIsCompressed = true;
  } else {

    if(glType != GL_BYTE) {
      fprintf(stderr, "KTX loader - unsupported OpenGL type: 0x%x\n", glType);
      return false;
    }

    if(glInternalFormat != GL_RGBA8) {
      fprintf(stderr, "KTX loader - unsupported internal format: 0x%x\n", glFormat);
      return false;
    }

    // We should have RGBA8 data here so we can simply load it
    // as we normally would.
    uint32 pixelDataSz = m_Width * m_Height * 4;
    m_PixelData = new uint8[pixelDataSz];
    memcpy(m_PixelData, rdr.GetData(), pixelDataSz);
    rdr.Advance(pixelDataSz);
  }
  return rdr.GetBytesLeft() == 0;
}

