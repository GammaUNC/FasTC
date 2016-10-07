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

#ifndef _COMPRESSED_IMAGE_H_
#define _COMPRESSED_IMAGE_H_

#include "FasTC/TexCompTypes.h"
#include "FasTC/CompressionFormat.h"
#include "FasTC/Image.h"

class CompressedImage : public FasTC::Image<FasTC::Pixel> {
 private:
  FasTC::ECompressionFormat m_Format;
  uint8 *m_CompressedData;

  typedef FasTC::Image<FasTC::Pixel> UncompressedImage;

 public:
  CompressedImage(const CompressedImage &);
  CompressedImage &operator=(const CompressedImage &);

  // Create a compressed image from the given data according to
  // the passed format. The size of the data is expected to conform
  // to the width, height, and format specified.
  CompressedImage(
    const uint32 width,
    const uint32 height,
    const FasTC::ECompressionFormat format,
    const uint8 *data
  );

  virtual ~CompressedImage();

  virtual FasTC::Image<FasTC::Pixel> *Clone() const {
    return new CompressedImage(*this);
  }

  virtual void ComputePixels();

  static uint32 GetCompressedSize(uint32 width, uint32 height, FasTC::ECompressionFormat format);

  uint32 GetCompressedSize() const {
    return GetCompressedSize(GetWidth(), GetHeight(), m_Format);
  }
  uint32 GetUncompressedSize() const {
    return GetWidth() * GetHeight() * sizeof(uint32);
  }

  // Decompress the compressed image data into outBuf. outBufSz is expected
  // to be the proper size determined by the width, height, and format.
  // !FIXME! We should have a function to explicitly return the in/out buf
  // size for a given compressed image.
  bool DecompressImage(uint8 *outBuf, uint32 outBufSz) const;

  const uint8 *GetCompressedData() const { return m_CompressedData; }

  FasTC::ECompressionFormat GetFormat() const { return m_Format; }
};

#endif // _COMPRESSED_IMAGE_H_
