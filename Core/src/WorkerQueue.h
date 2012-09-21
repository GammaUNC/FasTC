#ifndef __TEXCOMP_WORKDER_QUEUE_H__
#define __TEXCOMP_WORKDER_QUEUE_H__

#include "TexCompTypes.h"
#include "TexComp.h"

// Forward declare...
class WorkerQueue;
namespace boost {
  class thread;
  class mutex;
  class barrier;
  class condition_variable;
}

struct WorkerThread {
  friend class WorkerQueue;
public:

  WorkerThread(WorkerQueue *, uint32 idx);
  void operator ()();

private:
  uint32 m_ThreadIdx;
  WorkerQueue *const m_Parent;
};

class WorkerQueue {
  friend class WorkerThread;
 public:
  WorkerQueue(
    uint32 numThreads, 
    uint32 jobSize,
    const uint8 *inBuf, 
    uint32 inBufSz, 
    CompressionFunc func, 
    uint8 *outBuf
  );

  ~WorkerQueue() { }

  // Runs the 
  void Run();

 private:

  static const int kMaxNumWorkerThreads = 256;
  int m_Offsets[kMaxNumWorkerThreads];

  int GetOffsetForThread(const int threadIdx) const;
  
  const CompressionFunc m_CompressionFunc;
  CompressionFunc GetCompressionFunc() const { return m_CompressionFunc; }

  void SignalThreadReady(int threadIdx);
  bool AcceptThreadData(int threadIdx) const;
};

#endif //__TEXCOMP_WORKDER_QUEUE_H__
