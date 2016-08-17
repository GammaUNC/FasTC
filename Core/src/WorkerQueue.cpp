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

#include "WorkerQueue.h"

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <iostream>

#include "FasTC/BPTCCompressor.h"

using FasTC::CompressionJob;

template <typename T>
static inline void clamp(T &x, const T &min, const T &max) {
  if(x < min) x = min;
  else if(x > max) x = max;
}

WorkerThread::WorkerThread(WorkerQueue * parent, uint32 idx) 
  : TCCallable()
  , m_ThreadIdx(idx)
  , m_Parent(parent)
{ }

void WorkerThread::operator()() {

  if(!m_Parent) {
    fprintf(stderr, "%s\n", "Illegal worker thread initialization -- parent is NULL.");
    return;
  }

  CompressionFunc f = m_Parent->GetCompressionFunc();
  CompressionFuncWithStats fStat = m_Parent->GetCompressionFuncWithStats();
  std::ostream *logStream = m_Parent->GetLogStream();

  if(!(f || (fStat && logStream))) {
    fprintf(stderr, "%s\n", "Illegal worker queue initialization -- compression func is NULL.");
    return;
  }

  bool quitFlag = false;
  while(!quitFlag) {
    
    switch(m_Parent->AcceptThreadData(m_ThreadIdx)) {

      case eAction_Quit:
      {
        quitFlag = true;
        break;
      }

      case eAction_Wait:
      {
        TCThread::Yield();
        break;
      }

      case eAction_DoWork:
      {
        const CompressionJob &job = m_Parent->GetCompressionJob();

        uint32 start[2];
        m_Parent->GetStartForThread(m_ThreadIdx, start);

        uint32 end[2];
        m_Parent->GetEndForThread(m_ThreadIdx, end);

        CompressionJob cj (job.Format(),
                           job.InBuf(), job.OutBuf(),
                           job.Width(), job.Height(),
                           start[0], start[1],
                           end[0], end[1]);
        if(f)
          (*f)(cj);
        else
          (*fStat)(cj, logStream);

        break;
      }

      default:
      {
        fprintf(stderr, "Unrecognized thread command!\n");
        quitFlag = true;
        break;
      }
    }
  }

  m_Parent->NotifyWorkerFinished();

  return;
}

WorkerQueue::WorkerQueue(
  uint32 numCompressions,
  uint32 numThreads,
  uint32 jobSize,
  const CompressionJob &job,
  CompressionFunc func
)
  : m_NumCompressions(0)
  , m_TotalNumCompressions(std::max(uint32(1), numCompressions))
  , m_NumThreads(numThreads)
  , m_WaitingThreads(0)
  , m_ActiveThreads(0)
  , m_JobSize(std::max(uint32(1), jobSize))
  , m_Job(job)
  , m_NextBlock(0)
  , m_CompressionFunc(func)
  , m_CompressionFuncWithStats(NULL)
  , m_LogStream(NULL)
{
  clamp(m_NumThreads, uint32(1), uint32(kMaxNumWorkerThreads));
}

WorkerQueue::WorkerQueue(
  uint32 numCompressions,
  uint32 numThreads, 
  uint32 jobSize,
  const CompressionJob &job,
  CompressionFuncWithStats func, 
  std::ostream *logStream
)
  : m_NumCompressions(0)
  , m_TotalNumCompressions(std::max(uint32(1), numCompressions))
  , m_NumThreads(numThreads)
  , m_WaitingThreads(0)
  , m_ActiveThreads(0)
  , m_JobSize(std::max(uint32(1), jobSize))
  , m_Job(job)
  , m_NextBlock(0)
  , m_CompressionFunc(NULL)
  , m_CompressionFuncWithStats(func)
  , m_LogStream(logStream)
{
  clamp(m_NumThreads, uint32(1), uint32(kMaxNumWorkerThreads));
}

void WorkerQueue::Run() {
  
  // Spawn a bunch of threads...
  TCLock lock(m_Mutex);
  for(uint32 i = 0; i < m_NumThreads; i++) {
    m_Workers[i] = new WorkerThread(this, i);
    m_ThreadHandles[m_ActiveThreads] = new TCThread(*m_Workers[i]);
    m_ActiveThreads++;
  }

  m_StopWatch.Reset();
  m_StopWatch.Start();

  m_NextBlock = 0;
  m_WaitingThreads = 0;

  // Wait for them to finish...
  while(m_ActiveThreads > 0) {
    m_CV.Wait(lock);
  }

  m_StopWatch.Stop();

  // Join them all together..
  for(uint32 i = 0; i < m_NumThreads; i++) {
    m_ThreadHandles[i]->Join();
    delete m_ThreadHandles[i];
    delete m_Workers[i];
  }
}

void WorkerQueue::NotifyWorkerFinished() {
  {
    TCLock lock(m_Mutex);
    m_ActiveThreads--;
  }
  m_CV.NotifyOne();
}

WorkerThread::EAction WorkerQueue::AcceptThreadData(uint32 threadIdx) {
  if(threadIdx >= m_ActiveThreads) {
    return WorkerThread::eAction_Quit;
  }

  // How many blocks total do we have?
  uint32 blockDim[2];
  GetBlockDimensions(m_Job.Format(), blockDim);
  const uint32 totalBlocks = (m_Job.Width() * m_Job.Height()) / (blockDim[0] * blockDim[1]);
  
  // Make sure we have exclusive access...
  TCLock lock(m_Mutex);
  
  // If we've completed all blocks, then mark the thread for 
  // completion.
  if(m_NextBlock == totalBlocks) {
    if(m_NumCompressions < m_TotalNumCompressions) {
      if(++m_WaitingThreads == m_ActiveThreads) {
        m_NextBlock = 0;
        m_WaitingThreads = 0;
      } else {
        return WorkerThread::eAction_Wait;
      }
    }
    else {
      return WorkerThread::eAction_Quit;
    }
  }

  // Otherwise, this thread's offset is the current block...
  m_Offsets[threadIdx] = m_NextBlock;
  
  // The number of blocks to process is either the job size
  // or the number of blocks remaining.
  int blocksProcessed = std::min(m_JobSize, totalBlocks - m_NextBlock);
  m_NumBlocks[threadIdx] = blocksProcessed;

  // Make sure the next block is updated.
  m_NextBlock += blocksProcessed;

  if(m_NextBlock == totalBlocks) {
    ++m_NumCompressions;
  }

  return WorkerThread::eAction_DoWork;
}

void WorkerQueue::GetStartForThread(const uint32 threadIdx, uint32 (&start)[2]) {
  assert(threadIdx >= 0);
  assert(threadIdx < m_NumThreads);
  assert(m_Offsets[threadIdx] >= 0);

  const uint32 blockIdx = m_Offsets[threadIdx];
  m_Job.BlockIdxToCoords(blockIdx, start);
}

void WorkerQueue::GetEndForThread(const uint32 threadIdx, uint32 (&end)[2]) {
  assert(threadIdx >= 0);
  assert(threadIdx < m_NumThreads);
  assert(m_Offsets[threadIdx] >= 0);
  assert(m_NumBlocks[threadIdx] >= 0);

  const uint32 blockIdx = m_Offsets[threadIdx] + m_NumBlocks[threadIdx];
  m_Job.BlockIdxToCoords(blockIdx, end);
}
