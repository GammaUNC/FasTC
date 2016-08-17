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

#ifndef PVRTCENCODER_INCLUDE_PVRTCCOMPRESSOR_H_
#define PVRTCENCODER_INCLUDE_PVRTCCOMPRESSOR_H_

#include "FasTC/CompressionJob.h"
#include "FasTC/PVRTCDefines.h"

namespace PVRTCC {

  // PVRTC works by bilinearly interpolating between blocks in order to
  // compress and decompress data. As such, the wrap mode defines how the
  // texture behaves at the boundaries.
  enum EWrapMode {
    eWrapMode_Clamp,  // Block endpoints are clamped at boundaries
    eWrapMode_Wrap,   // Block endpoints wrap around at boundaries
  };

  // Takes a stream of compressed PVRTC data and decompresses it into R8G8B8A8
  // format. The width and height must be specified in order to properly
  // decompress the data.
  void Decompress(const FasTC::DecompressionJob &,
                  const EWrapMode wrapMode = eWrapMode_Wrap,
                  bool bDebugImages = false);

  // Takes a stream of uncompressed RGBA8 data and compresses it into PVRTC
  // version one. The width and height must be specified in order to properly
  // decompress the data.
  void Compress(const FasTC::CompressionJob &,
                const EWrapMode wrapMode = eWrapMode_Wrap);

#ifdef PVRTEXLIB_FOUND
  void CompressPVRLib(const FasTC::CompressionJob &,
                      bool bTwoBitMode = false,
                      const EWrapMode wrapMode = eWrapMode_Wrap);
#endif

  static const uint32 kBlockSize = sizeof(uint64);

}  // namespace PVRTCC

#endif  // PVRTCENCODER_INCLUDE_PVRTCCOMPRESSOR_H_
