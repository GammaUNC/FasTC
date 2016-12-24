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

#if _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include "PVRTCImage.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>

#include "FasTC/Pixel.h"
using FasTC::Pixel;

#ifdef DEBUG_PVRTC_DECODER
#  include "../../IO/include/ImageFile.h"
#endif

#include "Indexer.h"

template <typename T>
inline T Clamp(const T &v, const T &a, const T &b) {
  return ::std::min(::std::max(a, v), b);
}

namespace PVRTCC {

Image::Image(uint32 width, uint32 height)
  : FasTC::Image<FasTC::Pixel>(width, height)
  , m_FractionalPixels(new FasTC::Pixel[width * height]) {
  assert(width > 0);
  assert(height > 0);
}

Image::Image(uint32 width, uint32 height, const FasTC::Pixel *pixels)
  : FasTC::Image<FasTC::Pixel>(width, height, pixels)
  , m_FractionalPixels(new FasTC::Pixel[width * height]) {
  assert(width > 0);
  assert(height > 0);
}

Image::Image(const Image &other)
  : FasTC::Image<FasTC::Pixel>(other)
  , m_FractionalPixels(new FasTC::Pixel[other.GetWidth() * other.GetHeight()]) {
  memcpy(m_FractionalPixels, other.m_FractionalPixels, GetWidth() * GetHeight() * sizeof(FasTC::Pixel));
}

Image &Image::operator=(const Image &other) {
  FasTC::Image<FasTC::Pixel>::operator=(other);

  assert(m_FractionalPixels);
  delete [] m_FractionalPixels;
  m_FractionalPixels = new FasTC::Pixel[other.GetWidth() * other.GetHeight()];
  memcpy(m_FractionalPixels, other.m_FractionalPixels,
         GetWidth() * GetHeight() * sizeof(FasTC::Pixel));

  return *this;
}

Image::~Image() {
  assert(m_FractionalPixels);
  delete [] m_FractionalPixels;
}

#ifndef NDEBUG
static bool CompareBitDepths(const uint8 (&depth1)[4],
                             const uint8 (&depth2)[4]) {
  bool ok = true;
  for(uint32 i = 0; i < 4; i++) {
    ok = ok && depth1[i] == depth2[i];
  }
  return ok;
}
#endif

void Image::BilinearUpscale(uint32 xtimes, uint32 ytimes,
                            EWrapMode wrapMode) {
  const uint32 newWidth = GetWidth() << xtimes;
  const uint32 newHeight = GetHeight() << ytimes;

  const uint32 xscale = 1 << xtimes;
  const uint32 xoffset = xscale >> 1;

  const uint32 yscale = 1 << ytimes;
  const uint32 yoffset = yscale >> 1;

  FasTC::Pixel *upscaledPixels = new FasTC::Pixel[newWidth * newHeight];

  assert(m_FractionalPixels);
  delete [] m_FractionalPixels;
  m_FractionalPixels = new FasTC::Pixel[newWidth * newHeight];

  Indexer idxr(newWidth, newHeight, wrapMode);
  for(uint32 j = 0; j < newHeight; j++) {
    for(uint32 i = 0; i < newWidth; i++) {

      const uint32 pidx = idxr(i, j);
      FasTC::Pixel &p = upscaledPixels[pidx];
      FasTC::Pixel &fp = m_FractionalPixels[pidx];

      const int32 highXIdx = (i + xoffset) / xscale;
      const int32 lowXIdx = highXIdx - 1;
      const int32 highYIdx = (j + yoffset) / yscale;
      const int32 lowYIdx = highYIdx - 1;

      const uint32 highXWeight = (i + xoffset) % xscale;
      const uint32 lowXWeight = xscale - highXWeight;
      const uint32 highYWeight = (j + yoffset) % yscale;
      const uint32 lowYWeight = yscale - highYWeight;

      const uint32 topLeftWeight = lowXWeight * lowYWeight;
      const uint32 topRightWeight = highXWeight * lowYWeight;
      const uint32 bottomLeftWeight = lowXWeight * highYWeight;
      const uint32 bottomRightWeight = highXWeight * highYWeight;

      const FasTC::Pixel &topLeft = GetPixel(lowXIdx, lowYIdx, wrapMode);
      const FasTC::Pixel &topRight = GetPixel(highXIdx, lowYIdx, wrapMode);
      const FasTC::Pixel &bottomLeft = GetPixel(lowXIdx, highYIdx, wrapMode);
      const FasTC::Pixel &bottomRight = GetPixel(highXIdx, highYIdx, wrapMode);

      // Make sure the bit depth matches the original...
      uint8 bitDepth[4];
      topLeft.GetBitDepth(bitDepth);
      p.ChangeBitDepth(bitDepth);

#ifndef NDEBUG
      uint8 debugDepth[4];

      topRight.GetBitDepth(debugDepth);
      assert(CompareBitDepths(bitDepth, debugDepth));

      bottomLeft.GetBitDepth(debugDepth);
      assert(CompareBitDepths(bitDepth, debugDepth));

      bottomRight.GetBitDepth(debugDepth);
      assert(CompareBitDepths(bitDepth, debugDepth));
#endif  // NDEBUG

      // bilerp each channel....
      const uint16 scaleMask = (xscale * yscale) - 1;
      uint8 fpDepths[4];
      for(uint32 c = 0; c < 4; c++) fpDepths[c] = xtimes + ytimes;
      fp.ChangeBitDepth(fpDepths);

      const FasTC::Pixel tl = topLeft * topLeftWeight;
      const FasTC::Pixel tr = topRight * topRightWeight;
      const FasTC::Pixel bl = bottomLeft * bottomLeftWeight;
      const FasTC::Pixel br = bottomRight * bottomRightWeight;
      const FasTC::Pixel sum = tl + tr + bl + br;

      for(uint32 c = 0; c < 4; c++) {
        fp.Component(c) = sum.Component(c) & scaleMask;
      }

      p = sum / (xscale * yscale);
    }
  }

  SetImageData(newWidth, newHeight, upscaledPixels);
}

static Pixel AveragePixels(const ::std::vector<Pixel> &pixels) {
  if(pixels.size() == 0) {
    return Pixel();
  }

  uint32 sum[4] = {0};
  ::std::vector<Pixel>::const_iterator it;
  for(it = pixels.begin(); it != pixels.end(); it++) {
    for(uint32 c = 0; c < 4; c++) {
      sum[c] += (*it).Component(c);
    }
  }

  Pixel result;
  for(uint32 c = 0; c < 4; c++) {
    result.Component(c) = static_cast<uint16>(sum[c] / pixels.size());
  }

  return result;
}

void Image::AverageDownscale(uint32 xtimes, uint32 ytimes) {
  const uint32 w = GetWidth();
  const uint32 h = GetHeight();

  const uint32 newWidth = w >> xtimes;
  const uint32 newHeight = h >> ytimes;

  Pixel *downscaledPixels = new Pixel[newWidth * newHeight];

  uint8 bitDepth[4];
  GetPixel(0, 0).GetBitDepth(bitDepth);

  uint32 pixelsX = 1 << xtimes;
  uint32 pixelsY = 1 << ytimes;

  ::std::vector<Pixel> toAvg;
  toAvg.reserve(pixelsX * pixelsY);

  for(uint32 j = 0; j < newHeight; j++) {
    for(uint32 i = 0; i < newWidth; i++) {
      uint32 newIdx = j * newWidth + i;

      toAvg.clear();
      for(uint32 y = j * pixelsY; y < (j+1) * pixelsY; y++) {
        for(uint32 x = i * pixelsX; x < (i+1) * pixelsX; x++) {
          toAvg.push_back((*this)(x, y));
        }
      }

      downscaledPixels[newIdx] = AveragePixels(toAvg);
    }
  }

  SetImageData(newWidth, newHeight, downscaledPixels);
}

void Image::ContentAwareDownscale(uint32 xtimes, uint32 ytimes,
                                  EWrapMode wrapMode, bool bOffsetNewPixels) {
  const uint32 w = GetWidth();
  const uint32 h = GetHeight();

  const uint32 newWidth = w >> xtimes;
  const uint32 newHeight = h >> ytimes;

  FasTC::Pixel *downscaledPixels = new FasTC::Pixel[newWidth * newHeight];
  const uint32 numDownscaledPixels = newWidth * newHeight;

  uint8 bitDepth[4];
  GetPixels()[0].GetBitDepth(bitDepth);

  for(uint32 i = 0; i < numDownscaledPixels; i++) {
    downscaledPixels[i].ChangeBitDepth(bitDepth);
  }

  // Allocate memory
  float *imgData = new float[19 * w * h];
  float *I = imgData;
  float *Ix[5] = {
    imgData + (w * h),
    imgData + (2 * w * h),
    imgData + (3 * w * h),
    imgData + (4 * w * h),
    imgData + (18 * w * h),
  };
  float *Iy = imgData + (5 * w * h);
  float *Ixx[4] = {
    imgData + (6 * w * h),
    imgData + (7 * w * h),
    imgData + (8 * w * h),
    imgData + (9 * w * h)
  };
  float *Iyy[4] = {
    imgData + (10 * w * h),
    imgData + (11 * w * h),
    imgData + (12 * w * h),
    imgData + (13 * w * h)
  };
  float *Ixy[4] = {
    imgData + (14 * w * h),
    imgData + (15 * w * h),
    imgData + (16 * w * h),
    imgData + (17 * w * h)
  };

  // Then, compute the intensity of the image
  for(uint32 i = 0; i < w * h; i++) {
    I[i] = GetPixels()[i].ToIntensity();
  }

  // Use central differences to calculate Ix, Iy, Ixx, Iyy...
  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      uint32 xmhidx = GetPixelIndex(i-1, j);
      uint32 xphidx = GetPixelIndex(i+1, j);

      uint32 ymhidx = GetPixelIndex(i, j-1);
      uint32 yphidx = GetPixelIndex(i, j+1);

      uint32 idx = GetPixelIndex(i, j);

      uint32 upidx = GetPixelIndex(i + 1, j + 1);
      uint32 downidx = GetPixelIndex(i - 1, j - 1);

      Ix[4][idx] = (I[xphidx] - I[xmhidx]) / 2.0f;
      Iy[idx] = (I[yphidx] - I[ymhidx]) / 2.0f;

      for(uint32 c = 0; c <= 3; c++) {
        #define CPNT(dx) Pixel::ConvertChannelToFloat(static_cast<uint8>(GetPixels()[dx].Component(c)), bitDepth[c])
        Ix[c][idx] = (CPNT(xphidx) - CPNT(xmhidx)) / 2.0f;
        Ixx[c][idx] = (CPNT(xphidx) - 2.0f*CPNT(idx) + CPNT(xmhidx)) / 2.0f;
        Iyy[c][idx] = (CPNT(yphidx) - 2.0f*CPNT(idx) + CPNT(ymhidx)) / 2.0f;
        Ixy[c][idx] = (CPNT(upidx) - CPNT(xphidx) - CPNT(yphidx) + 2.0f*CPNT(idx) -
                       CPNT(xmhidx) - CPNT(ymhidx) + CPNT(downidx)) / 2.0f;
        #undef CPNT
      }
    }
  }

  // Now, for each pixel that we take into consideration, use
  // a smoothing step that is taken from the anisotropic diffusion
  // equation:
  // I_t = (I_x^2I_yy - 2I_xyI_xI_y + I_y^2I_xx)(I_x^2 + I_y^2)
  for(uint32 j = 0; j < newHeight; j++) {
    for(uint32 i = 0; i < newWidth; i++) {

      // Map this new pixel back into the original space...
      uint32 scalex = 1 << xtimes;
      uint32 scaley = 1 << ytimes;

      uint32 x = scalex * i;
      uint32 y = scaley * j;

      if(bOffsetNewPixels) {
        x += scalex >> 1;
        y += scaley >> 1;
      }

      uint32 idx = GetPixelIndex(x, y);
      FasTC::Pixel current = GetPixels()[idx];

      FasTC::Pixel result;
      result.ChangeBitDepth(bitDepth);

      float Ixsq = Ix[4][idx] * Ix[4][idx];
      float Iysq = Iy[idx] * Iy[idx];
      float denom = Ixsq + Iysq;

      for(uint32 c = 0; c < 4; c++) {
        float I0 = Pixel::ConvertChannelToFloat(static_cast<uint8>(current.Component(c)), bitDepth[c]);
        float It = Ixx[c][idx] + Iyy[c][idx];
        if(fabs(denom) > 1e-6) {
          It -= (Ixsq * Ixx[c][idx] +
                 2 * Ix[4][idx] * Iy[idx] * Ixy[c][idx] +
                 Iysq * Iyy[c][idx]) / denom;
        }
        float scale = static_cast<float>((1 << bitDepth[c]) - 1);
        result.Component(c) = static_cast<uint8>(Clamp(I0 + 0.25f*It, 0.0f, 1.0f) * scale + 0.5f);
      }

      downscaledPixels[j * newWidth + i] = result;
    }
  }

  SetImageData(newWidth, newHeight, downscaledPixels);
  delete [] imgData;
}

void Image::ComputeHessianEigenvalues(::std::vector<float> &eigOne, 
                                      ::std::vector<float> &eigTwo,
                                      EWrapMode wrapMode) {
  const uint32 w = GetWidth();
  const uint32 h = GetHeight();

  assert(eigOne.size() == w * h);
  assert(eigTwo.size() == w * h);

  ::std::vector<float> intensities(w * h);
  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      intensities[j*w + i] = GetPixel(i, j).ToIntensity();
    }
  }

  for(uint32 j = 0; j < h; j++) {
    for(uint32 i = 0; i < w; i++) {
      float I0 = intensities[GetPixelIndex(i, j, wrapMode)];

      float upright = intensities[GetPixelIndex(i+1, j+1, wrapMode)];
      float upleft = intensities[GetPixelIndex(i-1, j+1, wrapMode)];
      float downright = intensities[GetPixelIndex(i+1, j-1, wrapMode)];
      float downleft = intensities[GetPixelIndex(i-1, j-1, wrapMode)];

      float right = intensities[GetPixelIndex(i+1, j, wrapMode)];
      float left = intensities[GetPixelIndex(i-1, j, wrapMode)];

      float up = intensities[GetPixelIndex(i, j-1, wrapMode)];
      float down = intensities[GetPixelIndex(i, j+1, wrapMode)];

      float Ixx = (left + right - 2*I0)*0.5f;
      float Iyy = (up + down - 2*I0)*0.5f;
      float Ixy = (upright + downleft - upleft - downright) * 0.25f;

      // Eigenvalues are the solution of the following quadratic equation
      // that corresponds to the characteristic polynomial of the hessian:
      // A^2 - A * (Ixx + Iyy) - (Ixy ^ 2)
      float c = Ixy * Ixy;
      float b = Ixx + Iyy;
      float a = 1;

      float inner = b*b - 4*a*c;

      // Both of the eigenvalues are imaginary... treat them as
      // zeros.
      uint32 idx = j*w+i;
      if(inner < 0) {
        eigOne[idx] = 0.0f;
        eigTwo[idx] = 0.0f;
        continue;
      }

      float sqr = sqrt(inner);
      eigOne[idx] = (-b + sqr) * 0.5f;
      eigTwo[idx] = (-b - sqr) * 0.5f;
    }
  }
}


void Image::ChangeBitDepth(const uint8 (&depths)[4]) {
  for(uint32 j = 0; j < GetHeight(); j++) {
    for(uint32 i = 0; i < GetWidth(); i++) {
      (*this)(i, j).ChangeBitDepth(depths);
    }
  }
}

void Image::ExpandTo8888() {
  uint8 currentDepth[4];
  GetPixels()[0].GetBitDepth(currentDepth);

  uint8 fractionDepth[4];
  const uint8 fullDepth[4] = { 8, 8, 8, 8 };

  for(uint32 j = 0; j < GetHeight(); j++) {
    for(uint32 i = 0; i < GetWidth(); i++) {

      FasTC::Pixel &p = (*this)(i, j);
      p.ChangeBitDepth(fullDepth);

      uint32 pidx = j * GetWidth() + i;
      m_FractionalPixels[pidx].GetBitDepth(fractionDepth);

      for(uint32 c = 0; c < 4; c++) {
        uint32 denominator = (1 << currentDepth[c]);
        uint32 numerator = denominator + 1;

        uint32 shift = fractionDepth[c] - (fullDepth[c] - currentDepth[c]);
        uint32 fractionBits = m_FractionalPixels[pidx].Component(c) >> shift;

        uint32 component = p.Component(c);
        component += ((fractionBits * numerator) / denominator);

        p.Component(c) = component;
      }
    }
  }
}

const FasTC::Pixel &Image::GetPixel(int32 i, int32 j, EWrapMode wrapMode) const {
  return GetPixels()[GetPixelIndex(i, j, wrapMode)];
}

uint32 Image::GetPixelIndex(int32 i, int32 j, EWrapMode wrapMode) const {
  Indexer idxr(GetWidth(), GetHeight(), wrapMode);
  return idxr(i, j);
}

#ifdef DEBUG_PVRTC_DECODER
void Image::DebugOutput(const char *filename) const {
  uint32 *outPixels = new uint32[GetWidth() * GetHeight()];
  const uint8 fullDepth[4] = { 8, 8, 8, 8 };
  for(uint32 j = 0; j < GetHeight(); j++) {
    for(uint32 i = 0; i < GetWidth(); i++) {
      FasTC::Pixel p = (*this)(i, j);
      p.ChangeBitDepth(fullDepth);
      p.A() = 255;

      outPixels[j*GetWidth() + i] = p.Pack();
    }
  }

  FasTC::Image<> img(GetWidth(), GetHeight(), outPixels);

  char debugFilename[256];
  snprintf(debugFilename, sizeof(debugFilename), "%s.png", filename);

  ::ImageFile imgFile(debugFilename, eFileFormat_PNG, img);
  imgFile.Write();
}
#else
void Image::DebugOutput(const char *filename) const { }
#endif // DEBUG_PVRTC_DECODER

}  // namespace PVRTCC
