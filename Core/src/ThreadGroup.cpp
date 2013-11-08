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

#include "ThreadGroup.h"
#include "BC7Compressor.h"

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <iostream>

using FasTC::CompressionJob;

CmpThread::CmpThread() 
  : m_ParentCounter(NULL)
  , m_StartBarrier(NULL)
  , m_ParentCounterLock(NULL)
  , m_FinishCV(NULL)
  , m_ParentExitFlag(NULL)
  , m_Job(CompressionJob(FasTC::kNumCompressionFormats, NULL, NULL, 0, 0))
  , m_CmpFunc(NULL)
  , m_CmpFuncWithStats(NULL)
  , m_LogStream(NULL)
{ }

void CmpThread::operator()() {
  if(!m_Job.OutBuf() || !m_Job.InBuf() 
     || !m_ParentCounter || !m_ParentCounterLock || !m_FinishCV
     || !m_StartBarrier
     || !m_ParentExitFlag
  ) {
    fprintf(stderr, "Incorrect thread initialization.\n");
    return;
  }

  if(!(m_CmpFunc || (m_CmpFuncWithStats && m_LogStream))) {
    fprintf(stderr, "Incorrect thread function pointer.\n");
    return;
  }

  while(1) {
    // Wait for signal to start work...
    m_StartBarrier->Wait();

    if(*m_ParentExitFlag) {
      return;
    }

    if(m_CmpFunc)
      (*m_CmpFunc)(m_Job);
    else
      (*m_CmpFuncWithStats)(m_Job, m_LogStream);

    {
      TCLock lock(*m_ParentCounterLock);
      (*m_ParentCounter)++;
    }

    m_FinishCV->NotifyOne();
  }
}

ThreadGroup::ThreadGroup(uint32 numThreads,
                         const CompressionJob &job,
                         CompressionFunc func)
  : m_StartBarrier(new TCBarrier(numThreads + 1))
  , m_FinishMutex(new TCMutex())
  , m_FinishCV(new TCConditionVariable())
  , m_NumThreads(numThreads)
  , m_ActiveThreads(0)
  , m_Job(job)
  , m_ThreadState(eThreadState_Done)
  , m_ExitFlag(false)
{ 
  for(uint32 i = 0; i < kMaxNumThreads; i++) {
    // Thread synchronization primitives
    m_Threads[i].m_ParentCounterLock = m_FinishMutex;
    m_Threads[i].m_FinishCV = m_FinishCV;
    m_Threads[i].m_ParentCounter = &m_ThreadsFinished;
    m_Threads[i].m_StartBarrier = m_StartBarrier;
    m_Threads[i].m_ParentExitFlag = &m_ExitFlag;
    m_Threads[i].m_CmpFunc = func;
  }
}

ThreadGroup::ThreadGroup( 
  uint32 numThreads, 
  const CompressionJob &job,
  CompressionFuncWithStats func, 
  std::ostream *logStream
)
  : m_StartBarrier(new TCBarrier(numThreads + 1))
  , m_FinishMutex(new TCMutex())
  , m_FinishCV(new TCConditionVariable())
  , m_NumThreads(numThreads)
  , m_ActiveThreads(0)
  , m_Job(job)
  , m_ThreadState(eThreadState_Done)
  , m_ExitFlag(false)
{ 
  for(uint32 i = 0; i < kMaxNumThreads; i++) {
    // Thread synchronization primitives
    m_Threads[i].m_ParentCounterLock = m_FinishMutex;
    m_Threads[i].m_FinishCV = m_FinishCV;
    m_Threads[i].m_ParentCounter = &m_ThreadsFinished;
    m_Threads[i].m_StartBarrier = m_StartBarrier;
    m_Threads[i].m_ParentExitFlag = &m_ExitFlag;
    m_Threads[i].m_CmpFuncWithStats = func;
    m_Threads[i].m_LogStream = logStream;
  }
}

ThreadGroup::~ThreadGroup() {
  delete m_StartBarrier;
  delete m_FinishMutex;
  delete m_FinishCV;
}

bool ThreadGroup::PrepareThreads() {

  // Make sure that threads aren't running.
  if(m_ThreadState != eThreadState_Done) {
    return false;
  }

  // Have we already activated the thread group?
  if(m_ActiveThreads > 0) {
    m_ThreadState = eThreadState_Waiting;
    return true;
  }

  // We can assume that the image data is in block stream order
  // so, the size of the data given to each thread will be (nb*4)x4
  uint32 blockDim[2];
  GetBlockDimensions(m_Job.Format(), blockDim);
  uint32 numBlocks = (m_Job.Width() * m_Job.Height()) / (blockDim[0] * blockDim[1]);
  uint32 blocksProcessed = 0;
  uint32 blocksPerThread = (numBlocks/m_NumThreads) + ((numBlocks % m_NumThreads)? 1 : 0);

  // Currently no threads are finished...
  m_ThreadsFinished = 0;
  for(int i = 0; i < m_NumThreads; i++) {

    if(m_ActiveThreads >= kMaxNumThreads)
      break;

    int numBlocksThisThread = blocksPerThread;
    if(blocksProcessed + numBlocksThisThread > numBlocks) {
      numBlocksThisThread = numBlocks - blocksProcessed;
    }

    uint32 start[2], end[2];
    m_Job.BlockIdxToCoords(blocksProcessed, start);
    m_Job.BlockIdxToCoords(blocksProcessed + numBlocksThisThread, end);

    // !TODO! This should be moved to a unit test...
    assert(m_Job.CoordsToBlockIdx(start[0], start[1]) == blocksProcessed);
    assert(m_Job.CoordsToBlockIdx(end[0], end[1]) == blocksProcessed + numBlocksThisThread);

    CompressionJob cj(m_Job.Format(),
                      m_Job.InBuf(), m_Job.OutBuf(),
                      m_Job.Width(), m_Job.Height(),
                      start[0], start[1],
                      end[0], end[1]);

    CmpThread &t = m_Threads[m_ActiveThreads];
    t.m_Job = cj;

    blocksProcessed += numBlocksThisThread;
    
    m_ThreadHandles[m_ActiveThreads] = new TCThread(t);

    m_ActiveThreads++;
  }

  m_ThreadState = eThreadState_Waiting;
  return true;
}

bool ThreadGroup::Start() {

  if(m_ActiveThreads <= 0) {
    return false;
  }

  if(m_ThreadState != eThreadState_Waiting) {
    return false;
  }

  m_StopWatch.Reset();
  m_StopWatch.Start();

  // Last thread to activate the barrier is this one.
  m_ThreadState = eThreadState_Running;
  m_StartBarrier->Wait();

  return true;
}

bool ThreadGroup::CleanUpThreads() {

  // Are the threads currently running?
  if(m_ThreadState == eThreadState_Running) {
    // ... if so, wait for them to finish
    Join();
  }

  assert(m_ThreadState == eThreadState_Done || m_ThreadState == eThreadState_Waiting);
  
  // Mark all threads for exit
  m_ExitFlag = true;

  // Hit the barrier to signal them to go.
  m_StartBarrier->Wait();

  // Clean up.
  for(uint32 i = 0; i < m_ActiveThreads; i++) {
    m_ThreadHandles[i]->Join();
    delete m_ThreadHandles[i];
  }

  // Reset active number of threads...
  m_ActiveThreads = 0;
  m_ExitFlag = false;

  return true;
}

void ThreadGroup::Join() {

  TCLock lock(*m_FinishMutex);
  while(m_ThreadsFinished != m_ActiveThreads) {
    m_FinishCV->Wait(lock);
  }

  m_StopWatch.Stop();
  m_ThreadState = eThreadState_Done;
  m_ThreadsFinished = 0;
}
