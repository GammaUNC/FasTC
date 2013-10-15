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

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cmath>

#include "Color.h"
#include "Pixel.h"
#include "IPixel.h"

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

template<typename PixelType>
double Image<PixelType>::ComputePSNR(Image<PixelType> *other) {
  if(!other)
    return -1.0;

  if(GetWidth() != other->GetWidth() ||
     GetHeight() != other->GetHeight()) {
    return -1.0;
  }

  // Compute raw 8-bit RGBA data...
  ComputePixels();
  other->ComputePixels();

  const PixelType *ourPixels = GetPixels();
  const PixelType *otherPixels = other->GetPixels();

  //  const double w[3] = { 0.2126, 0.7152, 0.0722 };
  const double w[3] = { 1.0, 1.0, 1.0 };
    
  double mse = 0.0;
  const uint32 imageSz = GetNumPixels();
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

  mse /= GetWidth() * GetHeight();

  const double C = 255.0 * 255.0;
  double maxi = (w[0]*w[0] + w[1]*w[1] + w[2]*w[2]) * C;
  return 10 * log10(maxi/mse);
}

static Image<IPixel> FilterValid(const Image<IPixel> &img, uint32 size, double sigma) {
  assert(size % 2);
  Image<IPixel> gaussian(size, size);
  GenerateGaussianKernel(gaussian, size, static_cast<float>(sigma));

  double sum = 0.0;
  for(uint32 j = 0; j < size; j++) {
    for(uint32 i = 0; i < size; i++) {
      sum += static_cast<float>(gaussian(i, j));
    }
  }

  for(uint32 j = 0; j < size; j++) {
    for(uint32 i = 0; i < size; i++) {
      double v = static_cast<float>(gaussian(i, j));
      gaussian(i, j) = static_cast<float>(v / sum);
    }
  }      

  int32 h = static_cast<int32>(img.GetHeight());
  int32 w = static_cast<int32>(img.GetWidth());

  Image<IPixel> out(img.GetWidth() - size + 1, img.GetHeight() - size + 1);
  int32 halfSz = static_cast<int32>(size) >> 1;
  for(int32 j = halfSz; j < h-halfSz; j++) {
    for(int32 i = halfSz; i < w-halfSz; i++) {
      int32 xoffset = -halfSz;
      int32 yoffset = -halfSz;

      double result = 0;
      for(int32 y = 0; y < static_cast<int32>(size); y++)
      for(int32 x = 0; x < static_cast<int32>(size); x++) {
        double s = static_cast<float>(gaussian(x, y));
        result += s * static_cast<float>(img(i+xoffset+x, j+yoffset+y));
      }
      out(i+xoffset, j+yoffset) = static_cast<float>(result);
    }
  }

  return out;
}

template<typename PixelType>
double Image<PixelType>::ComputeSSIM(Image<PixelType> *other) {
  if(!other) {
    return -1.0;
  }

  if(GetWidth() != other->GetWidth() ||
     GetHeight() != other->GetHeight()) {
    return -1.0;
  }

  ComputePixels();
  other->ComputePixels();

  double C1 = (0.01 * 255.0 * 0.01 * 255.0);
  double C2 = (0.03 * 255.0 * 0.03 * 255.0);

  Image<IPixel> img1(GetWidth(), GetHeight());
  Image<IPixel> img2(GetWidth(), GetHeight());

  ConvertTo(img1);
  other->ConvertTo(img2);

  for(uint32 j = 0; j < GetHeight(); j++) {
    for(uint32 i = 0; i < GetWidth(); i++) {
      img1(i, j) = 255.0f * static_cast<float>(img1(i, j));
      img2(i, j) = 255.0f * static_cast<float>(img2(i, j));
    }
  }

  /* Matlab code taken from 
     http://www.cns.nyu.edu/lcv/ssim/ssim_index.m

     C1 = (K(1)*L)^2;
     C2 = (K(2)*L)^2;
     window = window/sum(sum(window));
     img1 = double(img1);
     img2 = double(img2);

     mu1   = filter2(window, img1, 'valid');
     mu2   = filter2(window, img2, 'valid');
     mu1_sq = mu1.*mu1;
     mu2_sq = mu2.*mu2;
     mu1_mu2 = mu1.*mu2;
     sigma1_sq = filter2(window, img1.*img1, 'valid') - mu1_sq;
     sigma2_sq = filter2(window, img2.*img2, 'valid') - mu2_sq;
     sigma12 = filter2(window, img1.*img2, 'valid') - mu1_mu2;

     ssim_map = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))./
                ((mu1_sq + mu2_sq + C1).*(sigma1_sq + sigma2_sq + C2));
  */

  const uint32 filterSz = 11;
  const double filterSigma = 1.5;

  Image<IPixel> mu1 = FilterValid(img1, filterSz, filterSigma);
  Image<IPixel> mu2 = FilterValid(img2, filterSz, filterSigma);

  assert(mu1.GetHeight() == mu2.GetHeight());
  assert(mu1.GetWidth() == mu2.GetWidth());

  Image<IPixel> mu1_sq(mu1);
  Image<IPixel> mu2_sq(mu2);
  Image<IPixel> mu1_mu2(mu1);
  Image<IPixel> sigma1_sq(img1);
  Image<IPixel> sigma2_sq(img2);
  Image<IPixel> sigma12(img1);

  uint32 w = ::std::max(img1.GetWidth(), mu1.GetWidth());
  uint32 h = ::std::max(img1.GetHeight(), mu1.GetHeight());
  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      if(i < mu1.GetWidth() && j < mu1.GetHeight()) {
        double m1 = static_cast<float>(mu1(i, j));
        double m2 = static_cast<float>(mu2(i, j));

        mu1_sq(i, j) = static_cast<float>(m1 * m1);
        mu2_sq(i, j) = static_cast<float>(m2 * m2);
        mu1_mu2(i, j) = static_cast<float>(m1 * m2);
      }

      if(i < img1.GetWidth() && j < img1.GetHeight()) {
        double i1 = static_cast<float>(img1(i, j));
        double i2 = static_cast<float>(img2(i, j));

        sigma1_sq(i, j) = static_cast<float>(i1 * i1);
        sigma2_sq(i, j) = static_cast<float>(i2 * i2);
        sigma12(i, j) = static_cast<float>(i1 * i2);
      }
    }
  }

  sigma1_sq = FilterValid(sigma1_sq, filterSz, filterSigma);
  sigma2_sq = FilterValid(sigma2_sq, filterSz, filterSigma);
  sigma12 = FilterValid(sigma12, filterSz, filterSigma);

  assert(sigma1_sq.GetWidth() == mu1.GetWidth());
  assert(sigma1_sq.GetHeight() == mu1.GetHeight());

  assert(sigma2_sq.GetWidth() == mu1.GetWidth());
  assert(sigma2_sq.GetHeight() == mu1.GetHeight());

  assert(sigma12.GetWidth() == mu1.GetWidth());
  assert(sigma12.GetHeight() == mu1.GetHeight());

  w = mu1_sq.GetWidth();
  h = mu2_sq.GetHeight();

  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      double m1sq = static_cast<float>(mu1_sq(i, j));
      double m2sq = static_cast<float>(mu2_sq(i, j));
      double m1m2 = static_cast<float>(mu1_mu2(i, j));

      double s1sq = static_cast<float>(sigma1_sq(i, j));
      double s2sq = static_cast<float>(sigma2_sq(i, j));
      double s1s2 = static_cast<float>(sigma12(i, j));

      sigma1_sq(i, j) = static_cast<float>(s1sq - m1sq);
      sigma2_sq(i, j) = static_cast<float>(s2sq - m2sq);
      sigma12(i, j) = static_cast<float>(s1s2 - m1m2);
    }
  }

  double mssim = 0.0;
  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      double m1sq = static_cast<float>(mu1_sq(i, j));
      double m2sq = static_cast<float>(mu2_sq(i, j));
      double m1m2 = static_cast<float>(mu1_mu2(i, j));

      double s1sq = static_cast<float>(sigma1_sq(i, j));
      double s2sq = static_cast<float>(sigma2_sq(i, j));
      double s1s2 = static_cast<float>(sigma12(i, j));

      double ssim =
        ((2.0 * m1m2 + C1) * (2.0 * s1s2 + C2)) /
        ((m1sq + m2sq + C1) * (s1sq + s2sq + C2));

      mssim += ssim;
    }
  }

  return mssim / static_cast<double>(w * h);
}

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

  template<typename T>
  static inline T Clamp(const T &v, const T &a, const T &b) {
    return ::std::min(::std::max(a, v), b);
  }

template<typename PixelType>
void Image<PixelType>::Filter(const Image<IPixel> &kernel) {
  Image<IPixel> k(kernel);

  // Only odd sized filters make sense....
  assert(k.GetWidth() % 2);
  assert(k.GetHeight() % 2);

  double sum = 0.0;
  for(uint32 j = 0; j < k.GetHeight(); j++) {
    for(uint32 i = 0; i < k.GetWidth(); i++) {
      sum += static_cast<float>(k(i, j));
    }
  }

  for(uint32 j = 0; j < k.GetHeight(); j++) {
    for(uint32 i = 0; i < k.GetWidth(); i++) {
      k(i, j) = static_cast<float>(k(i, j) / sum);
    }
  }

  int32 ih = static_cast<int32>(GetHeight());
  int32 iw = static_cast<int32>(GetWidth());

  int32 kh = static_cast<int32>(k.GetHeight());
  int32 kw = static_cast<int32>(k.GetWidth());

  Image<PixelType> filtered(iw, ih);

  for(int32 j = 0; j < ih; j++) {
    for(int32 i = 0; i < iw; i++) {
      int32 yoffset = j - (k.GetHeight() / 2);
      int32 xoffset = i - (k.GetWidth() / 2);

      Color newPixel;
      for(int32 y = 0; y < kh; y++) {
        for(int32 x = 0; x < kw; x++) {
          PixelType pixel = ((*this)(
            Clamp<int32>(x + xoffset, 0, GetWidth() - 1),
            Clamp<int32>(y + yoffset, 0, GetHeight() - 1)));
          Color c; c.Unpack(pixel.Pack());
          Color scaled = c * static_cast<float>(k(x, y));
          newPixel += scaled;
        }
      }

      filtered(i, j).Unpack(newPixel.Pack());
    }
  }

  *this = filtered;
}

template class Image<Pixel>;
template class Image<IPixel>;
template class Image<Color>;

void GenerateGaussianKernel(Image<IPixel> &out, uint32 size, float sigma) {

  assert(size % 2);

  out = Image<IPixel>(size, size);
  if(size == 1) {
    out(0, 0) = 1.0f;
    return;
  }

  int32 halfSz = static_cast<int32>(size) / 2;
  for(int32 j = -halfSz; j <= halfSz; j++) {
    for(int32 i = -halfSz; i <= halfSz; i++) {
      out(halfSz + i, halfSz + j) = exp(- (j*j + i*i) / (2*sigma*sigma));
    }
  }
}

}  // namespace FasTC
