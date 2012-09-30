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
  m_Impl->DecreaseReferenceCount();
  m_Impl = other.m_Impl;
  m_Impl->IncreaseReferenceCount();
}

TCThreadBase::~TCThreadBase() {
  if(m_Impl->GetReferenceCount() <= 1) {
    assert(m_Impl->GetReferenceCount() >= 0);
    delete m_Impl;
  }
}

#ifndef NDEBUG
void TCThreadBase::CheckReferenceCount() {
  assert(m_Impl->GetReferenceCount() > 0);
}
#endif
