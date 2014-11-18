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
