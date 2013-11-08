/* FasTC
 * Copyright (c) 2012 University of North Carolina at Chapel Hill. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its documentation for educational, 
 * research, and non-profit purposes, without fee, and without a written agreement is hereby granted, 
 * provided that the above copyright notice, this paragraph, and the following four paragraphs appear 
 * in all copies.
 *
 * Permission to incorporate this software into commercial products may be obtained by contacting the 
 * authors or the Office of Technology Development at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of North Carolina at Chapel Hill. 
 * The software program and documentation are supplied "as is," without any accompanying services from the 
 * University of North Carolina at Chapel Hill or the authors. The University of North Carolina at Chapel Hill 
 * and the authors do not warrant that the operation of the program will be uninterrupted or error-free. The 
 * end-user understands that the program was developed for research purposes and is advised not to rely 
 * exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE AUTHORS BE LIABLE TO ANY PARTY FOR 
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE 
 * USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE 
 * AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING, 
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY 
 * OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
 * ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Please send all BUG REPORTS to <pavel@cs.unc.edu>.
 *
 * The authors may be contacted via:
 *
 * Pavel Krajcevski
 * Dept of Computer Science
 * 201 S Columbia St
 * Frederick P. Brooks, Jr. Computer Science Bldg
 * Chapel Hill, NC 27599-3175
 * USA
 * 
 * <http://gamma.cs.unc.edu/FasTC/>
 */

#ifndef __TEXCOMP_WORKDER_QUEUE_H__
#define __TEXCOMP_WORKDER_QUEUE_H__

// Forward declare...
class WorkerQueue;

// Necessary includes...
#include "TexCompTypes.h"
#include "Thread.h"
#include "StopWatch.h"
#include "CompressionFuncs.h"

#include <iosfwd>

class WorkerThread : public TCCallable {
  friend class WorkerQueue;
public:

  WorkerThread(WorkerQueue *, uint32 idx);
  virtual ~WorkerThread() { }
  virtual void operator ()();

  enum EAction {
    eAction_Wait,
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
    const FasTC::CompressionJob &job,
    CompressionFunc func
  );

  WorkerQueue(
    uint32 numCompressions,
    uint32 numThreads, 
    uint32 jobSize,
    const FasTC::CompressionJob &job,
    CompressionFuncWithStats func, 
    std::ostream *logStream
  );

  ~WorkerQueue() { }

  // Runs the workers
  void Run();
  const StopWatch &GetStopWatch() const { return m_StopWatch; }

 private:
  uint32 m_NumCompressions;
  const uint32 m_TotalNumCompressions;
  uint32 m_NumThreads;
  uint32 m_WaitingThreads;
  uint32 m_ActiveThreads;
  uint32 m_JobSize;
  FasTC::CompressionJob m_Job;

  TCConditionVariable m_CV;
  TCMutex m_Mutex;

  uint32 m_NextBlock;

  static const int kMaxNumWorkerThreads = 256;
  uint32 m_Offsets[kMaxNumWorkerThreads];
  uint32 m_NumBlocks[kMaxNumWorkerThreads];

  WorkerThread *m_Workers[kMaxNumWorkerThreads];
  TCThread *m_ThreadHandles[kMaxNumWorkerThreads];

  const FasTC::CompressionJob &GetCompressionJob() const { return m_Job; }
  void GetStartForThread(const uint32 threadIdx, uint32 (&start)[2]);
  void GetEndForThread(const uint32 threadIdx, uint32 (&start)[2]);
  
  const CompressionFunc m_CompressionFunc;
  CompressionFunc GetCompressionFunc() const { return m_CompressionFunc; }

  const CompressionFuncWithStats m_CompressionFuncWithStats;
  CompressionFuncWithStats GetCompressionFuncWithStats() const { return m_CompressionFuncWithStats; }

  std::ostream *m_LogStream;
  std::ostream *GetLogStream() const { return m_LogStream; }

  StopWatch m_StopWatch;

  WorkerThread::EAction AcceptThreadData(uint32 threadIdx);
  void NotifyWorkerFinished();
};

#endif //__TEXCOMP_WORKDER_QUEUE_H__
