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

#ifndef _IMAGE_WRITER_PNG_H_
#define _IMAGE_WRITER_PNG_H_

#include "FasTC/ImageWriter.h"
#include "FasTC/ImageFwd.h"

// Forward Declare
class ImageWriterPNG : public ImageWriter {
 public:
  ImageWriterPNG(FasTC::Image<> &);
  virtual ~ImageWriterPNG() { }

  virtual bool WriteImage();
 private:
  uint32 m_StreamPosition;
  friend class PNGStreamWriter;
};

#endif // _IMAGE_LOADER_H_
