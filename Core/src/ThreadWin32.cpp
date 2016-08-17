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

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

void ErrorHandler(LPTSTR lpszFunction) 
{ 
  // Retrieve the system error message for the last-error code.
  LPVOID lpMsgBuf;
  LPVOID lpDisplayBuf;
  DWORD dw = GetLastError(); 

  FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    dw,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPTSTR) &lpMsgBuf,
    0, NULL );

  // Display the error message.
  lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
  (lstrlen((LPCTSTR) lpMsgBuf) + lstrlen((LPCTSTR) lpszFunction) + 40) * sizeof(TCHAR)); 
  StringCchPrintf((LPTSTR)lpDisplayBuf, 
    LocalSize(lpDisplayBuf) / sizeof(TCHAR),
    TEXT("%s failed with error %d: %s"), 
    lpszFunction, dw, lpMsgBuf); 
  MessageBox(NULL, (LPCTSTR) lpDisplayBuf, TEXT("Error"), MB_OK); 

  // Free error-handling buffer allocations.
  LocalFree(lpMsgBuf);
  LocalFree(lpDisplayBuf);
}

////////////////////////////////////////////////////////////////////////////////
//
// Thread Implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCThreadImpl : public TCThreadBaseImpl {
private:
  static DWORD WINAPI RunThread(LPVOID arg) {
    TCThreadImpl *impl = (TCThreadImpl *)arg;
    impl->m_Callable();
    return 0;
  }

  HANDLE m_ThreadID;
  TCCallable &m_Callable;

public:
  TCThreadImpl(TCCallable &callable) :
    m_Callable(callable)
  {
    m_ThreadID = CreateThread(NULL, 0, RunThread, (void *)this, 0, 0);
    if(m_ThreadID == NULL) {
      ErrorHandler("CreateThread");
    }
  }
  virtual ~TCThreadImpl() { }

  void Join() {
    DWORD result = WaitForSingleObject(m_ThreadID, INFINITE);
    if(result == WAIT_FAILED) {
      ErrorHandler("WaitForSingleObject");
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

// !HACK! wtf, Microsoft?
#undef Yield
void TCThread::Yield() {
  if(!SwitchToThread()) {
    ErrorHandler("SwitchToThread");
  }
}

uint64 TCThread::ThreadID() {
  return static_cast<uint64>(GetCurrentThreadId());
}

////////////////////////////////////////////////////////////////////////////////
//
// Mutex Implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCMutexImpl : public TCThreadBaseImpl {
private:
  CRITICAL_SECTION m_CS;

public:
  LPCRITICAL_SECTION GetMutex() { return &m_CS; }
  TCMutexImpl() : TCThreadBaseImpl() {
    InitializeCriticalSection( &m_CS );
  }

  virtual ~TCMutexImpl() { 
    DeleteCriticalSection( &m_CS );
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
  LPCRITICAL_SECTION const m_CSPtr;
  // Disallow copy and assign...
  TCLockImpl(const TCLockImpl &) : m_CSPtr(NULL) { }
  TCLockImpl &operator =(const TCLockImpl &) { return *this; }
public:
  TCLockImpl(TCMutex &mutex)
    : m_CSPtr(((TCMutexImpl *)(mutex.m_Impl))->GetMutex()) { 
    EnterCriticalSection( m_CSPtr );
  }

  virtual ~TCLockImpl() { LeaveCriticalSection( m_CSPtr ); }
  LPCRITICAL_SECTION GetMutexPtr() const { return m_CSPtr; }
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
  CONDITION_VARIABLE m_CV;
public:
  TCConditionVariableImpl() { InitializeConditionVariable( &m_CV ); }
  virtual ~TCConditionVariableImpl() { 
    // No destroy condition variable...?
  }

  void Wait(TCLock &lock) {
    TCLockImpl *lockImpl = (TCLockImpl *)(lock.m_Impl);
    
    BOOL result = SleepConditionVariableCS( &m_CV, lockImpl->GetMutexPtr(), INFINITE );
    if(!result) {
      ErrorHandler( "SleepConditionVariableCS" );
    }
  }

  void NotifyOne() { WakeConditionVariable( &m_CV ); }
  void NotifyAll() { WakeAllConditionVariable( &m_CV ); }
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
