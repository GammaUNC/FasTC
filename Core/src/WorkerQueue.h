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

#ifndef __TEXCOMP_WORKDER_QUEUE_H__
#define __TEXCOMP_WORKDER_QUEUE_H__

// Forward declare...
class WorkerQueue;

// Necessary includes...
#include "FasTC/TexCompTypes.h"
#include "FasTC/StopWatch.h"

#include "Thread.h"
#include "CompressionFuncs.h"

#include <iosfwd>

class WorkerThread : public TCCallable {
  friend class WorkerQueue;
public:

  WorkerThread(WorkerQueue *, uint32 idx);
  virtual ~WorkerThread() { }
  virtual void operator ()();

  enum EAction {
    eAction_Wait,
    eAction_DoWork,
    eAction_Quit,

    kNumWorkerThreadActions
  };

private:
  uint32 m_ThreadIdx;
  WorkerQueue *const m_Parent;
};

class WorkerQueue {
  friend class WorkerThread;
 public:
  WorkerQueue(
    uint32 numCompressions,
    uint32 numThreads,
    uint32 jobSize,
    const FasTC::CompressionJob &job,
    CompressionFunc func
  );

  WorkerQueue(
    uint32 numCompressions,
    uint32 numThreads, 
    uint32 jobSize,
    const FasTC::CompressionJob &job,
    CompressionFuncWithStats func, 
    std::ostream *logStream
  );

  ~WorkerQueue() { }

  // Runs the workers
  void Run();
  const StopWatch &GetStopWatch() const { return m_StopWatch; }

 private:
  uint32 m_NumCompressions;
  const uint32 m_TotalNumCompressions;
  uint32 m_NumThreads;
  uint32 m_WaitingThreads;
  uint32 m_ActiveThreads;
  uint32 m_JobSize;
  FasTC::CompressionJob m_Job;

  TCConditionVariable m_CV;
  TCMutex m_Mutex;

  uint32 m_NextBlock;

  static const int kMaxNumWorkerThreads = 256;
  uint32 m_Offsets[kMaxNumWorkerThreads];
  uint32 m_NumBlocks[kMaxNumWorkerThreads];

  WorkerThread *m_Workers[kMaxNumWorkerThreads];
  TCThread *m_ThreadHandles[kMaxNumWorkerThreads];

  const FasTC::CompressionJob &GetCompressionJob() const { return m_Job; }
  void GetStartForThread(const uint32 threadIdx, uint32 (&start)[2]);
  void GetEndForThread(const uint32 threadIdx, uint32 (&start)[2]);
  
  const CompressionFunc m_CompressionFunc;
  CompressionFunc GetCompressionFunc() const { return m_CompressionFunc; }

  const CompressionFuncWithStats m_CompressionFuncWithStats;
  CompressionFuncWithStats GetCompressionFuncWithStats() const { return m_CompressionFuncWithStats; }

  std::ostream *m_LogStream;
  std::ostream *GetLogStream() const { return m_LogStream; }

  StopWatch m_StopWatch;

  WorkerThread::EAction AcceptThreadData(uint32 threadIdx);
  void NotifyWorkerFinished();
};

#endif //__TEXCOMP_WORKDER_QUEUE_H__
