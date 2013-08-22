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

#include "ImageLoaderPVR.h"

#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include <iostream>

#include "TexCompTypes.h"

#include "PVRTextureUtilities.h"

static void ReportError(const char *msg) {
  fprintf(stderr, "ERROR: ImageLoaderPVR -- %s\n", msg);
}

ImageLoaderPVR::ImageLoaderPVR(const unsigned char *rawData) 
  : ImageLoader(rawData)
{
}

ImageLoaderPVR::~ImageLoaderPVR() {
}

bool ImageLoaderPVR::ReadData() {
  pvrtexture::CPVRTexture pvrTex((const void *)m_RawData);
  if(!pvrtexture::Transcode(pvrTex,
                            pvrtexture::PVRStandard8PixelType,
                            ePVRTVarTypeUnsignedByte,
                            ePVRTCSpacelRGB)) {
    return false;
  }

  m_RedChannelPrecision = 8;
  m_GreenChannelPrecision = 8;
  m_BlueChannelPrecision = 8;
  m_AlphaChannelPrecision = 8;

  const pvrtexture::CPVRTextureHeader &hdr = pvrTex.getHeader();

  m_Width = hdr.getWidth();
  m_Height = hdr.getHeight();

  const int nPixels = m_Width * m_Height;
  m_RedData = new uint8[nPixels];
  m_GreenData = new uint8[nPixels];
  m_BlueData = new uint8[nPixels];
  m_AlphaData = new uint8[nPixels];

  uint32 *data = (uint32 *)(pvrTex.getDataPtr());
  for (uint32 i = 0; i < m_Width; i++) {
    for (uint32 j = 0; j < m_Height; j++) {
      uint32 idx = j*m_Height + i;
      uint32 pixel = data[idx];
      m_RedData[idx] = pixel & 0xFF;
      m_GreenData[idx] = (pixel >> 8) & 0xFF;
      m_BlueData[idx] = (pixel >> 16) & 0xFF;
      m_AlphaData[idx] = (pixel >> 24) & 0xFF;
    }
  }

  return true;
}

