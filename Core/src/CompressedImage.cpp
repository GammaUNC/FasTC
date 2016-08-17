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

#include "FasTC/CompressedImage.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "FasTC/Pixel.h"

#include "FasTC/TexCompTypes.h"
#include "FasTC/BPTCCompressor.h"
#include "FasTC/PVRTCCompressor.h"
#include "FasTC/DXTCompressor.h"
#include "FasTC/ETCCompressor.h"
#include "FasTC/ASTCCompressor.h"

using FasTC::CompressionJob;
using FasTC::DecompressionJob;
using FasTC::ECompressionFormat;

CompressedImage::CompressedImage( const CompressedImage &other )
  : UncompressedImage(other)
  , m_Format(other.m_Format)
  , m_CompressedData(0)
{
  if(other.m_CompressedData) {
    uint32 compressedSz = GetCompressedSize();
    m_CompressedData = new uint8[compressedSz];
    memcpy(m_CompressedData, other.m_CompressedData, compressedSz);
  }
}

CompressedImage::CompressedImage(
  const unsigned int width,
  const unsigned int height,
  const ECompressionFormat format,
  const unsigned char *data
)
  : UncompressedImage(width, height, reinterpret_cast<uint32 *>(NULL))
  , m_Format(format)
  , m_CompressedData(0)
{
  uint32 cmpSz = GetCompressedSize();
  if(cmpSz > 0) {
    assert(!m_CompressedData);
    m_CompressedData = new uint8[cmpSz];
    memcpy(m_CompressedData, data, cmpSz);
  }
}

CompressedImage &CompressedImage::operator=(const CompressedImage &other) {
  UncompressedImage::operator=(other);
  m_Format = other.m_Format;
  if(other.m_CompressedData) {
    uint32 cmpSz = GetCompressedSize();
    m_CompressedData = new uint8[cmpSz];
    memcpy(m_CompressedData, other.m_CompressedData, cmpSz);
  }
  return *this;
}

CompressedImage::~CompressedImage() {
  if(m_CompressedData) {
    delete m_CompressedData;
    m_CompressedData = NULL;
  }
}

bool CompressedImage::DecompressImage(unsigned char *outBuf, unsigned int outBufSz) const {

  assert(outBufSz == GetUncompressedSize());

  uint8 *byteData = reinterpret_cast<uint8 *>(m_CompressedData);
  DecompressionJob dj (m_Format, byteData, outBuf, GetWidth(), GetHeight());
  if(m_Format == FasTC::eCompressionFormat_DXT1) {
    DXTC::DecompressDXT1(dj);
  } else if(m_Format == FasTC::eCompressionFormat_ETC1) {
    ETCC::Decompress(dj);
  } else if(FasTC::COMPRESSION_FORMAT_PVRTC_BEGIN <= m_Format &&
            FasTC::COMPRESSION_FORMAT_PVRTC_END >= m_Format) {
#ifndef NDEBUG
    PVRTCC::Decompress(dj, PVRTCC::eWrapMode_Wrap, true);
#else
    PVRTCC::Decompress(dj);
#endif
  } else if(m_Format == FasTC::eCompressionFormat_BPTC) {
    BPTCC::Decompress(dj);
  } else if(FasTC::COMPRESSION_FORMAT_ASTC_BEGIN <= m_Format &&
            FasTC::COMPRESSION_FORMAT_ASTC_END >= m_Format) {
    ASTCC::Decompress(dj);
  } else {
    const char *errStr = "Have not implemented decompression method.";
    fprintf(stderr, "%s\n", errStr);
    assert(!errStr);
    return false;
  }

  return true;
}

void CompressedImage::ComputePixels() {

  uint32 unCompSz = GetWidth() * GetHeight() * 4;
  uint8 *unCompBuf = new uint8[unCompSz];
  DecompressImage(unCompBuf, unCompSz);

  uint32 * newPixelBuf = reinterpret_cast<uint32 *>(unCompBuf);

  FasTC::Pixel *newPixels = new FasTC::Pixel[GetWidth() * GetHeight()];
  for(uint32 i = 0; i < GetWidth() * GetHeight(); i++) {
    newPixels[i].Unpack(newPixelBuf[i]);
  }

  SetImageData(GetWidth(), GetHeight(), newPixels);
}

uint32 CompressedImage::GetCompressedSize(uint32 uncompressedSize, ECompressionFormat format) {

  // Make sure that the uncompressed size is a multiple of the pixel size.
  assert(uncompressedSize % sizeof(uint32) == 0);

  // The compressed size is the block size times the number of blocks
  uint32 blockDim[2];
  GetBlockDimensions(format, blockDim);

  const uint32 uncompBlockSize = blockDim[0] * blockDim[1] * sizeof(uint32);

  // The uncompressed block size should be a factor of the uncompressed size.
  assert(uncompressedSize % uncompBlockSize == 0);

  const uint32 nBlocks = uncompressedSize / uncompBlockSize;
  const uint32 blockSz = GetBlockSize(format);

  return nBlocks * blockSz;
}
