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

#ifndef _THREAD_GROUP_H_
#define _THREAD_GROUP_H_

#include "FasTC/StopWatch.h"

#include "CompressionFuncs.h"
#include "Thread.h"

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
};

#endif // _THREAD_GROUP_H_
