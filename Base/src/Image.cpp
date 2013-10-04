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

#include "Image.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cmath>

#include "Pixel.h"

template <typename T>
static inline T sad( const T &a, const T &b ) {
  return (a > b)? a - b : b - a;
}

namespace FasTC {

template<typename PixelType>
Image<PixelType>::Image(uint32 width, uint32 height)
  : m_Width(width)
  , m_Height(height)
  , m_bBlockStreamOrder(false)
  , m_Pixels(new PixelType[GetNumPixels()])
{ }

template<typename PixelType>
Image<PixelType>::Image(uint32 width, uint32 height,
                        const PixelType *pixels,
                        bool bBlockStreamOrder)
  : m_Width(width)
  , m_Height(height)
  , m_bBlockStreamOrder(false)
{
  if(pixels) {
    m_Pixels = new PixelType[GetNumPixels()];
    memcpy(m_Pixels, pixels, GetNumPixels() * sizeof(PixelType));
  } else {
    m_Pixels = 0;
  }
}

template<typename PixelType>
Image<PixelType>::Image(const Image<PixelType> &other)
  : m_Width(other.m_Width)
  , m_Height(other.m_Height)
  , m_bBlockStreamOrder(other.GetBlockStreamOrder())
  , m_Pixels(new PixelType[GetNumPixels()])
{
  memcpy(m_Pixels, other.m_Pixels, GetNumPixels() * sizeof(PixelType));
}

template<typename PixelType>
bool Image<PixelType>::ReadPixels(const uint32 *rgba) {

  assert(m_Pixels);
  for(uint32 i = 0; i < GetNumPixels(); i++) {
    m_Pixels[i].Unpack(rgba[i]);
  }

  return true;
}

template<typename PixelType>
Image<PixelType>::Image(uint32 width, uint32 height, const uint32 *pixels, bool bBlockStreamOrder)
  : m_Width(width)
  , m_Height(height)
  , m_bBlockStreamOrder(bBlockStreamOrder)
{
  if(pixels) {
    m_Pixels = new PixelType[GetNumPixels()];
    ReadPixels(pixels);
  } else {
    m_Pixels = NULL;
  }
}

template<typename PixelType>
Image<PixelType>::~Image() {
  if(m_Pixels) {
    delete [] m_Pixels;
    m_Pixels = 0;
  }
}

template<typename PixelType>
Image<PixelType> &Image<PixelType>::operator=(const Image &other) {
  
  m_Width = other.m_Width;
  m_Height = other.m_Height;
  m_bBlockStreamOrder = other.GetBlockStreamOrder();
  
  if(m_Pixels) {
    delete [] m_Pixels;
  }
  
  if(other.m_Pixels) {
    m_Pixels = new PixelType[GetNumPixels()];
    if(m_Pixels)
      memcpy(m_Pixels, other.m_Pixels, GetNumPixels() * sizeof(PixelType));
    else
      fprintf(stderr, "Out of memory!\n");
  } else {
    m_Pixels = NULL;
  }

  return *this;
}

template<typename PixelType>
PixelType & Image<PixelType>::operator()(uint32 i, uint32 j) {
  assert(i < GetWidth());
  assert(j < GetHeight());
  return m_Pixels[j * GetWidth() + i];
}

template<typename PixelType>
const PixelType & Image<PixelType>::operator()(uint32 i, uint32 j) const {
  assert(i < GetWidth());
  assert(j < GetHeight());
  return m_Pixels[j * GetWidth() + i];
}

template<typename PixelTypeOne, typename PixelTypeTwo>
double ComputePSNR(Image<PixelTypeOne> *img1, Image<PixelTypeTwo> *img2) {
  if(!img1 || !img2)
    return -1.0;

  if(img1->GetWidth() != img2->GetWidth() ||
     img1->GetHeight() != img2->GetHeight()) {
    return -1.0;
  }

  // Compute raw 8-bit RGBA data...
  img1->ComputePixels();
  img2->ComputePixels();

  const PixelTypeOne *ourPixels = img1->GetPixels();
  const PixelTypeTwo *otherPixels = img2->GetPixels();

  //  const double w[3] = { 0.2126, 0.7152, 0.0722 };
  const double w[3] = { 1.0, 1.0, 1.0 };
    
  double mse = 0.0;
  const uint32 imageSz = img1->GetNumPixels();
  for(uint32 i = 0; i < imageSz; i++) {

    uint32 ourPixel = ourPixels[i].Pack();
    uint32 otherPixel = otherPixels[i].Pack();

    double r[4], u[4];
    for(uint32 c = 0; c < 4; c++) {
      uint32 shift = c * 8;
      if(c == 3) {
        r[c] = static_cast<double>((ourPixel >> shift) & 0xFF) / 255.0;
        u[c] = static_cast<double>((otherPixel >> shift) & 0xFF) / 255.0;
      } else {
        r[c] = static_cast<double>((ourPixel >> shift) & 0xFF) * w[c];
        u[c] = static_cast<double>((otherPixel >> shift) & 0xFF) * w[c];
      }
    }

    for(uint32 c = 0; c < 3; c++) {
      double diff = (r[3] * r[c] - u[3] * u[c]);
      mse += diff * diff;
    }
  }

  mse /= img1->GetWidth() * img1->GetHeight();

  const double C = 255.0 * 255.0;
  double maxi = (w[0]*w[0] + w[1]*w[1] + w[2]*w[2]) * C;
  return 10 * log10(maxi/mse);
}

template double ComputePSNR(Image<Pixel> *, Image<Pixel> *);

// !FIXME! These won't work for non-RGBA8 data.
template<typename PixelType>
void Image<PixelType>::ConvertToBlockStreamOrder() {
  if(m_bBlockStreamOrder || !m_Pixels)
    return;

  PixelType *newPixelData = new PixelType[GetWidth() * GetHeight()];
  for(uint32 j = 0; j < GetHeight(); j+=4) {
    for(uint32 i = 0; i < GetWidth(); i+=4) {
      uint32 blockX = i / 4;
      uint32 blockY = j / 4;
      uint32 blockIdx = blockY * (GetWidth() / 4) + blockX;

      uint32 offset = blockIdx * 4 * 4;
      for(uint32 t = 0; t < 16; t++) {
        uint32 x = i + t % 4;
        uint32 y = j + t / 4;
        newPixelData[offset + t] = m_Pixels[y*GetWidth() + x];
      }
    }
  }

  delete m_Pixels;
  m_Pixels = newPixelData;
  m_bBlockStreamOrder = true;
}

template<typename PixelType>
void Image<PixelType>::ConvertFromBlockStreamOrder() {
  if(!m_bBlockStreamOrder || !m_Pixels)
    return;

  PixelType *newPixelData = new PixelType[GetWidth() * GetHeight()];
  for(uint32 j = 0; j < GetHeight(); j+=4) {
    for(uint32 i = 0; i < GetWidth(); i+=4) {
      uint32 blockX = i / 4;
      uint32 blockY = j / 4;
      uint32 blockIdx = blockY * (GetWidth() / 4) + blockX;

      uint32 offset = blockIdx * 4 * 4;
      for(uint32 t = 0; t < 16; t++) {
        uint32 x = i + t % 4;
        uint32 y = j + t / 4;
        newPixelData[y*GetWidth() + x] = m_Pixels[offset + t];
      }
    }
  }

  delete m_Pixels;
  m_Pixels = newPixelData;
  m_bBlockStreamOrder = false;
}

template<typename PixelType>
void Image<PixelType>::SetImageData(uint32 width, uint32 height, PixelType *data) {
  if(m_Pixels) {
    delete m_Pixels;
  }

  if(!data) {
    width = 0;
    height = 0;
    m_Pixels = NULL;
  } else {
    m_Width = width;
    m_Height = height;
    m_Pixels = data;
  }
}

template class Image<Pixel>;

}  // namespace FasTC
