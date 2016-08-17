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

#include "FasTC/ImageLoader.h"
#include "FasTC/Image.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////
//
// Static helper functions
//
///////////////////////////////////////////////////////////////////////////////

template <typename T>
static inline T min(const T &a, const T &b) {
  return (a > b)? b : a;
}

template <typename T>
static inline T abs(const T &a) {
  return (a > 0)? a : -a;
}

template <typename T>
static inline T sad(const T &a, const T &b) {
  return (a > b)? a - b : b - a;
}

void ReportError(const char *str) {
  fprintf(stderr, "ImageLoader.cpp -- ERROR: %s\n", str);
}

unsigned int ImageLoader::GetChannelForPixel(uint32 x, uint32 y, uint32 ch) {

  // First make sure that we're in bounds...
  if(x >= GetWidth()) {
    return 0;
  }

  if(y >= GetHeight()) {
    return 0;
  }

  uint32 prec = 0;
  const uint8 *data = NULL;

  switch(ch) {
  case 0:
    prec = GetRedChannelPrecision();
    data = GetRedPixelData();
    break;

  case 1:
    prec = GetGreenChannelPrecision();
    data = GetGreenPixelData();
    break;

  case 2:
    prec = GetBlueChannelPrecision();
    data = GetBluePixelData();
    break;

  case 3:
    prec = GetAlphaChannelPrecision();
    data = GetAlphaPixelData();
    break;

  default:
    ReportError("Unspecified channel");
    return INT_MAX;
  }

  if(0 == prec)
    return 0;

  uint32 pixelIdx = y * GetWidth() + x;
  const uint32 val = data[pixelIdx];
  
  if(prec < 8) {
    int32 ret = 0;
    for(uint32 precLeft = 8; precLeft > 0; precLeft -= min(prec, sad(prec, precLeft))) {
      
      if(prec > precLeft) {
        const int toShift = prec - precLeft;
        ret = ret << precLeft;
        ret |= val >> toShift;
      }
      else {
        ret = ret << prec;
        ret |= val;
      }
    }

    return static_cast<unsigned int>(ret);
  }
  else if(prec > 8) {
    const int32 toShift = prec - 8;
    return val >> toShift;
  }

  return val;
}

bool ImageLoader::LoadFromPixelBuffer(const uint32 *data, bool flipY) {
  m_RedChannelPrecision = 8;
  m_GreenChannelPrecision = 8;
  m_BlueChannelPrecision = 8;
  m_AlphaChannelPrecision = 8;

  const int nPixels = m_Width * m_Height;
  m_RedData = new uint8[nPixels];
  m_GreenData = new uint8[nPixels];
  m_BlueData = new uint8[nPixels];
  m_AlphaData = new uint8[nPixels];

  for (uint32 j = 0; j < m_Height; j++) {
    for (uint32 i = 0; i < m_Width; i++) {
      uint32 idx = j*m_Width + i;
      uint32 pIdx = idx;
      if(flipY)
        idx = (m_Height - j - 1)*m_Width + i;
      uint32 pixel = data[idx];
      m_RedData[pIdx] = static_cast<uint8>(pixel & 0xFF);
      m_GreenData[pIdx] = static_cast<uint8>((pixel >> 8) & 0xFF);
      m_BlueData[pIdx] = static_cast<uint8>((pixel >> 16) & 0xFF);
      m_AlphaData[pIdx] = static_cast<uint8>((pixel >> 24) & 0xFF);
    }
  }

  return true;
}

FasTC::Image<> *ImageLoader::LoadImage() {

  // Do we already have pixel data?
  if(m_PixelData) {
    uint32 *pixels = reinterpret_cast<uint32 *>(m_PixelData);
    return new FasTC::Image<>(m_Width, m_Height, pixels);
  }

  // Read the image data!
  if(!ReadData())
    return NULL;

  // Create RGBA buffer 
  const unsigned int dataSz = 4 * GetWidth() * GetHeight();
  m_PixelData = new unsigned char[dataSz];

  int byteIdx = 0;
  for(uint32 j = 0; j < GetHeight(); j++) {
    for(uint32 i = 0; i < GetWidth(); i++) {

      unsigned int redVal = GetChannelForPixel(i, j, 0);
      if(redVal == INT_MAX) {
        return NULL;
      }

      unsigned int greenVal = redVal;
      unsigned int blueVal = redVal;

      if(GetGreenChannelPrecision() > 0) {
        greenVal = GetChannelForPixel(i, j, 1);
        if(greenVal == INT_MAX) {
          return NULL;
        }
      }

      if(GetBlueChannelPrecision() > 0) {
        blueVal = GetChannelForPixel(i, j, 2);
        if(blueVal == INT_MAX) {
          return NULL;
        }
      }

      unsigned int alphaVal = 0xFF;
      if(GetAlphaChannelPrecision() > 0) {
        alphaVal = GetChannelForPixel(i, j, 3);
        if(alphaVal == INT_MAX) {
          return NULL;
        }
      }

      // Red channel
      m_PixelData[byteIdx++] = static_cast<uint8>(redVal & 0xFF);

      // Green channel
      m_PixelData[byteIdx++] = static_cast<uint8>(greenVal & 0xFF);

      // Blue channel
      m_PixelData[byteIdx++] = static_cast<uint8>(blueVal & 0xFF);

      // Alpha channel
      m_PixelData[byteIdx++] = static_cast<uint8>(alphaVal & 0xFF);
    }
  }

  uint32 *pixels = reinterpret_cast<uint32 *>(m_PixelData);
  return new FasTC::Image<>(m_Width, m_Height, pixels);
}
