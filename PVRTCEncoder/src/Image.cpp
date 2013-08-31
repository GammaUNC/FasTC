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

#include "Pixel.h"

namespace PVRTCC {

Image::Image(uint32 height, uint32 width)
  : m_Width(width)
  , m_Height(height)
  , m_Pixels(new Pixel[width * height]) {
  assert(width > 0);
  assert(height > 0);
  memset(m_Pixels, 0, width * height * sizeof(Pixel));
}

Image::Image(uint32 height, uint32 width, const Pixel *pixels)
  : m_Width(width)
  , m_Height(height)
  , m_Pixels(new Pixel[width * height]) {
  assert(width > 0);
  assert(height > 0);
  memcpy(m_Pixels, pixels, width * height * sizeof(Pixel));
}

Image::Image(const Image &other)
  : m_Width(other.m_Width)
  , m_Height(other.m_Height)
  , m_Pixels(new Pixel[other.m_Width * other.m_Height]) {
  memcpy(m_Pixels, other.m_Pixels, m_Width * m_Height * sizeof(Pixel));
}

Image &Image::operator=(const Image &other) {
  m_Width = other.m_Width;
  m_Height = other.m_Height;
  m_Pixels = new Pixel[other.m_Width * other.m_Height];
  memcpy(m_Pixels, other.m_Pixels, m_Width * m_Height * sizeof(Pixel));
  return *this;
}

Image::~Image() {
  assert(m_Pixels);
  delete [] m_Pixels;
}

void Image::BilinearUpscale(uint32 times, EWrapMode wrapMode) {
  const uint32 newWidth = m_Width << times;
  const uint32 newHeight = m_Height << times;

  const uint32 scale = 1 << times;
  const uint32 offset = scale >> 1;

  Pixel *upscaledPixels = new Pixel[newWidth * newHeight];

  for(uint32 j = 0; j < newHeight; j++) {
    for(uint32 i = 0; i < newWidth; i++) {

      Pixel &p = upscaledPixels[j * newWidth + i];

      int32 highXIdx = (i + offset) / scale;
      int32 lowXIdx = highXIdx - 1;
      int32 highYIdx = (j + offset) / scale;
      int32 lowYIdx = highYIdx - 1;

      uint32 lowXWeight = (i + offset) % scale;
      uint32 highXWeight = scale - lowXWeight;
      uint32 lowYWeight = (j + offset) % scale;
      uint32 highYWeight = scale - lowYWeight;

      const Pixel &topLeft = GetPixel(highXIdx, lowYIdx, wrapMode);
      const Pixel &topRight = GetPixel(lowXIdx, lowYIdx, wrapMode);
      const Pixel &bottomLeft = GetPixel(highXIdx, highYIdx, wrapMode);
      const Pixel &bottomRight = GetPixel(lowXIdx, highYIdx, wrapMode);

      // bilerp each channel....
      for(uint32 c = 0; c < 4; c++) {
        const uint16 left =
          (lowYWeight * static_cast<uint16>(topLeft.Component(c)) +
           highYWeight * static_cast<uint16>(bottomLeft.Component(c)))
          >> scale;
        const uint16 right =
          (lowYWeight * static_cast<uint16>(topRight.Component(c)) +
           highYWeight * static_cast<uint16>(bottomRight.Component(c)))
          >> scale;

        p.Component(c) = (left * lowXWeight + right * highXWeight) >> scale;
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
      m_Pixels[j * m_Width + i].ChangeBitDepth(depths);
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
  assert(i > m_Height);
  return m_Pixels[j * m_Width + i];
}

const Pixel & Image::operator()(uint32 i, uint32 j) const {
  assert(i < m_Width);
  assert(i > m_Height);
  return m_Pixels[j * m_Width + i];
}

}  // namespace PVRTCC
