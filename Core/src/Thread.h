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

#ifndef __TEX_COMP_THREAD_H__
#define __TEX_COMP_THREAD_H__

#include "FasTC/TexCompTypes.h"

//!HACK! Apparently MSVC has issues with Yield()...????
#ifdef _MSC_VER
#undef Yield
#endif

////////////////////////////////////////////////////////////////////////////////
//
// Base implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCThreadBaseImpl {
  friend class TCThreadBase;
 private:
  int m_ReferenceCount;

  void IncreaseReferenceCount() { m_ReferenceCount++; }
  void DecreaseReferenceCount() { m_ReferenceCount--; }

  int GetReferenceCount() const { return m_ReferenceCount; }
 protected:
  TCThreadBaseImpl()
    : m_ReferenceCount(1)
  { }

  virtual ~TCThreadBaseImpl() { }
};

class TCThreadBaseImplFactory {
 protected:
  TCThreadBaseImplFactory() { }
  virtual ~TCThreadBaseImplFactory() { }
 public:
  virtual TCThreadBaseImpl *CreateImpl() const = 0;
};

class TCThreadBase {
 protected:
  TCThreadBase(const TCThreadBaseImplFactory &);
  TCThreadBase(const TCThreadBase &);
  TCThreadBase &operator=(const TCThreadBase &);
  ~TCThreadBase();

  TCThreadBaseImpl *m_Impl;
#ifndef NDEBUG
  void CheckReferenceCount();
#else
  void CheckReferenceCount() { }
#endif
};

////////////////////////////////////////////////////////////////////////////////
//
// Thread implementation
//
////////////////////////////////////////////////////////////////////////////////

// The base class for a thread implementation
class TCCallable {
 protected:
  TCCallable() { }
 public:
  virtual ~TCCallable() { }
  virtual void operator()() = 0;
};

class TCThread : public TCThreadBase {

 public:
  TCThread(TCCallable &);
  
  static void Yield();
  static uint64 ThreadID();
  void Join();
};

////////////////////////////////////////////////////////////////////////////////
//
// Mutex implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCMutex : public TCThreadBase {
  friend class TCLockImpl;
 public:
  TCMutex();
};

////////////////////////////////////////////////////////////////////////////////
//
// Lock implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCLock : public TCThreadBase {
  friend class TCConditionVariableImpl;
 public:
  TCLock(TCMutex &);
};

////////////////////////////////////////////////////////////////////////////////
//
// Condition Variable implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCConditionVariable : public TCThreadBase {
 public:
  TCConditionVariable();
  void Wait(TCLock &);
  void NotifyOne();
  void NotifyAll();
};

////////////////////////////////////////////////////////////////////////////////
//
// Barrier implementation
//
////////////////////////////////////////////////////////////////////////////////

class TCBarrier : public TCThreadBase {
 public:
  TCBarrier(int threads);
  void Wait();
};

#endif //__TEX_COMP_THREAD_H__
