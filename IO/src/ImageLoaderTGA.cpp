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

#include "ImageLoaderTGA.h"

#include <algorithm>
#include <cstdio>
#include <cassert>
#include <iostream>

#include "FasTC/TexCompTypes.h"

#include "targa.h"

ImageLoaderTGA::ImageLoaderTGA(const uint8 *rawData, const int32 rawDataSz)
  : ImageLoader(rawData, rawDataSz)
{ }

ImageLoaderTGA::~ImageLoaderTGA() { }

bool ImageLoaderTGA::ReadData() {
  Targa tga;
  if (targa_loadFromData(&tga, m_RawData, m_NumRawDataBytes) < 0) {
    return false;
  }

  m_Width = tga.width;
  m_Height = tga.height;

  assert(static_cast<uint32>(tga.imageLength) == m_Width * m_Height * 4);

  return LoadFromPixelBuffer(reinterpret_cast<uint32 *>(tga.image), true);
}

