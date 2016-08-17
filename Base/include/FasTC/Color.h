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

#ifndef BASE_INCLUDE_COLOR_H_
#define BASE_INCLUDE_COLOR_H_

#include "TexCompTypes.h"
#include "Vector4.h"

namespace FasTC {

class Color : public Vec4f {
 public:
  Color(float r, float g, float b, float a) : Vec4f(a, r, g, b) { }
  Color() : Vec4f(0, 0, 0, 0) { }

  // Let's allow us to use the operators...
  template<typename T>
  Color &operator=(const Vector4<T> &other) {
    Vec4f::operator=(other);
    return *this;
  }

  template<typename T>
  Color(const Vector4<T> &other) : Vec4f(other) { }
  
  const float &A() const { return vec[0]; }
  float &A() { return vec[0]; }
  const float &R() const { return vec[1]; }
  float &R() { return vec[1]; }
  const float &G() const { return vec[2]; }
  float &G() { return vec[2]; }
  const float &B() const { return vec[3]; }
  float &B() { return vec[3]; }
  const float &Component(uint32 idx) const { return vec[idx]; }
  float &Component(uint32 idx) { return vec[idx]; }

  // Take all of the components, transform them to their 8-bit variants,
  // and then pack each channel into an R8G8B8A8 32-bit integer. We assume
  // that the architecture is little-endian, so the alpha channel will end
  // up in the most-significant byte.
  uint32 Pack() const;
  void Unpack(uint32 rgba);

  // Tests for equality by comparing the values and the bit depths.
  bool operator==(const Color &) const;

  void MakeOpaque() { A() = 1.f ; }
};
REGISTER_VECTOR_TYPE(Color);

}  // namespace FasTC

#endif  // BASE_INCLUDE_COLOR_H_
