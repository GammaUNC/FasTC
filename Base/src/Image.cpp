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

template <typename T>
static inline T sad( const T &a, const T &b ) {
  return (a > b)? a - b : b - a;
}

Image::Image(const Image &other)
  : m_Width(other.m_Width)
  , m_Height(other.m_Height)
  , m_bBlockStreamOrder(other.GetBlockStreamOrder())
  , m_Data(new uint8[m_Width * m_Height * 4])
{
  if(m_Data) {
    memcpy(m_Data, other.m_Data, m_Width * m_Height * 4);
  } else {
    fprintf(stderr, "Out of memory!\n");
  }
}

Image::Image(uint32 width, uint32 height, const uint32 *pixels) 
  : m_Width(width)
  , m_Height(height)
  , m_bBlockStreamOrder(false)
{
  if(pixels) {
    m_Data = new uint8[4 * m_Width * m_Height];
    memcpy(m_Data, pixels, m_Width * m_Height * sizeof(uint32));
  } else {
    m_Data = NULL;
  }
}


Image &Image::operator=(const Image &other) {
  
  m_Width = other.m_Width;
  m_Height = other.m_Height;
  m_bBlockStreamOrder = other.GetBlockStreamOrder();
  
  if(m_Data) {
    delete [] m_Data;
  }
  
  if(other.m_Data) {
    m_Data = new uint8[m_Width * m_Height * 4];
    if(m_Data)
      memcpy(m_Data, other.m_Data, m_Width * m_Height * 4);
    else
      fprintf(stderr, "Out of memory!\n");
  }
  else {
    m_Data = other.m_Data;
  }
  
  return *this;
}

Image::~Image() {
  if(m_Data) {
    delete [] m_Data;
    m_Data = 0;
  }
}

double Image::ComputePSNR(Image *other) {
  if(!other)
    return -1.0;

  if(other->GetWidth() != GetWidth() ||
     other->GetHeight() != GetHeight()) {
    return -1.0;
  }

  // Compute raw 8-bit RGBA data...
  other->ComputeRGBA();
  ComputeRGBA();

  const uint8 *ourData =
    reinterpret_cast<const uint8 *>(GetRGBA());
  const uint8 *otherData =
    reinterpret_cast<const uint8 *>(other->GetRGBA());

  const double wr = 1.0;
  const double wg = 1.0;
  const double wb = 1.0;
    
  double MSE = 0.0;
  const uint32 imageSz = GetWidth() * GetHeight() * 4;
  for(uint32 i = 0; i < imageSz; i+=4) {

    const unsigned char *ourPixel = ourData + i;
    const unsigned char *otherPixel = otherData + i;

    double ourAlphaScale = double(ourPixel[3]) / 255.0;
    double otherAlphaScale = double(otherPixel[3]) / 255.0;
    double dr = double(sad(ourAlphaScale * ourPixel[0],
                           otherAlphaScale * otherPixel[0])) * wr;
    double dg = double(sad(ourAlphaScale * ourPixel[1],
                           otherAlphaScale * otherPixel[1])) * wg;
    double db = double(sad(ourAlphaScale * ourPixel[2],
                           otherAlphaScale * otherPixel[2])) * wb;
    
    const double pixelMSE = 
      (double(dr) * double(dr)) + 
      (double(dg) * double(dg)) + 
      (double(db) * double(db));
    
    //fprintf(stderr, "Pixel MSE: %f\n", pixelMSE);
    MSE += pixelMSE;
  }

  MSE /= (double(GetWidth()) * double(GetHeight()));

  double MAXI = 
    (255.0 * wr) * (255.0 * wr) + 
    (255.0 * wg) * (255.0 * wg) + 
    (255.0 * wb) * (255.0 * wb);

  double PSNR = 10 * log10(MAXI/MSE);
  return PSNR;
}
