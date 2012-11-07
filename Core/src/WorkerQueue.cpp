#include "WorkerQueue.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>

#include "BC7Compressor.h"

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
  BlockStatManager *statManager = m_Parent->GetBlockStatManager();

  if(!(f || (fStat && statManager))) {
    fprintf(stderr, "%s\n", "Illegal worker queue initialization -- compression func is NULL.");
    return;
  }

  bool quitFlag = false;
  while(!quitFlag) {
    
    switch(m_Parent->AcceptThreadData(m_ThreadIdx)) 
    {

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
	const uint8 *src = m_Parent->GetSrcForThread(m_ThreadIdx);
	uint8 *dst = m_Parent->GetDstForThread(m_ThreadIdx);
	if(f)
	  (*f)(src, dst, 4 * m_Parent->GetNumBlocksForThread(m_ThreadIdx), 4);
	else
	  (*fStat)(src, dst, 4 * m_Parent->GetNumBlocksForThread(m_ThreadIdx), 4, *statManager);

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
  const uint8 *inBuf,
  uint32 inBufSz,
  CompressionFunc func,
  uint8 *outBuf
)
  : m_NumCompressions(0)
  , m_TotalNumCompressions(std::max(uint32(1), numCompressions))
  , m_NumThreads(numThreads)
  , m_WaitingThreads(0)
  , m_ActiveThreads(0)
  , m_JobSize(std::max(uint32(1), jobSize))
  , m_InBufSz(inBufSz)
  , m_InBuf(inBuf)
  , m_OutBuf(outBuf)
  , m_NextBlock(0)
  , m_CompressionFunc(func)
  , m_CompressionFuncWithStats(NULL)
  , m_BlockStatManager(NULL)
{
  clamp(m_NumThreads, uint32(1), uint32(kMaxNumWorkerThreads));

#ifndef NDEBUG
  if(m_InBufSz % 64) {
    fprintf(stderr, "WorkerQueue.cpp -- WARNING: InBufSz not a multiple of 64. Are you sure that your image dimensions are correct?\n");
  }
#endif
}

WorkerQueue::WorkerQueue(
  uint32 numCompressions,
  uint32 numThreads, 
  uint32 jobSize,
  const uint8 *inBuf, 
  uint32 inBufSz, 
  CompressionFuncWithStats func, 
  BlockStatManager &blockStatManager,
  uint8 *outBuf
)
  : m_NumCompressions(0)
  , m_TotalNumCompressions(std::max(uint32(1), numCompressions))
  , m_NumThreads(numThreads)
  , m_WaitingThreads(0)
  , m_ActiveThreads(0)
  , m_JobSize(std::max(uint32(1), jobSize))
  , m_InBufSz(inBufSz)
  , m_InBuf(inBuf)
  , m_OutBuf(outBuf)
  , m_NextBlock(0)
  , m_CompressionFunc(NULL)
  , m_CompressionFuncWithStats(func)
  , m_BlockStatManager(&blockStatManager)
{
  clamp(m_NumThreads, uint32(1), uint32(kMaxNumWorkerThreads));

#ifndef NDEBUG
  if(m_InBufSz % 64) {
    fprintf(stderr, "WorkerQueue.cpp -- WARNING: InBufSz not a multiple of 64. Are you sure that your image dimensions are correct?\n");
  }
#endif
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
  if(threadIdx < 0 || threadIdx >= m_ActiveThreads) {
    return WorkerThread::eAction_Quit;
  }

  // How many blocks total do we have?
  const uint32 totalBlocks = m_InBufSz / 64;
  
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

const uint8 *WorkerQueue::GetSrcForThread(const int threadIdx) const {
  assert(m_Offsets[threadIdx] >= 0);
  assert(threadIdx >= 0);
  assert(threadIdx < int(m_NumThreads));

  const uint32 inBufBlockSz = 16 * 4;
  return m_InBuf + m_Offsets[threadIdx] * inBufBlockSz;
}

uint8 *WorkerQueue::GetDstForThread(const int threadIdx) const {
  assert(m_Offsets[threadIdx] >= 0);
  assert(threadIdx >= 0);
  assert(threadIdx < int(m_NumThreads));

  const uint32 outBufBlockSz = 16;
  return m_OutBuf + m_Offsets[threadIdx] * outBufBlockSz;
}

uint32 WorkerQueue::GetNumBlocksForThread(const int threadIdx) const {
  assert(m_Offsets[threadIdx] >= 0);
  assert(threadIdx >= 0);
  assert(threadIdx < int(m_NumThreads));

  return m_NumBlocks[threadIdx];
}
