#ifndef _THREAD_GROUP_H_
#define _THREAD_GROUP_H_

#include "TexComp.h"
#include "StopWatch.h"

// forward declare
namespace boost {
  class barrier;
  class thread;
}

struct CmpThread {
  friend class ThreadGroup;  

private:
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
  boost::barrier *m_Barrier;

  static const int kMaxNumThreads = 256;
  const int m_NumThreads;

  int m_ActiveThreads;

  CmpThread m_Threads[kMaxNumThreads];
  boost::thread *m_ThreadHandles[kMaxNumThreads];

  StopWatch m_StopWatch;
};

#endif // _THREAD_GROUP_H_
