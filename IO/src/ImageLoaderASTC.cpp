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

#include "ImageLoaderASTC.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "FasTC/TexCompTypes.h"
#include "FasTC/Image.h"
#include "FasTC/CompressionFormat.h"
#include "FasTC/ScopedAllocator.h"

#include "GLDefines.h"
#include "FasTC/CompressedImage.h"

static bool GetFormatForBlockDimensions(FasTC::ECompressionFormat &out,
                                        uint32 blockWidth, uint32 blockHeight) {
  switch((blockWidth << 4) | blockHeight) {
    case ((4 << 4) | 4):
      out = FasTC::eCompressionFormat_ASTC4x4;
      break;
    case ((5 << 4) | 4):
      out = FasTC::eCompressionFormat_ASTC5x4;
      break;
    case ((5 << 4) | 5):
      out = FasTC::eCompressionFormat_ASTC5x5;
      break;
    case ((6 << 4) | 5):
      out = FasTC::eCompressionFormat_ASTC6x5;
      break;
    case ((6 << 4) | 6):
      out = FasTC::eCompressionFormat_ASTC6x6;
      break;
    case ((8 << 4) | 5):
      out = FasTC::eCompressionFormat_ASTC8x5;
      break;
    case ((8 << 4) | 6):
      out = FasTC::eCompressionFormat_ASTC8x6;
      break;
    case ((8 << 4) | 8):
      out = FasTC::eCompressionFormat_ASTC8x8;
      break;
    case ((10 << 4) | 5):
      out = FasTC::eCompressionFormat_ASTC10x5;
      break;
    case ((10 << 4) | 6):
      out = FasTC::eCompressionFormat_ASTC10x6;
      break;
    case ((10 << 4) | 8):
      out = FasTC::eCompressionFormat_ASTC10x8;
      break;
    case ((10 << 4) | 10):
      out = FasTC::eCompressionFormat_ASTC10x10;
      break;
    case ((12 << 4) | 10):
      out = FasTC::eCompressionFormat_ASTC12x10;
      break;
    case ((12 << 4) | 12):
      out = FasTC::eCompressionFormat_ASTC12x12;
      break;

    default:
      fprintf(stderr, "Unsupported block dimensions (%d, %d)\n", blockWidth, blockHeight);
      return false;
  }

  return true;
}

ImageLoaderASTC::ImageLoaderASTC(const uint8 *rawData, const int32 rawDataSz)
  : ImageLoader(rawData, rawDataSz), m_BlockSizeX(0), m_BlockSizeY(0)
{ }

ImageLoaderASTC::~ImageLoaderASTC() { }

FasTC::Image<> *ImageLoaderASTC::LoadImage() {

  // Get rid of the pixel data if it exists...
  if(m_PixelData) {
    delete m_PixelData;
    m_PixelData = NULL;
  }

  if(!ReadData()) {
    return NULL;
  }

  FasTC::ECompressionFormat fmt;
  if(!GetFormatForBlockDimensions(fmt, m_BlockSizeX, m_BlockSizeY)) {
    return NULL;
  }

  return new CompressedImage(m_Width, m_Height, fmt, m_PixelData);
}

template <typename T>
static inline void Reverse(T &v) {
  uint8 *vp = reinterpret_cast<uint8 *>(&v);
  for(uint32 i = 0; i < sizeof(T) >> 1; i++) {
    std::swap(vp[i], vp[sizeof(T) - 1 - i]);
  }
}

uint32 ReadThreeWideInt(const uint8 *data, const bool bLittleEndian) {
  uint32 result = *(reinterpret_cast<const uint32 *>(data));
  if(!bLittleEndian) {
    Reverse(result);
  }
  result &= 0xFFFFFF;
  return result;
}

bool ImageLoaderASTC::ReadData() {
  
  const uint8 *data = m_RawData;
  uint32 magic = reinterpret_cast<const uint32 *>(data)[0];

  // Read the magic bytes at the beginning.
  bool bLittleEndian = m_RawData[0] == (magic & 0xFF);
  bool OK = false;
  if(bLittleEndian && magic == 0x5CA1AB13) {
    OK = true;
  } else if(!bLittleEndian && magic == 0x13ABA15C) {
    OK = true;
  }

  if(!OK) return false;
  data += 4;

  // The next byte is the block width
  uint8 blockWidth = *data;
  data++;
  uint8 blockHeight = *data;
  data++;
  uint8 blockDepth = *data;
  data++;

  assert(blockWidth <= 12);
  assert(blockHeight <= 12);
  if(blockDepth != 1) {
    fprintf(stderr, "3D compressed textures unsupported!\n");
    return false;
  }

  uint32 pixelWidth = ReadThreeWideInt(data, bLittleEndian);
  data += 3;

  uint32 pixelHeight = ReadThreeWideInt(data, bLittleEndian);
  data += 3;

  uint32 pixelDepth = ReadThreeWideInt(data, bLittleEndian);
  data += 3;

  if(pixelDepth != 1) {
    fprintf(stderr, "3D compressed textures unsupported!\n");
    return false;
  }

  assert(data - m_RawData == 16);

  // Figure out the format based on the block size...
  FasTC::ECompressionFormat fmt;
  if(!GetFormatForBlockDimensions(fmt, blockWidth, blockHeight)) {
    return false;
  }

  m_BlockSizeX = blockWidth;
  m_BlockSizeY = blockHeight;
  m_Width = pixelWidth;
  m_Height = pixelHeight;

  uint32 uncompressedSize = pixelWidth * pixelHeight * 4;
  uint32 compressedSize = CompressedImage::GetCompressedSize(pixelWidth, pixelHeight, fmt);

  assert(compressedSize + 16 == m_NumRawDataBytes);
  m_PixelData = new uint8[compressedSize];
  memcpy(m_PixelData, data, compressedSize);
  return true;
}

