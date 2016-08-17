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

#include "ImageLoaderPVR.h"

#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include <iostream>

#include "FasTC/TexCompTypes.h"

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
    ReportError("Unable to convert PVRTexture... possibly failed to load file");
    return false;
  }

  const pvrtexture::CPVRTextureHeader &hdr = pvrTex.getHeader();

  m_Width = hdr.getWidth();
  m_Height = hdr.getHeight();

  return LoadFromPixelBuffer(reinterpret_cast<uint32 *>(pvrTex.getDataPtr()));
}

