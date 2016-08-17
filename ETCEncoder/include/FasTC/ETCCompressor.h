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

#ifndef ETCENCODER_INCLUDE_ETCCOMPRESSOR_H_
#define ETCENCODER_INCLUDE_ETCCOMPRESSOR_H_

#include "FasTC/CompressionJob.h"
#include "FasTC/TexCompTypes.h"

namespace ETCC {

  // Takes a stream of compressed ETC1 data and decompresses it into R8G8B8A8
  // format. The width and height must be specified in order to properly
  // decompress the data.
  void Decompress(const FasTC::DecompressionJob &);

  // Takes a stream of uncompressed RGBA8 data and compresses it into ETC1
  // version one. The width and height must be specified in order to properly
  // decompress the data. This uses the library created by Rich Geldreich found here:
  // https://code.google.com/p/rg-etc1
  void Compress_RG(const FasTC::CompressionJob &);

}  // namespace PVRTCC

#endif  // ETCENCODER_INCLUDE_ETCCOMPRESSOR_H_
