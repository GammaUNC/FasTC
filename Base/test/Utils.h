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

#ifndef PVRTCENCODER_TEST_TESTUTILS_H_
#define PVRTCENCODER_TEST_TESTUTILS_H_

#include "FasTC/TexCompTypes.h"

class PixelPrinter {
 private:
  uint32 m_PixelValue;
 public:
  explicit PixelPrinter(uint32 p) : m_PixelValue(p) { }
  bool operator==(const PixelPrinter &other) const {
    return other.m_PixelValue == this->m_PixelValue;
  }
  uint32 Value() const { return m_PixelValue; }
};

inline ::std::ostream& operator<<(::std::ostream& os, const PixelPrinter& pp) {
  uint32 p = pp.Value();
  uint32 r = p & 0xFF;
  uint32 g = (p >> 8) & 0xFF;
  uint32 b = (p >> 16) & 0xFF;
  uint32 a = (p >> 24) & 0xFF;
  return os <<
    "R: 0x" << ::std::hex << r << " " <<
    "G: 0x" << ::std::hex << g << " " <<
    "B: 0x" << ::std::hex << b << " " <<
    "A: 0x" << ::std::hex << a;
}

#endif  // PVRTCENCODER_TEST_TESTUTILS_H_
