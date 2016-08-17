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

#include "FasTC/Color.h"

namespace FasTC {

  uint32 Color::Pack() const {
    uint32 result = 0;
    result |= static_cast<uint32>((A() * 255.0f) + 0.5f);
    result <<= 8;
    result |= static_cast<uint32>((B() * 255.0f) + 0.5f);
    result <<= 8;
    result |= static_cast<uint32>((G() * 255.0f) + 0.5f);
    result <<= 8;
    result |= static_cast<uint32>((R() * 255.0f) + 0.5f);
    return result;
  }

  void Color::Unpack(uint32 rgba) {
    R() = static_cast<float>(rgba & 0xFF) / 255.0f;
    G() = static_cast<float>((rgba >> 8) & 0xFF) / 255.0f;
    B() = static_cast<float>((rgba >> 16) & 0xFF) / 255.0f;
    A() = static_cast<float>((rgba >> 24) & 0xFF) / 255.0f;
  }

  // Tests for equality by comparing the values and the bit depths.
  bool Color::operator==(const Color &other) const {
    static const float kEpsilon = 0.001f;
    for(uint32 c = 0; c < 4; c++) {
      if(fabs(Component(c) - other.Component(c)) > kEpsilon) {
        return false;
      }
    }

    return true;
  }
}  // namespace FasTC
