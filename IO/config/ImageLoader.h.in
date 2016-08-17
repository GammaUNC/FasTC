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

#ifndef _IMAGE_LOADER_H_
#define _IMAGE_LOADER_H_

#include "FasTC/TexCompTypes.h"
#include "FasTC/ImageFwd.h"
#include "FasTC/ImageFileFormat.h"

class ImageLoader {

 protected:

  const uint8 *const m_RawData;
  const uint32 m_NumRawDataBytes;
  uint8 *m_PixelData;

  uint32 m_Width;
  uint32 m_Height;

  uint32 m_RedChannelPrecision;
  uint8 *m_RedData;

  uint32 m_GreenChannelPrecision;
  uint8 *m_GreenData;
  
  uint32 m_BlueChannelPrecision;
  uint8 *m_BlueData;
 
  uint32 m_AlphaChannelPrecision;
  uint8 *m_AlphaData;

  ImageLoader(const uint8 *rawData) 
  : m_RawData(rawData)
  , m_NumRawDataBytes(-1)
  , m_PixelData(0)
  , m_Width(0), m_Height(0)
  , m_RedChannelPrecision(0), m_RedData(0)
  , m_GreenChannelPrecision(0), m_GreenData(0)
  , m_BlueChannelPrecision(0), m_BlueData(0)
  , m_AlphaChannelPrecision(0), m_AlphaData(0)
    { }

  ImageLoader(const uint8 *rawData, const int32 numBytes)
  : m_RawData(rawData)
  , m_NumRawDataBytes(numBytes)
  , m_PixelData(0)
  , m_Width(0), m_Height(0)
  , m_RedChannelPrecision(0), m_RedData(0)
  , m_GreenChannelPrecision(0), m_GreenData(0)
  , m_BlueChannelPrecision(0), m_BlueData(0)
  , m_AlphaChannelPrecision(0), m_AlphaData(0)
    { }

  uint32 GetChannelForPixel(uint32 x, uint32 y, uint32 ch);

  bool LoadFromPixelBuffer(const uint32 *data, bool flipY = false);

 public:
  virtual ~ImageLoader() {
    if(m_RedData) {
      delete [] m_RedData;
      m_RedData = 0;
    }

    if(m_GreenData) {
      delete [] m_GreenData;
      m_GreenData = 0;
    }

    if(m_BlueData) {
      delete [] m_BlueData;
      m_BlueData = 0;
    }

    if(m_AlphaData) {
      delete [] m_AlphaData;
      m_AlphaData = 0;
    }
    
    if(m_PixelData) {
      delete [] m_PixelData;
      m_PixelData = 0;
    }
  }

  virtual bool ReadData() = 0;

  uint32 GetRedChannelPrecision() const { return m_RedChannelPrecision; }
  const uint8 * GetRedPixelData() const { return m_RedData; }

  uint32 GetGreenChannelPrecision() const { return m_GreenChannelPrecision; }
  const uint8 * GetGreenPixelData() const { return m_GreenData; }

  uint32 GetBlueChannelPrecision() const { return m_BlueChannelPrecision; }
  const uint8 * GetBluePixelData() const { return m_BlueData; }

  uint32 GetAlphaChannelPrecision() const { return m_AlphaChannelPrecision; }
  const uint8 * GetAlphaPixelData() const { return m_AlphaData; }

  uint32 GetWidth() const { return m_Width; }
  uint32 GetHeight() const { return m_Height; }
  uint32 GetImageDataSz() const { return m_Width * m_Height * 4; }

  virtual FasTC::Image<> *LoadImage();
  const uint8 *GetImageData() const { return m_PixelData; }
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
