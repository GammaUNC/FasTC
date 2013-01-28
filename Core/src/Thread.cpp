/* FasTC
 * Copyright (c) 2012 University of North Carolina at Chapel Hill. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its documentation for educational, 
 * research, and non-profit purposes, without fee, and without a written agreement is hereby granted, 
 * provided that the above copyright notice, this paragraph, and the following four paragraphs appear 
 * in all copies.
 *
 * Permission to incorporate this software into commercial products may be obtained by contacting the 
 * authors or the Office of Technology Development at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of North Carolina at Chapel Hill. 
 * The software program and documentation are supplied "as is," without any accompanying services from the 
 * University of North Carolina at Chapel Hill or the authors. The University of North Carolina at Chapel Hill 
 * and the authors do not warrant that the operation of the program will be uninterrupted or error-free. The 
 * end-user understands that the program was developed for research purposes and is advised not to rely 
 * exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE AUTHORS BE LIABLE TO ANY PARTY FOR 
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE 
 * USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE 
 * AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, 
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY 
 * OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
 * ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Please send all BUG REPORTS to <pavel@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Pavel Krajcevski
 * Dept of Computer Science
 * 201 S Columbia St
 * Frederick P. Brooks, Jr. Computer Science Bldg
 * Chapel Hill, NC 27599-3175
 * USA
 * 
 * <http://gamma.cs.unc.edu/FasTC/>
 */

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

  return *this;
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
