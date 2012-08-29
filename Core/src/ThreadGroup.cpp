#include "ThreadGroup.h"

#include <stdlib.h>
#include <stdio.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>

CmpThread::CmpThread() 
  : m_Barrier(NULL)
  , m_Width(0)
  , m_Height(0)
  , m_CmpFunc(NULL)
  , m_OutBuf(NULL)
  , m_InBuf(NULL)
{ }

void CmpThread::operator()() {
  if(!m_Barrier || !m_CmpFunc || !m_OutBuf || !m_InBuf ) {
    fprintf(stderr, "Incorrect thread initialization.\n");
    return;
  }

  m_Barrier->wait();
}


ThreadGroup::ThreadGroup( int numThreads, const ImageFile &, CompressionFunc func, unsigned char *outBuf )
  : m_Barrier(new boost::barrier(numThreads))
  , m_NumThreads(numThreads)
  , m_ActiveThreads(0)
{ }

ThreadGroup::~ThreadGroup() {
  delete m_Barrier;
}

void ThreadGroup::Start() {

  for(int i = 0; i < m_NumThreads; i++) {

    if(m_ActiveThreads >= kMaxNumThreads)
      break;

    CmpThread &t = m_Threads[m_ActiveThreads];
    m_ThreadHandles[m_ActiveThreads] = new boost::thread(t);

    m_ActiveThreads++;
  }

}

void ThreadGroup::Join() {

  for(int i = 0; i < m_ActiveThreads; i++) {
    m_ThreadHandles[i]->join();
    delete m_ThreadHandles[i];
  }

  m_ActiveThreads = 0;
}
