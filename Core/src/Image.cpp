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
#include "ImageLoader.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

template <typename T>
static inline T sad( const T &a, const T &b ) {
  return (a > b)? a - b : b - a;
}

Image::Image(const Image &other)
: m_Width(other.m_Width)
, m_Height(other.m_Height)
, m_PixelData(new uint8[m_Width * m_Height * 4])
{
  if(m_PixelData) {
    memcpy(m_PixelData, other.m_PixelData, m_Width * m_Height * 4);
  }
  else {
    fprintf(stderr, "Out of memory!\n");
  }
}

Image &Image::operator=(const Image &other) {
  
  m_Width = other.m_Width;
  m_Height = other.m_Height;
  
  if(m_PixelData) {
    delete [] m_PixelData;
  }
  
  if(other.m_PixelData) {
    m_PixelData = new uint8[m_Width * m_Height * 4];
    if(m_PixelData)
      memcpy(m_PixelData, other.m_PixelData, m_Width * m_Height * 4);
    else
      fprintf(stderr, "Out of memory!\n");
  }
  else {
    m_PixelData = other.m_PixelData;
  }
  
  return *this;
}

Image::Image(const CompressedImage &ci)
  : m_Width(ci.GetWidth())
  , m_Height(ci.GetHeight())
{
	unsigned int bufSz = ci.GetWidth() * ci.GetHeight() * 4;
	m_PixelData = new uint8[ bufSz ];
	if(!m_PixelData) { fprintf(stderr, "%s\n", "Out of memory!"); return; }

	if(!ci.DecompressImage(m_PixelData, bufSz)) {
		fprintf(stderr, "Error decompressing image!\n");
		return;
	}
}

Image::Image(const ImageLoader &loader) 
  : m_Width(loader.GetWidth())
  , m_Height(loader.GetHeight())
  , m_PixelData(0)
{
  if(loader.GetImageData()) {
    m_PixelData = new uint8[ loader.GetImageDataSz() ];
    if(!m_PixelData) { fprintf(stderr, "%s\n", "Out of memory!"); return; }
    memcpy(m_PixelData, loader.GetImageData(), loader.GetImageDataSz());
  }
  else {
    fprintf(stderr, "%s\n", "Failed to get data from image loader!");
  }
}

Image::~Image() {
  if(m_PixelData) {
    delete [] m_PixelData;
    m_PixelData = 0;
  }
}

CompressedImage *Image::Compress(const SCompressionSettings &settings) const {
  CompressedImage *outImg = NULL;
  const unsigned int dataSz = GetWidth() * GetHeight() * 4;

  assert(dataSz > 0);

  // Allocate data based on the compression method
  int cmpDataSz = 0;
  switch(settings.format) {
    default: assert(!"Not implemented!"); // Fall Through V
    case eCompressionFormat_DXT1: cmpDataSz = dataSz / 8; break;
    case eCompressionFormat_DXT5: cmpDataSz = dataSz / 4; break;
    case eCompressionFormat_BPTC: cmpDataSz = dataSz / 4; break;
  }

  unsigned char *cmpData = new unsigned char[cmpDataSz];
  CompressImageData(m_PixelData, dataSz, cmpData, cmpDataSz, settings);

  outImg = new CompressedImage(GetWidth(), GetHeight(), settings.format, cmpData);
  
  delete [] cmpData;
  return outImg;
}

double Image::ComputePSNR(const CompressedImage &ci) const {
  unsigned int imageSz = 4 * GetWidth() * GetHeight();
  unsigned char *unCompData = new unsigned char[imageSz];
  if(!(ci.DecompressImage(unCompData, imageSz))) {
    fprintf(stderr, "%s\n", "Failed to decompress image.");
    delete [] unCompData;
    return -1.0f;
  }

  const double wr = 1.0;
  const double wg = 1.0;
  const double wb = 1.0;
    
  double MSE = 0.0;
  for(uint32 i = 0; i < imageSz; i+=4) {

    const unsigned char *pixelDataRaw = m_PixelData + i;
    const unsigned char *pixelDataUncomp = unCompData + i;

    double rawAlphaScale = double(pixelDataRaw[3]) / 255.0;
    double uncompAlphaScale = double(pixelDataUncomp[3]) / 255.0;
    double dr = double(sad(rawAlphaScale * pixelDataRaw[0], uncompAlphaScale * pixelDataUncomp[0])) * wr;
    double dg = double(sad(rawAlphaScale * pixelDataRaw[1], uncompAlphaScale * pixelDataUncomp[1])) * wg;
    double db = double(sad(rawAlphaScale * pixelDataRaw[2], uncompAlphaScale * pixelDataUncomp[2])) * wb;
    
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

  // Cleanup
  delete unCompData;
  return PSNR;
}
