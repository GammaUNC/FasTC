#include "ThreadGroup.h"
#include "BC7Compressor.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

CmpThread::CmpThread() 
  : m_StartBarrier(NULL)
  , m_ParentCounter(NULL)
  , m_ParentCounterLock(NULL)
  , m_FinishCV(NULL)
  , m_Width(0)
  , m_Height(0)
  , m_CmpFunc(NULL)
  , m_OutBuf(NULL)
  , m_InBuf(NULL)
  , m_ParentExitFlag(NULL)
{ }

void CmpThread::operator()() {
  if(!m_CmpFunc || !m_OutBuf || !m_InBuf 
     || !m_ParentCounter || !m_ParentCounterLock || !m_FinishCV
     || !m_StartBarrier
     || !m_ParentExitFlag
  ) {
    fprintf(stderr, "Incorrect thread initialization.\n");
    return;
  }

  while(1) {
    // Wait for signal to start work...
    m_StartBarrier->Wait();

    if(*m_ParentExitFlag) {
      return;
    }

    (*m_CmpFunc)(m_InBuf, m_OutBuf, m_Width, m_Height);

    {
      TCLock lock(*m_ParentCounterLock);
      (*m_ParentCounter)++;
    }

    m_FinishCV->NotifyOne();
  }
}


ThreadGroup::ThreadGroup( int numThreads, const unsigned char *inBuf, unsigned int inBufSz, CompressionFunc func, unsigned char *outBuf )
  : m_StartBarrier(new TCBarrier(numThreads + 1))
  , m_FinishMutex(new TCMutex())
  , m_FinishCV(new TCConditionVariable())
  , m_NumThreads(numThreads)
  , m_ActiveThreads(0)
  , m_Func(func)
  , m_ImageDataSz(inBufSz)
  , m_ImageData(inBuf)
  , m_OutBuf(outBuf)
  , m_ThreadState(eThreadState_Done)
  , m_ExitFlag(false)
{ 
  for(int i = 0; i < kMaxNumThreads; i++) {
    // Thread synchronization primitives
    m_Threads[i].m_ParentCounterLock = m_FinishMutex;
    m_Threads[i].m_FinishCV = m_FinishCV;
    m_Threads[i].m_ParentCounter = &m_ThreadsFinished;
    m_Threads[i].m_StartBarrier = m_StartBarrier;
    m_Threads[i].m_ParentExitFlag = &m_ExitFlag;
  }
}

ThreadGroup::~ThreadGroup() {
  delete m_StartBarrier;
  delete m_FinishMutex;
  delete m_FinishCV;
}

unsigned int ThreadGroup::GetCompressedBlockSize() {
  if(m_Func == BC7C::CompressImageBC7) return 16;
#ifdef HAS_SSE_41
  if(m_Func == BC7C::CompressImageBC7SIMD) return 16;
#endif
}

unsigned int ThreadGroup::GetUncompressedBlockSize() {
  if(m_Func == BC7C::CompressImageBC7) return 64;
#ifdef HAS_SSE_41
  if(m_Func == BC7C::CompressImageBC7SIMD) return 64;
#endif
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
  int numBlocks = m_ImageDataSz / 64;

  int blocksProcessed = 0;
  int blocksPerThread = (numBlocks/m_NumThreads) + ((numBlocks % m_NumThreads)? 1 : 0);

  // Currently no threads are finished...
  m_ThreadsFinished = 0;
  for(int i = 0; i < m_NumThreads; i++) {

    if(m_ActiveThreads >= kMaxNumThreads)
      break;

    int numBlocksThisThread = blocksPerThread;
    if(blocksProcessed + numBlocksThisThread > numBlocks) {
      numBlocksThisThread = numBlocks - blocksProcessed;
    }

    CmpThread &t = m_Threads[m_ActiveThreads];
    t.m_Height = 4;
    t.m_Width = numBlocksThisThread * 4;
    t.m_CmpFunc = m_Func;
    t.m_OutBuf = m_OutBuf + (blocksProcessed * GetCompressedBlockSize());
    t.m_InBuf = m_ImageData + (blocksProcessed * GetUncompressedBlockSize());

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
  for(int i = 0; i < m_ActiveThreads; i++) {
    m_ThreadHandles[i]->Join();
    delete m_ThreadHandles[i];
  }

  // Reset active number of threads...
  m_ActiveThreads = 0;
  m_ExitFlag = false;
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
