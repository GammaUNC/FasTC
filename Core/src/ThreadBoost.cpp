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

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

////////////////////////////////////////////////////////////////////////////////
//
// Thread Implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCThreadImpl : public TCThreadBaseImpl {
private:
  class Instance {
  private:
    TCCallable &m_Callable;
  public:
    Instance(TCCallable &c) : m_Callable(c) { }

    void operator()() {
      m_Callable();
    }
  };

  boost::thread m_Thread;
public:
  TCThreadImpl(TCCallable &callable)
    : m_Thread(Instance(callable))
  { }
  virtual ~TCThreadImpl() { }

  void Join() {
    m_Thread.join();
  }
};

class TCThreadImplFactory : public TCThreadBaseImplFactory {
  TCCallable &m_Callable;
public:
  TCThreadImplFactory(TCCallable &callable) : m_Callable(callable) { }
  virtual ~TCThreadImplFactory() { }
  virtual TCThreadBaseImpl *CreateImpl() const {
    return new TCThreadImpl(m_Callable);
  }
};

TCThread::TCThread(TCCallable &callable)
  : TCThreadBase(TCThreadImplFactory(callable))
{ }

void TCThread::Join() {
  CheckReferenceCount();
  ((TCThreadImpl *)m_Impl)->Join();
}

void TCThread::Yield() {
  boost::thread::yield();
}

////////////////////////////////////////////////////////////////////////////////
//
// Mutex Implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCMutexImpl : public TCThreadBaseImpl {
private:
  boost::mutex m_Mutex;

public:
  boost::mutex &GetMutex() { return m_Mutex; }
  virtual ~TCMutexImpl() { }
};

class TCMutexImplFactory : public TCThreadBaseImplFactory {
public:
  TCMutexImplFactory() { }
  virtual ~TCMutexImplFactory() { }
  virtual TCThreadBaseImpl *CreateImpl() const {
    return new TCMutexImpl();
  }
};

TCMutex::TCMutex() : TCThreadBase(TCMutexImplFactory())
{ }

////////////////////////////////////////////////////////////////////////////////
//
// Lock Implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCLockImpl : public TCThreadBaseImpl {
private:
  boost::unique_lock<boost::mutex> lock;
public:
  TCLockImpl(TCMutex &mutex)
    : lock(((TCMutexImpl *)(mutex.m_Impl))->GetMutex())
  { }
  virtual ~TCLockImpl() { }

  boost::unique_lock<boost::mutex> &GetLock() { return lock; }
};

class TCLockImplFactory : public TCThreadBaseImplFactory {
private:
  TCMutex &m_Mutex;
public:
  TCLockImplFactory(TCMutex &mutex) : m_Mutex(mutex){ }
  virtual ~TCLockImplFactory() { }
  virtual TCThreadBaseImpl *CreateImpl() const {
    return new TCLockImpl(m_Mutex);
  }
};

TCLock::TCLock(TCMutex &mutex) : TCThreadBase(TCLockImplFactory(mutex))
{ }

////////////////////////////////////////////////////////////////////////////////
//
// Condition Variable Implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCConditionVariableImpl : public TCThreadBaseImpl {
private:
  boost::condition_variable m_CV;
public:
  TCConditionVariableImpl() { }
  virtual ~TCConditionVariableImpl() { }

  void Wait(TCLock &lock) {
    TCLockImpl *lockImpl = (TCLockImpl *)(lock.m_Impl);
    m_CV.wait(lockImpl->GetLock());
  }

  void NotifyOne() {
    m_CV.notify_one();
  }

  void NotifyAll() {
    m_CV.notify_all();
  }
};

class TCConditionVariableImplFactory : public TCThreadBaseImplFactory {
public:
  TCConditionVariableImplFactory() { }
  virtual ~TCConditionVariableImplFactory() { }
  virtual TCThreadBaseImpl *CreateImpl() const {
    return new TCConditionVariableImpl();
  }
};

TCConditionVariable::TCConditionVariable()
  : TCThreadBase(TCConditionVariableImplFactory())
{ }

void TCConditionVariable::Wait(TCLock &lock) {
  CheckReferenceCount();

  TCConditionVariableImpl *impl = (TCConditionVariableImpl *)m_Impl;
  impl->Wait(lock);
}

void TCConditionVariable::NotifyOne() {
  CheckReferenceCount();

  TCConditionVariableImpl *impl = (TCConditionVariableImpl *)m_Impl;
  impl->NotifyOne();
}

void TCConditionVariable::NotifyAll() {
  CheckReferenceCount();

  TCConditionVariableImpl *impl = (TCConditionVariableImpl *)m_Impl;
  impl->NotifyAll();
}
