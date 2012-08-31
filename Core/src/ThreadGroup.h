#ifndef _THREAD_GROUP_H_
#define _THREAD_GROUP_H_

#include "TexComp.h"
#include "StopWatch.h"

// forward declare
namespace boost {
  class barrier;
  class thread;
  class mutex;
  class condition_variable;
}

struct CmpThread {
  friend class ThreadGroup;  

private:
  int *m_ParentCounter;

  boost::mutex *m_ParentCounterLock;
  boost::condition_variable *m_FinishCV;

  boost::barrier *m_Barrier;

  int m_Width;
  int m_Height;

  CompressionFunc m_CmpFunc;

  unsigned char *m_OutBuf;
  const unsigned char *m_InBuf;

  CmpThread();

public:
  void operator ()();
};


class ThreadGroup {
 public:
  ThreadGroup( int numThreads, const ImageFile &, CompressionFunc func, unsigned char *outBuf );
  ~ThreadGroup();

  void Start();
  void Join();

  const StopWatch &GetStopWatch() const { return m_StopWatch; }

 private:
  boost::barrier *const m_Barrier;
  boost::mutex *const m_FinishMutex;
  boost::condition_variable *const m_FinishCV;

  static const int kMaxNumThreads = 256;
  const int m_NumThreads;

  int m_ActiveThreads;
  int m_ThreadsFinished;

  CmpThread m_Threads[kMaxNumThreads];
  boost::thread *m_ThreadHandles[kMaxNumThreads];

  // State variables.
  const ImageFile &m_Image;
  const CompressionFunc m_Func;
  unsigned char *m_OutBuf;

  unsigned int GetCompressedBlockSize();
  unsigned int GetUncompressedBlockSize();

  StopWatch m_StopWatch;
};

#endif // _THREAD_GROUP_H_
