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

#ifndef PVRTCENCODER_SRC_INDEXER_H_
#define PVRTCENCODER_SRC_INDEXER_H_

#include "FasTC/PVRTCCompressor.h"

#include <algorithm>
#include <cassert>

namespace PVRTCC {

class Indexer {
 private:
  const EWrapMode m_WrapMode;
  const uint32 m_Width;
  const uint32 m_Height;

  uint32 Resolve(int32 i, uint32 limit) const {
    int32 l = static_cast<int32>(limit);

    // Assumptions
    assert(i > -l);
    assert(i < 2*l);

    int32 r = -1;
    switch(m_WrapMode) {
    case eWrapMode_Clamp:
      r = static_cast<uint32>(std::max(0, std::min(i, l-1)));
      break;

    case eWrapMode_Wrap:
      {
        r = i;
        if ((l & (l-1)) == 0) {
          r = (r + l) & (l - 1);
        } else {
          if (r >= l) { r -= l; }
          if (r <  0) { r += l; }
        }
      }
      break;
    }

    assert (r >= 0);
    assert (r < static_cast<int32>(limit));
    return r;
  }

 public:
  Indexer(uint32 width, uint32 height, EWrapMode wrapMode = eWrapMode_Wrap)
    : m_WrapMode(wrapMode)
    , m_Width(width)
    , m_Height(height)
  { }

  uint32 GetWidth() const { return this->m_Width; }
  uint32 GetHeight() const { return this->m_Height; }
  EWrapMode GetWrapMode() const { return this->m_WrapMode; }

  uint32 ResolveX(int32 i) const { return Resolve(i, this->m_Width); }
  uint32 ResolveY(int32 i) const { return Resolve(i, this->m_Height); }

  uint32 operator()(int32 i, int32 j) const {
    uint32 _i = this->ResolveX(i);
    uint32 _j = this->ResolveY(j);

    uint32 index = _j * this->m_Width + _i;
    assert (index < this->m_Width * this->m_Height);
    return index;
  }
};

}  // namespace PVRTCC

#endif  // PVRTCENCODER_SRC_INDEXER_H_
