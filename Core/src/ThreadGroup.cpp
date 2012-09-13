#include "ThreadGroup.h"
#include "BC7Compressor.h"

#include <stdlib.h>
#include <stdio.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

CmpThread::CmpThread() 
  : m_ParentCounter(NULL)
  , m_ParentCounterLock(NULL)
  , m_FinishCV(NULL)
  , m_Barrier(NULL)
  , m_Width(0)
  , m_Height(0)
  , m_CmpFunc(NULL)
  , m_OutBuf(NULL)
  , m_InBuf(NULL)
{ }

void CmpThread::operator()() {
  if(!m_Barrier || !m_CmpFunc || !m_OutBuf || !m_InBuf 
     || !m_ParentCounter || !m_ParentCounterLock
     || !m_FinishCV
  ) {
    fprintf(stderr, "Incorrect thread initialization.\n");
    return;
  }

  // Wait for all threads to be ready...
  m_Barrier->wait();

  (*m_CmpFunc)(m_InBuf, m_OutBuf, m_Width, m_Height);

  {
    boost::lock_guard<boost::mutex> lock(*m_ParentCounterLock);
    (*m_ParentCounter)++;
  }

  m_FinishCV->notify_one();
}


ThreadGroup::ThreadGroup( int numThreads, const ImageFile &image, CompressionFunc func, unsigned char *outBuf )
  : m_Barrier(new boost::barrier(numThreads))
  , m_FinishMutex(new boost::mutex())
  , m_FinishCV(new boost::condition_variable())
  , m_NumThreads(numThreads)
  , m_ActiveThreads(0)
  , m_Func(func)
  , m_Image(image)
  , m_OutBuf(outBuf)
{ 
  for(int i = 0; i < kMaxNumThreads; i++) {
    // Thread synchronization primitives
    m_Threads[i].m_ParentCounterLock = m_FinishMutex;
    m_Threads[i].m_FinishCV = m_FinishCV;
    m_Threads[i].m_ParentCounter = &m_ThreadsFinished;
    m_Threads[i].m_Barrier = m_Barrier;
  }
}

ThreadGroup::~ThreadGroup() {
  delete m_Barrier;
  delete m_FinishMutex;
  delete m_FinishCV;
}

unsigned int ThreadGroup::GetCompressedBlockSize() {
  if(m_Func == BC7C::CompressImageBC7) return 16;
  if(m_Func == BC7C::CompressImageBC7SIMD) return 16;
}

unsigned int ThreadGroup::GetUncompressedBlockSize() {
  if(m_Func == BC7C::CompressImageBC7) return 64;
  if(m_Func == BC7C::CompressImageBC7SIMD) return 64;
}

void ThreadGroup::Start() {

  // Have we already activated the thread group?
  if(m_ActiveThreads > 0) {
    return;
  }

  // Make sure that the image dimensions are multiples of 4
  assert((m_Image.GetWidth() & 3) == 0);
  assert((m_Image.GetHeight() & 3) == 0);

  // We can assume that the image data is in block stream order
  // so, the size of the data given to each thread will be (nb*4)x4
  int numBlocks = (m_Image.GetWidth() * m_Image.GetHeight()) / 16;

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
    t.m_InBuf = m_Image.GetPixels() + (blocksProcessed * GetUncompressedBlockSize());

    blocksProcessed += numBlocksThisThread;
    
    m_ThreadHandles[m_ActiveThreads] = new boost::thread(t);

    m_ActiveThreads++;
  }

  m_StopWatch.Reset();
  m_StopWatch.Start();
}

void ThreadGroup::Join() {

  boost::unique_lock<boost::mutex> lock(*m_FinishMutex);
  while(m_ThreadsFinished != m_ActiveThreads) {
    m_FinishCV->wait(lock);
  }

  m_StopWatch.Stop();

  for(int i = 0; i < m_ActiveThreads; i++) {
    m_ThreadHandles[i]->join();
    delete m_ThreadHandles[i];
  }

  // Reset active number of threads...
  m_ActiveThreads = 0;
}
