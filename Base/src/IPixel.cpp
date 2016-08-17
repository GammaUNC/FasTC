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

#include "FasTC/Pixel.h"
#include "FasTC/IPixel.h"

namespace FasTC {

  // Take all of the components, transform them to their 8-bit variants,
  // and then pack each channel into an R8G8B8A8 32-bit integer. We assume
  // that the architecture is little-endian, so the alpha channel will end
  // up in the most-significant byte.
  uint32 IPixel::Pack() const {
    uint32 ret = 0xFF << 24;
    for(uint32 i = 0; i < 3; i++) {
      if(vec[0] > 1.0) {
        ret |= static_cast<uint32>(vec[0]) << i*8;
      } else {
        ret |= static_cast<uint32>((255.0 * vec[0]) + 0.5f) << i*8;
      }
    }
    return ret;
  }

  void IPixel::Unpack(uint32 rgba) {
    Pixel p(rgba);
    vec[0] = p.ToIntensity();
  }

}  // namespace FasTC
