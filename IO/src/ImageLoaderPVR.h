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

#ifndef _IMAGE_LOADER_PVR_H_
#define _IMAGE_LOADER_PVR_H_

#include "FasTC/ImageLoader.h"

class ImageLoaderPVR : public ImageLoader {
 public:
  ImageLoaderPVR(const unsigned char *rawData);
  virtual ~ImageLoaderPVR();

  virtual bool ReadData();
};

#endif // _IMAGE_LOADER_PVR_H_
