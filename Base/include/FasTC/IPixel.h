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

#ifndef BASE_INCLUDE_IPIXEL_H_
#define BASE_INCLUDE_IPIXEL_H_

#include "TexCompTypes.h"
#include "VectorBase.h"

namespace FasTC {

class IPixel : public VectorBase<float, 1> {
 public:
  IPixel() : VectorBase<float, 1>() { vec[0] = 0.0f; }
  IPixel(float f) : VectorBase<float, 1>(&f) { }

  operator float() const {
    return vec[0];
  }

  IPixel operator=(const float &f) {
    return vec[0] = f;
  }

  // Take all of the components, transform them to their 8-bit variants,
  // and then pack each channel into an R8G8B8A8 32-bit integer. We assume
  // that the architecture is little-endian, so the alpha channel will end
  // up in the most-significant byte.
  uint32 Pack() const;
  void Unpack(uint32 rgba);

  void MakeOpaque() { /* Do nothing.. */ }

  bool operator==(const IPixel &other) const {
    return static_cast<float>(*this) == static_cast<float>(other);
  }

  bool operator!=(const IPixel &other) const {
    return static_cast<float>(*this) != static_cast<float>(other);
  }
};
REGISTER_VECTOR_TYPE(IPixel);

}  // namespace FasTC

#endif  // BASE_INCLUDE_PIXEL_H_
