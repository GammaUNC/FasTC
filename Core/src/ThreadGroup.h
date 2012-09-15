#ifndef _THREAD_GROUP_H_
#define _THREAD_GROUP_H_

#include "TexComp.h"
#include "StopWatch.h"

// forward declare
namespace boost {
  class thread;
  class mutex;
  class barrier;
  class condition_variable;
}

struct CmpThread {
  friend class ThreadGroup;  

private:
  boost::barrier *m_StartBarrier;

  int *m_ParentCounter;
  
  boost::mutex *m_ParentCounterLock;
  boost::condition_variable *m_FinishCV;

  int m_Width;
  int m_Height;

  CompressionFunc m_CmpFunc;

  unsigned char *m_OutBuf;
  const unsigned char *m_InBuf;

  bool *m_ParentExitFlag;

  CmpThread();

public:
  void operator ()();
};


class ThreadGroup {
 public:
  ThreadGroup( int numThreads, const ImageFile &, CompressionFunc func, unsigned char *outBuf );
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
  boost::barrier *const m_StartBarrier;

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

  EThreadState m_ThreadState;
  bool m_ExitFlag;
};

#endif // _THREAD_GROUP_H_
