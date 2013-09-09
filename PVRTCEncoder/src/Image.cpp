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

#include "Image.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#include "Pixel.h"

#include "Core/include/Image.h"
#include "IO/include/ImageFile.h"

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
  : m_Width(other.m_Width)
  , m_Height(other.m_Height)
  , m_Pixels(new Pixel[other.m_Width * other.m_Height])
  , m_FractionalPixels(new Pixel[other.m_Width * other.m_Height]) {
  memcpy(m_Pixels, other.m_Pixels, m_Width * m_Height * sizeof(Pixel));
}

Image &Image::operator=(const Image &other) {
  m_Width = other.m_Width;
  m_Height = other.m_Height;

  assert(m_Pixels);
  delete m_Pixels;
  m_Pixels = new Pixel[other.m_Width * other.m_Height];
  memcpy(m_Pixels, other.m_Pixels, m_Width * m_Height * sizeof(Pixel));

  assert(m_FractionalPixels);
  delete m_FractionalPixels;
  m_FractionalPixels = new Pixel[other.m_Width * other.m_Height];
  memcpy(m_FractionalPixels, other.m_FractionalPixels,
         m_Width * m_Height * sizeof(Pixel));

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

void Image::BilinearUpscale(uint32 times, EWrapMode wrapMode) {
  const uint32 newWidth = m_Width << times;
  const uint32 newHeight = m_Height << times;

  const uint32 scale = 1 << times;
  const uint32 offset = scale >> 1;

  Pixel *upscaledPixels = new Pixel[newWidth * newHeight];

  assert(m_FractionalPixels);
  delete m_FractionalPixels;
  m_FractionalPixels = new Pixel[newWidth * newHeight];

  for(uint32 j = 0; j < newHeight; j++) {
    for(uint32 i = 0; i < newWidth; i++) {

      const uint32 pidx = j * newWidth + i;
      Pixel &p = upscaledPixels[pidx];
      Pixel &fp = m_FractionalPixels[pidx];

      const int32 highXIdx = (i + offset) / scale;
      const int32 lowXIdx = highXIdx - 1;
      const int32 highYIdx = (j + offset) / scale;
      const int32 lowYIdx = highYIdx - 1;

      const uint32 highXWeight = (i + offset) % scale;
      const uint32 lowXWeight = scale - highXWeight;
      const uint32 highYWeight = (j + offset) % scale;
      const uint32 lowYWeight = scale - highYWeight;

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
      const uint16 scaleMask = (scale * scale) - 1;
      uint8 fpDepths[4];
      for(uint32 c = 0; c < 4; c++) fpDepths[c] = times * times;
      fp.ChangeBitDepth(fpDepths);

      for(uint32 c = 0; c < 4; c++) {
        const uint32 tl = topLeft.Component(c) * topLeftWeight;
        const uint32 tr = topRight.Component(c) * topRightWeight;
        const uint32 bl = bottomLeft.Component(c) * bottomLeftWeight;
        const uint32 br = bottomRight.Component(c) * bottomRightWeight;
        const uint32 sum = tl + tr + bl + br;
        fp.Component(c) = sum & scaleMask;
        p.Component(c) = sum / (scale * scale);
      }
    }
  }

  delete m_Pixels;
  m_Pixels = upscaledPixels;
  m_Width = newWidth;
  m_Height = newHeight;
}

void Image::ChangeBitDepth(const uint8 (&depths)[4]) {
  for(uint32 j = 0; j < m_Height; j++) {
    for(uint32 i = 0; i < m_Width; i++) {
      uint32 pidx = j * m_Width + i;
      m_Pixels[pidx].ChangeBitDepth(depths);
    }
  }
}

void Image::ExpandTo8888() {
  uint8 currentDepth[4];
  m_Pixels[0].GetBitDepth(currentDepth);

  uint8 fractionDepth[4];
  const uint8 fullDepth[4] = { 8, 8, 8, 8 };

  for(uint32 j = 0; j < m_Height; j++) {
    for(uint32 i = 0; i < m_Width; i++) {

      uint32 pidx = j * m_Width + i;
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

const Pixel &Image::GetPixel(int32 i, int32 j, EWrapMode wrapMode) {
  while(i < 0) {
    if(wrapMode == eWrapMode_Clamp) {
      i = 0;
    } else {
      i += m_Width;
    }
  }

  while(i >= static_cast<int32>(m_Width)) {
    if(wrapMode == eWrapMode_Clamp) {
      i = m_Width - 1;
    } else {
      i -= m_Width;
    }
  }

  while(j < 0) {
    if(wrapMode == eWrapMode_Clamp) {
      j = 0;
    } else {
      j += m_Height;
    }
  }

  while(j >= static_cast<int32>(m_Height)) {
    if(wrapMode == eWrapMode_Clamp) {
      j = m_Height - 1;
    } else {
      j -= m_Height;
    }
  }

  return m_Pixels[j * m_Width + i];
}

Pixel & Image::operator()(uint32 i, uint32 j) {
  assert(i < m_Width);
  assert(j < m_Height);
  return m_Pixels[j * m_Width + i];
}

const Pixel & Image::operator()(uint32 i, uint32 j) const {
  assert(i < m_Width);
  assert(j < m_Height);
  return m_Pixels[j * m_Width + i];
}

void Image::DebugOutput(const char *filename) const {
  uint32 *outPixels = new uint32[m_Width * m_Height];
  const uint8 fullDepth[4] = { 8, 8, 8, 8 };
  for(int j = 0; j < m_Height; j++) {
    for(int i = 0; i < m_Width; i++) {
      uint32 idx = j * m_Width + i;
      Pixel p = m_Pixels[idx];
      p.ChangeBitDepth(fullDepth);
      outPixels[idx] = p.PackRGBA();
    }
  }

  ::Image img(m_Height, m_Width, outPixels);

  char debugFilename[256];
  snprintf(debugFilename, sizeof(debugFilename), "%s.png", filename);

  ::ImageFile imgFile(debugFilename, eFileFormat_PNG, img);
  imgFile.Write();
}

}  // namespace PVRTCC
