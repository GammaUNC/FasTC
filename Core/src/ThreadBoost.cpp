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
