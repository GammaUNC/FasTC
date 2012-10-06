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
