#ifndef __TEX_COMP_THREAD_H__
#define __TEX_COMP_THREAD_H__

#include "TexCompTypes.h"

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

  void Join();
  static void Yield();
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
