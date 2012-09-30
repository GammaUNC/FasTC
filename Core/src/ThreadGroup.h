#ifndef _THREAD_GROUP_H_
#define _THREAD_GROUP_H_

#include "TexComp.h"
#include "Thread.h"
#include "StopWatch.h"

struct CmpThread : public TCCallable {
  friend class ThreadGroup;  

private:
  TCBarrier *m_StartBarrier;

  int *m_ParentCounter;
  
  TCMutex *m_ParentCounterLock;
  TCConditionVariable *m_FinishCV;

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
  ThreadGroup( 
    int numThreads, 
    const unsigned char *inBuf, 
    unsigned int inBufSz, 
    CompressionFunc func, 
    unsigned char *outBuf
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

  static const int kMaxNumThreads = 256;
  const int m_NumThreads;

  int m_ActiveThreads;
  int m_ThreadsFinished;

  CmpThread m_Threads[kMaxNumThreads];
  TCThread *m_ThreadHandles[kMaxNumThreads];

  // State variables.
  const unsigned int m_ImageDataSz;
  const unsigned char *const m_ImageData;
  const CompressionFunc m_Func;
  unsigned char *m_OutBuf;

  unsigned int GetCompressedBlockSize();
  unsigned int GetUncompressedBlockSize();

  StopWatch m_StopWatch;

  EThreadState m_ThreadState;
  bool m_ExitFlag;
};

#endif // _THREAD_GROUP_H_
