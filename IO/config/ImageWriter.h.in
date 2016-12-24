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

#ifndef _IMAGE_WRITER_H_
#define _IMAGE_WRITER_H_

#include "FasTC/TexCompTypes.h"
#include "FasTC/ImageFileFormat.h"

namespace FasTC {
  class Pixel;
}

class ImageWriter {

 protected:

  const FasTC::Pixel *m_Pixels;
  uint32 m_RawFileDataSz;
  uint8 *m_RawFileData;
  
  uint32 m_Width;
  uint32 m_Height;

  ImageWriter(const int width, const int height, const FasTC::Pixel *rawData) 
  : m_Pixels(rawData)
  , m_RawFileDataSz(256)
  , m_RawFileData(new uint8[m_RawFileDataSz])
  , m_Width(width), m_Height(height)
    { }

  uint32 GetChannelForPixel(uint32 x, uint32 y, uint32 ch);

 public:
  virtual ~ImageWriter() {
    if(m_RawFileData) {
      delete [] m_RawFileData;
      m_RawFileData = 0;
    }
  }

  uint32 GetWidth() const { return m_Width; }
  uint32 GetHeight() const { return m_Height; }
  uint32 GetImageDataSz() const { return m_Width * m_Height * sizeof(uint32); }
  uint32 GetRawFileDataSz() const { return m_RawFileDataSz; }
  uint8 *GetRawFileData() const { return m_RawFileData; }
  virtual bool WriteImage() = 0;
};

#ifndef PNG_FOUND
#cmakedefine PNG_FOUND
#endif // PNG_FOUND

#ifndef PVRTEXLIB_FOUND
#cmakedefine PVRTEXLIB_FOUND
#endif // PVRTEXLIB_FOUND

#ifndef OPENGL_FOUND
#cmakedefine OPENGL_FOUND
#endif // OPENGL_FOUND

#endif // _IMAGE_LOADER_H_
