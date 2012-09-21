#ifndef __TEXCOMP_WORKDER_QUEUE_H__
#define __TEXCOMP_WORKDER_QUEUE_H__

// Forward declare...
class WorkerQueue;
namespace boost {
  class thread;
}

// Necessary includes...
#include "TexCompTypes.h"
#include "TexComp.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include "StopWatch.h"

struct WorkerThread {
  friend class WorkerQueue;
public:

  WorkerThread(WorkerQueue *, uint32 idx);
  void operator ()();

  enum EAction {
    eAction_DoWork,
    eAction_Quit,

    kNumWorkerThreadActions
  };

private:
  uint32 m_ThreadIdx;
  WorkerQueue *const m_Parent;
};

class WorkerQueue {
  friend class WorkerThread;
 public:
  WorkerQueue(
    uint32 numCompressions,
    uint32 numThreads, 
    uint32 jobSize,
    const uint8 *inBuf, 
    uint32 inBufSz, 
    CompressionFunc func, 
    uint8 *outBuf
  );

  ~WorkerQueue() { }

  // Runs the workers
  void Run();
  const StopWatch &GetStopWatch() const { return m_StopWatch; }

 private:

  uint32 m_NumCompressions;
  const uint32 m_TotalNumCompressions;
  uint32 m_NumThreads;
  uint32 m_ActiveThreads;
  uint32 m_JobSize;
  uint32 m_InBufSz;
  const uint8 *m_InBuf;
  uint8 *m_OutBuf;

  boost::condition_variable m_CV;
  boost::mutex m_Mutex;
  uint32 m_NextBlock;

  static const int kMaxNumWorkerThreads = 256;
  uint32 m_Offsets[kMaxNumWorkerThreads];
  uint32 m_NumBlocks[kMaxNumWorkerThreads];

  boost::thread *m_ThreadHandles[kMaxNumWorkerThreads];

  const uint8 *GetSrcForThread(const int threadIdx) const;
  uint8 *GetDstForThread(const int threadIdx) const;
  uint32 GetNumBlocksForThread(const int threadIdx) const;
  
  const CompressionFunc m_CompressionFunc;
  CompressionFunc GetCompressionFunc() const { return m_CompressionFunc; }

  StopWatch m_StopWatch;

  WorkerThread::EAction AcceptThreadData(uint32 threadIdx);
  void NotifyWorkerFinished();
};

#endif //__TEXCOMP_WORKDER_QUEUE_H__
