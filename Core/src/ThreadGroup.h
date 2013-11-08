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

#ifndef _THREAD_GROUP_H_
#define _THREAD_GROUP_H_

#include "CompressionFuncs.h"
#include "Thread.h"
#include "StopWatch.h"

#include <iosfwd>

struct CmpThread : public TCCallable {
  friend class ThreadGroup;  

private:
  uint32 *m_ParentCounter;
  
  TCBarrier *m_StartBarrier;
  TCMutex *m_ParentCounterLock;
  TCConditionVariable *m_FinishCV;

  bool *m_ParentExitFlag;

  FasTC::CompressionJob m_Job;
  CompressionFunc m_CmpFunc;
  CompressionFuncWithStats m_CmpFuncWithStats;
  std::ostream *m_LogStream;

  CmpThread();

public:
  virtual ~CmpThread() { }
  virtual void operator ()();
};

class ThreadGroup {
 public:
  ThreadGroup(
    uint32 numThreads,
    const FasTC::CompressionJob &cj,
    CompressionFunc func
  );

  ThreadGroup(
    uint32 numThreads,
    const FasTC::CompressionJob &cj,
    CompressionFuncWithStats func,
    std::ostream *logStream
  );

  ~ThreadGroup();

  bool PrepareThreads();
  bool Start();
  void Join();
  bool CleanUpThreads();

  const StopWatch &GetStopWatch() const { return m_StopWatch; }

  enum EThreadState {
    eThreadState_Waiting,
    eThreadState_Running,
    eThreadState_Done
  };

 private:
  TCBarrier *const m_StartBarrier;

  TCMutex *const m_FinishMutex;
  TCConditionVariable *const m_FinishCV;

  static const uint32 kMaxNumThreads = 256;
  const int m_NumThreads;

  uint32 m_ActiveThreads;
  uint32 m_ThreadsFinished;

  CmpThread m_Threads[kMaxNumThreads];
  TCThread *m_ThreadHandles[kMaxNumThreads];

  FasTC::CompressionJob m_Job;

  StopWatch m_StopWatch;

  EThreadState m_ThreadState;
  bool m_ExitFlag;

  std::ostream *m_LogStream;
};

#endif // _THREAD_GROUP_H_
