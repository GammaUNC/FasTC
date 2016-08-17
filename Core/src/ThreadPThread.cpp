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

#include "Thread.h"

#include <assert.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#ifdef __APPLE__
#include <sched.h>
#endif

static void ReportErrorAndExit(int err, const char *msg) {
  char errMsg[1024];
  sprintf(errMsg, "Thread error -- %s: ", msg);

  errno = err;
  perror(errMsg);
  exit(1);
}

////////////////////////////////////////////////////////////////////////////////
//
// Thread Implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCThreadImpl : public TCThreadBaseImpl {
private:

  static void *RunThread(void *arg) {
    TCThreadImpl *impl = (TCThreadImpl *)arg;
    
    impl->m_Callable();

    return NULL;
  }

  pthread_t m_ThreadID;
  TCCallable &m_Callable;

public:
  TCThreadImpl(TCCallable &callable) :
    m_Callable(callable)
  {
    int result = pthread_create(&m_ThreadID, NULL, RunThread, (void *)this);
    if(result != 0) {
      ReportErrorAndExit(result, "pthread_create");
    }
  }
  virtual ~TCThreadImpl() { }

  void Join() {
    void *status;
    int result = pthread_join(m_ThreadID, &status);
    if(result != 0) {
      ReportErrorAndExit(result, "pthread_join");
    }
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

#ifdef WIN32
#undef Yield
#endif
void TCThread::Yield() {
#if defined(__APPLE__) || defined(__MINGW32__)
  int result = sched_yield();
#else
  int result = pthread_yield();
#endif
  if(result != 0) {
    ReportErrorAndExit(result, "pthread_yield");
  }
}

uint64 TCThread::ThreadID() {
#ifdef __MINGW32__
  return static_cast<uint64>(pthread_self().x);
#elif defined __APPLE__
  return reinterpret_cast<uint64>(pthread_self());
#else
  return static_cast<uint64>(pthread_self());
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
// Mutex Implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCMutexImpl : public TCThreadBaseImpl {
private:
  pthread_mutex_t m_Mutex;

public:
  pthread_mutex_t *GetMutex() { return &m_Mutex; }
  TCMutexImpl() : TCThreadBaseImpl() {
    int result = pthread_mutex_init( &m_Mutex, NULL );
    if(result != 0) {
      ReportErrorAndExit(result, "pthread_mutex_init");
    }
  }

  virtual ~TCMutexImpl() { 
    int result = pthread_mutex_destroy( &m_Mutex );
    if(result != 0) {
      ReportErrorAndExit(result, "pthread_mutex_destroy");
    }    
  }

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
  pthread_mutex_t *const m_MutexPtr;
public:
  TCLockImpl(TCMutex &mutex)
    : m_MutexPtr(((TCMutexImpl *)(mutex.m_Impl))->GetMutex())
  { 
    int result = pthread_mutex_lock( m_MutexPtr );
    if(result != 0) {
      ReportErrorAndExit(result, "pthread_mutex_lock");
    }
  }

  virtual ~TCLockImpl() { 
    int result = pthread_mutex_unlock( m_MutexPtr );
    if(result != 0) {
      ReportErrorAndExit(result, "pthread_mutex_unlock");
    }
  }

  pthread_mutex_t *GetMutexPtr() const { return m_MutexPtr; }
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
  pthread_cond_t m_CV;
public:
  TCConditionVariableImpl() { 
    int result = pthread_cond_init( &m_CV, NULL );
    if(result != 0) {
      ReportErrorAndExit(result, "pthread_cond_init");
    }
  }

  virtual ~TCConditionVariableImpl() { 
    int result = pthread_cond_destroy( &m_CV );
    if(result != 0) {
      ReportErrorAndExit(result, "pthread_cond_destroy");
    }
  }

  void Wait(TCLock &lock) {
    TCLockImpl *lockImpl = (TCLockImpl *)(lock.m_Impl);
    
    int result = pthread_cond_wait( &m_CV, lockImpl->GetMutexPtr() );
    if(result != 0) {
      ReportErrorAndExit( result, "pthread_cond_wait" );
    }
  }

  void NotifyOne() {
    int result = pthread_cond_signal( &m_CV );
    if(result != 0) {
      ReportErrorAndExit( result, "pthread_cond_signal" );
    }
  }

  void NotifyAll() {
    int result = pthread_cond_broadcast( &m_CV );
    if(result != 0) {
      ReportErrorAndExit( result, "pthread_cond_broadcast" );
    }
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
