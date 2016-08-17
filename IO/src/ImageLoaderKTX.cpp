// Copyright 2016 The University of North Carolina at Chapel Hill
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Please send all BUG REPORTS to <pavel@cs.unc.edu>.
// <http://gamma.cs.unc.edu/FasTC/>

#include "ImageLoaderKTX.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "FasTC/Image.h"
#include "FasTC/TexCompTypes.h"
#include "FasTC/CompressedImage.h"
#include "FasTC/ScopedAllocator.h"

#include "GLDefines.h"

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
      FasTC::ScopedAllocator<uint8> keyValueData(keyAndValueByteSize);
      if(!keyValueData) {
        fprintf(stderr, "KTX loader - out of memory.\n");
        return false;
      }

      // Read the bytes...
      memcpy(keyValueData, rdr.GetData(), keyAndValueByteSize);
      const char *key = reinterpret_cast<const char *>((const uint8 *)keyValueData);
      const char *value = key;
      while(*value != '\0') {
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

      case COMPRESSED_RGBA_ASTC_4x4_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC4x4;
        break;

      case COMPRESSED_RGBA_ASTC_5x4_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC5x4;
        break;

      case COMPRESSED_RGBA_ASTC_5x5_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC5x5;
        break;

      case COMPRESSED_RGBA_ASTC_6x5_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC6x5;
        break;

      case COMPRESSED_RGBA_ASTC_6x6_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC6x6;
        break;

      case COMPRESSED_RGBA_ASTC_8x5_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC8x5;
        break;

      case COMPRESSED_RGBA_ASTC_8x6_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC8x6;
        break;

      case COMPRESSED_RGBA_ASTC_8x8_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC8x8;
        break;

      case COMPRESSED_RGBA_ASTC_10x5_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC10x5;
        break;

      case COMPRESSED_RGBA_ASTC_10x6_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC10x6;
        break;

      case COMPRESSED_RGBA_ASTC_10x8_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC10x8;
        break;

      case COMPRESSED_RGBA_ASTC_10x10_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC10x10;
        break;

      case COMPRESSED_RGBA_ASTC_12x10_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC12x10;
        break;

      case COMPRESSED_RGBA_ASTC_12x12_KHR:
        m_Format = FasTC::eCompressionFormat_ASTC12x12;
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

