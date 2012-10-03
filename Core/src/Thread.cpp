#include "Thread.h"

#include <assert.h>

////////////////////////////////////////////////////////////////////////////////
//
// Base Implementation
//
////////////////////////////////////////////////////////////////////////////////

TCThreadBase::TCThreadBase(const TCThreadBaseImplFactory &factory)
  : m_Impl(factory.CreateImpl())
{ }

TCThreadBase::TCThreadBase(const TCThreadBase &other)
  : m_Impl(other.m_Impl)
{
  assert(m_Impl->GetReferenceCount() > 0);
  m_Impl->IncreaseReferenceCount();
}

TCThreadBase &TCThreadBase::operator=(const TCThreadBase &other) {
  assert(m_Impl->GetReferenceCount() > 0);

  // We are no longer referencing this implementation...
  m_Impl->DecreaseReferenceCount();

  // If we're the last ones to reference it, then it should be destroyed.
  if(m_Impl->GetReferenceCount() <= 0) {
    delete m_Impl;
    m_Impl = 0;
  }

  // Our implementation is now the same as the other.
  m_Impl = other.m_Impl;
  m_Impl->IncreaseReferenceCount();
}

TCThreadBase::~TCThreadBase() {

  // We are no longer referencing this implementation...
  m_Impl->DecreaseReferenceCount();

  // If we're the last ones to reference it, then it should be destroyed.
  if(m_Impl->GetReferenceCount() <= 0) {
    assert(m_Impl->GetReferenceCount() == 0);
    delete m_Impl;
  }
}

#ifndef NDEBUG
void TCThreadBase::CheckReferenceCount() {
  assert(m_Impl->GetReferenceCount() > 0);
}
#endif

////////////////////////////////////////////////////////////////////////////////
//
// Barrier Implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCBarrierImpl : public TCThreadBaseImpl {
private:
  unsigned int m_ThreadLimit;
  unsigned int m_ThreadCount;
  unsigned int m_Times;
  
  TCMutex m_Mutex;
  TCConditionVariable m_CV;

public:
  TCBarrierImpl(int threads) 
    : TCThreadBaseImpl()
    , m_ThreadCount(threads)
    , m_ThreadLimit(threads)
    , m_Times(0) 
  { 
    assert(threads > 0);
  }

  virtual ~TCBarrierImpl() { }
  
  bool Wait() {
    TCLock lock(m_Mutex);
    unsigned int times = m_Times;

    if(--m_ThreadCount == 0) {
      m_Times++;
      m_ThreadCount = m_ThreadLimit;
      m_CV.NotifyAll();
      return true;
    }

    while(times == m_Times) {
      m_CV.Wait(lock);
    }
    return false;
  }
};

class TCBarrierImplFactory : public TCThreadBaseImplFactory {
private:
  int m_NumThreads;
public:
  TCBarrierImplFactory(int threads) : TCThreadBaseImplFactory(), m_NumThreads(threads) { }
  virtual ~TCBarrierImplFactory() { }
  virtual TCThreadBaseImpl *CreateImpl() const {
    return new TCBarrierImpl(m_NumThreads);
  }
};

TCBarrier::TCBarrier(int threads) 
  : TCThreadBase(TCBarrierImplFactory(threads)) 
{ }

void TCBarrier::Wait() {
  ((TCBarrierImpl *)m_Impl)->Wait();
}
