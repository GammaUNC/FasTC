/* FasTC
 * Copyright (c) 2013 University of North Carolina at Chapel Hill.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for educational, research, and non-profit purposes, without
 * fee, and without a written agreement is hereby granted, provided that the
 * above copyright notice, this paragraph, and the following four paragraphs
 * appear in all copies.
 *
 * Permission to incorporate this software into commercial products may be
 * obtained by contacting the authors or the Office of Technology Development
 * at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of
 * North Carolina at Chapel Hill. The software program and documentation are
 * supplied "as is," without any accompanying services from the University of
 * North Carolina at Chapel Hill or the authors. The University of North
 * Carolina at Chapel Hill and the authors do not warrant that the operation of
 * the program will be uninterrupted or error-free. The end-user understands
 * that the program was developed for research purposes and is advised not to
 * rely exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE
 * AUTHORS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF
 * THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA
 * AT CHAPEL HILL OR THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY
 * DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON
 * AN "AS IS" BASIS, AND THE UNIVERSITY  OF NORTH CAROLINA AT CHAPEL HILL AND
 * THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
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

#if _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#  define snprintf _snprintf
#endif

#include "Image.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>

#include "Pixel.h"

#include "../../Base/include/Image.h"
#include "../../IO/include/ImageFile.h"

template <typename T>
inline T Clamp(const T &v, const T &a, const T &b) {
  return ::std::min(::std::max(a, v), b);
}

namespace PVRTCC {

Image::Image(uint32 height, uint32 width)
  : m_Width(width)
  , m_Height(height)
  , m_Pixels(new Pixel[width * height])
  , m_FractionalPixels(new Pixel[width * height]) {
  assert(width > 0);
  assert(height > 0);
}

Image::Image(uint32 height, uint32 width, const Pixel *pixels)
  : m_Width(width)
  , m_Height(height)
  , m_Pixels(new Pixel[width * height])
  , m_FractionalPixels(new Pixel[width * height]) {
  assert(width > 0);
  assert(height > 0);
  memcpy(m_Pixels, pixels, width * height * sizeof(Pixel));
}

Image::Image(const Image &other)
  : m_Width(other.GetWidth())
  , m_Height(other.GetHeight())
  , m_Pixels(new Pixel[other.GetWidth() * other.GetHeight()])
  , m_FractionalPixels(new Pixel[other.GetWidth() * other.GetHeight()]) {
  memcpy(m_Pixels, other.m_Pixels, GetWidth() * GetHeight() * sizeof(Pixel));
}

Image &Image::operator=(const Image &other) {
  m_Width = other.GetWidth();
  m_Height = other.GetHeight();

  assert(m_Pixels);
  delete m_Pixels;
  m_Pixels = new Pixel[other.GetWidth() * other.GetHeight()];
  memcpy(m_Pixels, other.m_Pixels, GetWidth() * GetHeight() * sizeof(Pixel));

  assert(m_FractionalPixels);
  delete m_FractionalPixels;
  m_FractionalPixels = new Pixel[other.GetWidth() * other.GetHeight()];
  memcpy(m_FractionalPixels, other.m_FractionalPixels,
         GetWidth() * GetHeight() * sizeof(Pixel));

  return *this;
}

Image::~Image() {
  assert(m_Pixels);
  delete [] m_Pixels;

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

  Pixel *upscaledPixels = new Pixel[newWidth * newHeight];

  assert(m_FractionalPixels);
  delete m_FractionalPixels;
  m_FractionalPixels = new Pixel[newWidth * newHeight];

  for(uint32 j = 0; j < newHeight; j++) {
    for(uint32 i = 0; i < newWidth; i++) {

      const uint32 pidx = j * newWidth + i;
      Pixel &p = upscaledPixels[pidx];
      Pixel &fp = m_FractionalPixels[pidx];

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

      const Pixel &topLeft = GetPixel(lowXIdx, lowYIdx, wrapMode);
      const Pixel &topRight = GetPixel(highXIdx, lowYIdx, wrapMode);
      const Pixel &bottomLeft = GetPixel(lowXIdx, highYIdx, wrapMode);
      const Pixel &bottomRight = GetPixel(highXIdx, highYIdx, wrapMode);

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

      for(uint32 c = 0; c < 4; c++) {
        const uint32 tl = topLeft.Component(c) * topLeftWeight;
        const uint32 tr = topRight.Component(c) * topRightWeight;
        const uint32 bl = bottomLeft.Component(c) * bottomLeftWeight;
        const uint32 br = bottomRight.Component(c) * bottomRightWeight;
        const uint32 sum = tl + tr + bl + br;
        fp.Component(c) = sum & scaleMask;
        p.Component(c) = sum / (xscale * yscale);
      }
    }
  }

  delete m_Pixels;
  m_Pixels = upscaledPixels;
  m_Width = newWidth;
  m_Height = newHeight;
}

void Image::ContentAwareDownscale(uint32 xtimes, uint32 ytimes,
                                  EWrapMode wrapMode, bool bOffsetNewPixels) {
  const uint32 w = GetWidth();
  const uint32 h = GetHeight();

  const uint32 newWidth = w >> xtimes;
  const uint32 newHeight = h >> ytimes;

  Pixel *downscaledPixels = new Pixel[newWidth * newHeight];
  const uint32 numDownscaledPixels = newWidth * newHeight;

  uint8 bitDepth[4];
  m_Pixels[0].GetBitDepth(bitDepth);

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
    I[i] = m_Pixels[i].ToIntensity();
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
#define CPNT(dx) Pixel::ConvertChannelToFloat(m_Pixels[dx].Component(c), bitDepth[c])
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
      Pixel current = m_Pixels[idx];

      Pixel result;
      result.ChangeBitDepth(bitDepth);

      float Ixsq = Ix[4][idx] * Ix[4][idx];
      float Iysq = Iy[idx] * Iy[idx];
      float denom = Ixsq + Iysq;

      for(uint32 c = 0; c < 4; c++) {
        float I0 = Pixel::ConvertChannelToFloat(current.Component(c), bitDepth[c]);
        float It = Ixx[c][idx] + Iyy[c][idx];
        if(fabs(denom) > 1e-6) {
          It -= (Ixsq * Ixx[c][idx] +
                 2 * Ix[4][idx] * Iy[idx] * Ixy[c][idx] +
                 Iysq * Iyy[c][idx]) / denom;
        }
        float scale = static_cast<float>((1 << bitDepth[c]) - 1);
        result.Component(c) = static_cast<uint8>(Clamp(I0 + 0.25f*It, 0.0f, 1.0f) * scale + 0.5f);
      }

      downscaledPixels[j * newHeight + i] = result;
    }
  }

  delete m_Pixels;
  m_Pixels = downscaledPixels;
  m_Width = newWidth;
  m_Height = newHeight;

  delete [] imgData;
}

void Image::ChangeBitDepth(const uint8 (&depths)[4]) {
  for(uint32 j = 0; j < GetHeight(); j++) {
    for(uint32 i = 0; i < GetWidth(); i++) {
      uint32 pidx = j * GetWidth() + i;
      m_Pixels[pidx].ChangeBitDepth(depths);
    }
  }
}

void Image::ExpandTo8888() {
  uint8 currentDepth[4];
  m_Pixels[0].GetBitDepth(currentDepth);

  uint8 fractionDepth[4];
  const uint8 fullDepth[4] = { 8, 8, 8, 8 };

  for(uint32 j = 0; j < GetHeight(); j++) {
    for(uint32 i = 0; i < GetWidth(); i++) {

      uint32 pidx = j * GetWidth() + i;
      m_Pixels[pidx].ChangeBitDepth(fullDepth);
      m_FractionalPixels[pidx].GetBitDepth(fractionDepth);

      for(uint32 c = 0; c < 4; c++) {
        uint32 denominator = (1 << currentDepth[c]);
        uint32 numerator = denominator + 1;

        uint32 shift = fractionDepth[c] - (fullDepth[c] - currentDepth[c]);
        uint32 fractionBits = m_FractionalPixels[pidx].Component(c) >> shift;

        uint32 component = m_Pixels[pidx].Component(c);
        component += ((fractionBits * numerator) / denominator);

        m_Pixels[pidx].Component(c) = component;
      }
    }
  }
}

const Pixel &Image::GetPixel(int32 i, int32 j, EWrapMode wrapMode) const {
  return m_Pixels[GetPixelIndex(i, j, wrapMode)];
}

const uint32 Image::GetPixelIndex(int32 i, int32 j, EWrapMode wrapMode) const {
  while(i < 0) {
    if(wrapMode == eWrapMode_Clamp) {
      i = 0;
    } else {
      i += GetWidth();
    }
  }

  while(i >= static_cast<int32>(GetWidth())) {
    if(wrapMode == eWrapMode_Clamp) {
      i = GetWidth() - 1;
    } else {
      i -= GetWidth();
    }
  }

  while(j < 0) {
    if(wrapMode == eWrapMode_Clamp) {
      j = 0;
    } else {
      j += GetHeight();
    }
  }

  while(j >= static_cast<int32>(GetHeight())) {
    if(wrapMode == eWrapMode_Clamp) {
      j = GetHeight() - 1;
    } else {
      j -= GetHeight();
    }
  }

  uint32 idx = j * GetWidth() + i;
  assert(idx >= 0);
  assert(idx < GetWidth() * GetHeight());
  return idx;
}

Pixel & Image::operator()(uint32 i, uint32 j) {
  assert(i < GetWidth());
  assert(j < GetHeight());
  return m_Pixels[j * GetWidth() + i];
}

const Pixel & Image::operator()(uint32 i, uint32 j) const {
  assert(i < GetWidth());
  assert(j < GetHeight());
  return m_Pixels[j * GetWidth() + i];
}

void Image::DebugOutput(const char *filename) const {
  uint32 *outPixels = new uint32[GetWidth() * GetHeight()];
  const uint8 fullDepth[4] = { 8, 8, 8, 8 };
  for(uint32 j = 0; j < GetHeight(); j++) {
    for(uint32 i = 0; i < GetWidth(); i++) {
      uint32 idx = j * GetWidth() + i;
      Pixel p = m_Pixels[idx];
      p.ChangeBitDepth(fullDepth);
      p.A() = 255;

      outPixels[idx] = p.PackRGBA();
    }
  }

  ::Image img(GetWidth(), GetHeight(), outPixels);

  char debugFilename[256];
  snprintf(debugFilename, sizeof(debugFilename), "%s.png", filename);

  ::ImageFile imgFile(debugFilename, eFileFormat_PNG, img);
  imgFile.Write();
}

}  // namespace PVRTCC
