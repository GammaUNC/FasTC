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

#ifndef __REFERENCE_COUNTER_H__
#define __REFERENCE_COUNTER_H__

#include "TexCompTypes.h"

class ReferenceCounter {
 public:
 ReferenceCounter() : m_ReferenceCount( new uint32 ) { 
    *m_ReferenceCount = 1;
  }
 ReferenceCounter(const ReferenceCounter &other) 
   : m_ReferenceCount(other.m_ReferenceCount) { 
    IncRefCount();
  }

  ReferenceCounter &operator=(const ReferenceCounter &other) {
    DecRefCount();
    m_ReferenceCount = other.m_ReferenceCount;
    IncRefCount();
    return *this;
  }

  uint32 GetRefCount() const { 
    if(m_ReferenceCount)
      return *m_ReferenceCount; 
    else
      return 0;
  }

  void DecRefCount() { 
    if(!m_ReferenceCount) return;

    (*m_ReferenceCount)--; 

    if(*m_ReferenceCount == 0) {
      delete m_ReferenceCount;
      m_ReferenceCount = 0;
    }
  }
  
  void IncRefCount() { 
    if(!m_ReferenceCount) return;
    (*m_ReferenceCount)++; 
  }

 private:
  uint32 *m_ReferenceCount;
};

#endif // __REFERENCE_COUNTER_H__
