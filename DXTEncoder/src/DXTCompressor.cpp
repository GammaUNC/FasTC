/*
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.
*/

// Refer to "Real-Time DXT Compression" by J.M.P. van Waveren for a more thorough discussion of the
// algorithms used in this code.

#include "DXTCompressor.h"
#include <cassert>
#include <cstdlib>
#include <cstring>

#define INSET_SHIFT 4 // Inset the bounding box with (range >> shift).
#define C565_5_MASK 0xF8 // 0xFF minus last three bits
#define C565_6_MASK 0xFC // 0xFF minus last two bits

namespace DXTC
{
  // Function prototypes
  void ExtractBlock(const uint8* inPtr, uint32 width, uint8* colorBlock);
  void GetMinMaxColors(const uint8* colorBlock, uint8* minColor, uint8* maxColor);
  void GetMinMaxColorsWithAlpha(const uint8* colorBlock, uint8* minColor, uint8* maxColor);
  void EmitColorIndices(const uint8* colorBlock, uint8*& outBuf, const uint8* minColor, const uint8* maxColor);
  void EmitAlphaIndices(const uint8* colorBlock,  uint8*& outBuf, const uint8 minAlpha, const uint8 maxAlpha);

  // Compress an image using DXT1 compression. Use the inBuf parameter to point to an image in
  // 4-byte RGBA format. The width and height parameters specify the size of the image in pixels.
  // The buffer pointed to by outBuf should be large enough to store the compressed image. This
  // implementation has an 8:1 compression ratio.
  void CompressImageDXT1(const CompressionJob &cj) {
    uint8 block[64];
    uint8 minColor[4];
    uint8 maxColor[4];

    uint8 *outBuf = cj.OutBuf();
    const uint8 *inBuf = cj.InBuf();
    for(int j = 0; j < cj.Height(); j += 4, inBuf += cj.Width() * 4 * 4)
    {
      for(int i = 0; i < cj.Width(); i += 4)
      {
        ExtractBlock(inBuf + i * 4, cj.Width(), block);
        GetMinMaxColors(block, minColor, maxColor);
        EmitWord(outBuf, ColorTo565(maxColor));
        EmitWord(outBuf, ColorTo565(minColor));
        EmitColorIndices(block, outBuf, minColor, maxColor);
      }
    }
  }

  // Compress an image using DXT5 compression. Use the inBuf parameter to point to an image in
  // 4-byte RGBA format. The width and height parameters specify the size of the image in pixels.
  // The buffer pointed to by outBuf should be large enough to store the compressed image. This
  // implementation has an 4:1 compression ratio.
  void CompressImageDXT5(const CompressionJob &cj) {
    uint8 block[64];
    uint8 minColor[4];
    uint8 maxColor[4];

    uint8 *outBuf = cj.OutBuf();
    const uint8 *inBuf = cj.InBuf();
    for(int j = 0; j < cj.Height(); j += 4, inBuf += cj.Width() * 4 * 4)
    {
      for(int i = 0; i < cj.Width(); i += 4)
      {
        ExtractBlock(inBuf + i * 4, cj.Width(), block);
        GetMinMaxColorsWithAlpha(block, minColor, maxColor);
        EmitByte(outBuf, maxColor[3]);
        EmitByte(outBuf, minColor[3]);
        EmitAlphaIndices(block, outBuf, minColor[3], maxColor[3]);
        EmitWord(outBuf, ColorTo565(maxColor));
        EmitWord(outBuf, ColorTo565(minColor));
        EmitColorIndices(block, outBuf, minColor, maxColor);
      }
    }
  }

  // Convert a color in 24-bit RGB888 format to 16-bit RGB565 format.
  uint16 ColorTo565(const uint8* color)
  {
    return ((color[0] >> 3) << 11) | ((color[1] >> 2) << 5) | (color[2] >> 3);
  }

  // Write a single byte to dest.
  void EmitByte(uint8*& dest, uint8 b)
  {
    dest[0] = b;
    dest += 1;
  }

  // Write a word to dest.
  void EmitWord(uint8*& dest, uint16 s)
  {
    dest[0] = (s >> 0) & 255;
    dest[1] = (s >> 8) & 255;
    dest += 2;
  }

  // Write a double word to dest.
  void EmitDoubleWord(uint8*& dest, uint32 i)
  {
    dest[0] = (i >> 0) & 255;
    dest[1] = (i >> 8) & 255;
    dest[2] = (i >> 16) & 255;
    dest[3] = (i >> 24) & 255;
    dest += 4;
  }

  // Extract a 4 by 4 block of pixels from inPtr and store it in colorBlock. The width parameter
  // specifies the size of the image in pixels.
  void ExtractBlock(const uint8* inPtr, uint32 width, uint8* colorBlock)
  {
    for(int j = 0; j < 4; j++)
    {
      memcpy(&colorBlock[j * 4 * 4], inPtr, 4 * 4);
      inPtr += width * 4;
    }
  }

  // Find a line of best fit through the color space of colorBlock. The line is approximated using
  // the extents of the bounding box of the color space. This function does not include the alpha
  // channel.
  void GetMinMaxColors(const uint8* colorBlock, uint8* minColor, uint8* maxColor)
  {
    int32 i;
    uint8 inset[3];

    minColor[0] = minColor[1] = minColor[2] = 255;
    maxColor[0] = maxColor[1] = maxColor[2] = 0;

    // Find the bounding box (defined by minimum and maximum color).
    for(i = 0; i < 16; i++) {
      if(colorBlock[i * 4 + 0] < minColor[0]) {
        minColor[0] = colorBlock[i * 4 + 0];
      }
      if(colorBlock[i * 4 + 1] < minColor[1]) {
        minColor[1] = colorBlock[i * 4 + 1];
      }
      if(colorBlock[i * 4 + 2] < minColor[2]) {
        minColor[2] = colorBlock[i * 4 + 2];
      }
      if(colorBlock[i * 4 + 0] > maxColor[0]) {
        maxColor[0] = colorBlock[i * 4 + 0];
      }
      if(colorBlock[i * 4 + 1] > maxColor[1]) {
        maxColor[1] = colorBlock[i * 4 + 1];
      }
      if(colorBlock[i * 4 + 2] > maxColor[2]) {
        maxColor[2] = colorBlock[i * 4 + 2];
      }
    }

    // Inset the bounding box by 1/16 of it's size. (i.e. shift right by 4).
    inset[0] = (maxColor[0] - minColor[0]) >> INSET_SHIFT;
    inset[1] = (maxColor[1] - minColor[1]) >> INSET_SHIFT;
    inset[2] = (maxColor[2] - minColor[2]) >> INSET_SHIFT;

    // Clamp the inset bounding box to 255.
    minColor[0] = (minColor[0] + inset[0] <= 255) ? minColor[0] + inset[0] : 255;
    minColor[1] = (minColor[1] + inset[1] <= 255) ? minColor[1] + inset[1] : 255;
    minColor[2] = (minColor[2] + inset[2] <= 255) ? minColor[2] + inset[2] : 255;

    // Clamp the inset bounding box to 0.
    maxColor[0] = (maxColor[0] >= inset[0]) ? maxColor[0] - inset[0] : 0;
    maxColor[1] = (maxColor[1] >= inset[1]) ? maxColor[1] - inset[1] : 0;
    maxColor[2] = (maxColor[2] >= inset[2]) ? maxColor[2] - inset[2] : 0;
  }

  // Find a line of best fit through the color space of colorBlock. The line is approximated using
  // the extents of the bounding box of the color space. This function includes the alpha channel.
  void GetMinMaxColorsWithAlpha(const uint8* colorBlock, uint8* minColor, uint8* maxColor)
  {
    int32 i;
    uint8 inset[4];

    minColor[0] = minColor[1] = minColor[2] = minColor[3] = 255;
    maxColor[0] = maxColor[1] = maxColor[2] = maxColor[3] = 0;

    // Find the bounding box (defined by minimum and maximum color).
    for(i = 0; i < 16; i++) {
      if(colorBlock[i * 4 + 0] < minColor[0]) {
        minColor[0] = colorBlock[i * 4 + 0];
      }
      if(colorBlock[i * 4 + 1] < minColor[1]) {
        minColor[1] = colorBlock[i * 4 + 1];
      }
      if(colorBlock[i * 4 + 2] < minColor[2]) {
        minColor[2] = colorBlock[i * 4 + 2];
      }
      if(colorBlock[i * 4 + 3] < minColor[3]) {
        minColor[3] = colorBlock[i * 4 + 3];
      }
      if(colorBlock[i * 4 + 0] > maxColor[0]) {
        maxColor[0] = colorBlock[i * 4 + 0];
      }
      if(colorBlock[i * 4 + 1] > maxColor[1]) {
        maxColor[1] = colorBlock[i * 4 + 1];
      }
      if(colorBlock[i * 4 + 2] > maxColor[2]) {
        maxColor[2] = colorBlock[i * 4 + 2];
      }
      if(colorBlock[i * 4 + 3] > maxColor[3]) {
        maxColor[3] = colorBlock[i * 4 + 3];
      }
    }

    // Inset the bounding box by 1/16 of it's size. (i.e. shift right by 4).
    inset[0] = (maxColor[0] - minColor[0]) >> INSET_SHIFT;
    inset[1] = (maxColor[1] - minColor[1]) >> INSET_SHIFT;
    inset[2] = (maxColor[2] - minColor[2]) >> INSET_SHIFT;
    inset[3] = (maxColor[3] - minColor[3]) >> INSET_SHIFT;

    // Clamp the inset bounding box to 255.
    minColor[0] = (minColor[0] + inset[0] <= 255) ? minColor[0] + inset[0] : 255;
    minColor[1] = (minColor[1] + inset[1] <= 255) ? minColor[1] + inset[1] : 255;
    minColor[2] = (minColor[2] + inset[2] <= 255) ? minColor[2] + inset[2] : 255;
    minColor[3] = (minColor[3] + inset[3] <= 255) ? minColor[3] + inset[3] : 255;

    // Clamp the inset bounding box to 0.
    maxColor[0] = (maxColor[0] >= inset[0]) ? maxColor[0] - inset[0] : 0;
    maxColor[1] = (maxColor[1] >= inset[1]) ? maxColor[1] - inset[1] : 0;
    maxColor[2] = (maxColor[2] >= inset[2]) ? maxColor[2] - inset[2] : 0;
    maxColor[3] = (maxColor[3] >= inset[3]) ? maxColor[3] - inset[3] : 0;
  }

  // Quantize the pixels of the colorBlock to 4 colors that lie on the line through the color space
  // of colorBlock.  The paramaters minColor and maxColor approximate the line through the color
  // space.  32 bits (2 bits per pixel) are written to outBuf, which represent the indices of the 4
  // colors. This function does not include the alpha channel.
  void EmitColorIndices(const uint8* colorBlock,  uint8*& outBuf, const uint8* minColor, const uint8* maxColor)
  {
    uint16 colors[4][4];
    uint32 result = 0;

    colors[0][0] = (maxColor[0] & C565_5_MASK) | (maxColor[0] >> 5);
    colors[0][1] = (maxColor[1] & C565_6_MASK) | (maxColor[1] >> 6);
    colors[0][2] = (maxColor[2] & C565_5_MASK) | (maxColor[2] >> 5);
    colors[1][0] = (minColor[0] & C565_5_MASK) | (minColor[0] >> 5);
    colors[1][1] = (minColor[1] & C565_6_MASK) | (minColor[1] >> 6);
    colors[1][2] = (minColor[2] & C565_5_MASK) | (minColor[2] >> 5);
    colors[2][0] = (2 * colors[0][0] + 1 * colors[1][0]) / 3;
    colors[2][1] = (2 * colors[0][1] + 1 * colors[1][1]) / 3;
    colors[2][2] = (2 * colors[0][2] + 1 * colors[1][2]) / 3;
    colors[3][0] = (1 * colors[0][0] + 2 * colors[1][0]) / 3;
    colors[3][1] = (1 * colors[0][1] + 2 * colors[1][1]) / 3;
    colors[3][2] = (1 * colors[0][2] + 2 * colors[1][2]) / 3;

    for(int i = 15; i >= 0; i--) {
      int32 c0 = colorBlock[i * 4 + 0];
      int32 c1 = colorBlock[i * 4 + 1];
      int32 c2 = colorBlock[i * 4 + 2];

      int32 d0 = abs(colors[0][0] - c0) + abs(colors[0][1] - c1) + abs(colors[0][2] - c2);
      int32 d1 = abs(colors[1][0] - c0) + abs(colors[1][1] - c1) + abs(colors[1][2] - c2);
      int32 d2 = abs(colors[2][0] - c0) + abs(colors[2][1] - c1) + abs(colors[2][2] - c2);
      int32 d3 = abs(colors[3][0] - c0) + abs(colors[3][1] - c1) + abs(colors[3][2] - c2);

      int32 b0 = d0 > d3;
      int32 b1 = d1 > d2;
      int32 b2 = d0 > d2;
      int32 b3 = d1 > d3;
      int32 b4 = d2 > d3;

      int32 x0 = b1 & b2;
      int32 x1 = b0 & b3;
      int32 x2 = b0 & b4;

      result |= (x2 | ((x0 | x1) << 1)) << (i << 1);
    }

    EmitDoubleWord(outBuf, result);
  }

  // Quantize the alpha channel of the pixels in colorBlock to 8 alpha values that are equally
  // spaced along the interval defined by minAlpha and maxAlpha. 48 bits (3 bits per alpha) are
  // written to outBuf, which represent the indices of the 8 alpha values.
  void EmitAlphaIndices(const uint8* colorBlock,  uint8*& outBuf, const uint8 minAlpha, const uint8 maxAlpha)
  {
    assert(maxAlpha >= minAlpha);

    uint8 indices[16];

    uint8 mid = (maxAlpha - minAlpha) / (2 * 7);

    uint8 ab1 = minAlpha + mid;
    uint8 ab2 = (6 * maxAlpha + 1 * minAlpha) / 7 + mid;
    uint8 ab3 = (5 * maxAlpha + 2 * minAlpha) / 7 + mid;
    uint8 ab4 = (4 * maxAlpha + 3 * minAlpha) / 7 + mid;
    uint8 ab5 = (3 * maxAlpha + 4 * minAlpha) / 7 + mid;
    uint8 ab6 = (2 * maxAlpha + 5 * minAlpha) / 7 + mid;
    uint8 ab7 = (1 * maxAlpha + 6 * minAlpha) / 7 + mid;

    colorBlock += 3;

    for(int i = 0; i < 16; i++) {
      uint8 a = colorBlock[i * 4];
      int32 b1 = (a <= ab1);
      int32 b2 = (a <= ab2);
      int32 b3 = (a <= ab3);
      int32 b4 = (a <= ab4);
      int32 b5 = (a <= ab5);
      int32 b6 = (a <= ab6);
      int32 b7 = (a <= ab7);
      int32 index = (b1 + b2 + b3 + b4 + b5 + b6 + b7 + 1) & 7;
      indices[i] = index ^ (2 > index);
    }

    EmitByte(outBuf, (indices[0] >> 0) | (indices[1] << 3) | (indices[2] << 6));
    EmitByte(outBuf, (indices[2] >> 2) | (indices[3] << 1) | (indices[4] << 4) | (indices[ 5] << 7));
    EmitByte(outBuf, (indices[5] >> 1) | (indices[6] << 2) | (indices[7] << 5));
    EmitByte(outBuf, (indices[8] >> 0) | (indices[9] << 3) | (indices[10] << 6));
    EmitByte(outBuf, (indices[10] >> 2) | (indices[11] << 1) | (indices[12] << 4) | (indices[13] << 7));
    EmitByte(outBuf, (indices[13] >> 1) | (indices[14] << 2) | (indices[15] << 5));
  }
}
