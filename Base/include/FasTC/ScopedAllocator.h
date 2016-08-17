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

#ifndef BASE_INCLUDE_SCOPEDALLOCATOR_H_
#define BASE_INCLUDE_SCOPEDALLOCATOR_H_

#include "TexCompTypes.h"

namespace FasTC {

  template<typename T>
  class ScopedAllocator {
   private:
    T *m_Ptr;
    ScopedAllocator() : m_Ptr(NULL) { }
   public:
    ScopedAllocator<T>(uint32 nBytes) : m_Ptr(new T[nBytes]) { }
    ~ScopedAllocator() {
      if(m_Ptr) {
        delete [] m_Ptr;
        m_Ptr = NULL;
      }
    }

    T &operator[](uint32 idx) {
      return m_Ptr[idx];
    }

    operator T *() {
      return m_Ptr;
    }

    operator bool() {
      return m_Ptr != NULL;
    }
  };

}  // namespace FasTC

#endif  // BASE_INCLUDE_SCOPEDALLOCATOR_H_
