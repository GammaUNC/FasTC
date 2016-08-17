// Copyright 2016 The University of North Carolina at Chapel Hill
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Please send all BUG REPORTS to <pavel@cs.unc.edu>.
// <http://gamma.cs.unc.edu/FasTC/>

// The original lisence from the code available at the following location:
// http://software.intel.com/en-us/vcsource/samples/fast-texture-compression
//
// This code has been modified significantly from the original.

//--------------------------------------------------------------------------------------
// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//
//--------------------------------------------------------------------------------------

#include "FasTC/StopWatch.h"
#include "FasTC/TexCompTypes.h"

#include <cassert>
#include <Windows.h>
#include <WinBase.h>

class StopWatchImpl {
public:
  uint64 frequency;
  uint64 start;
  uint64 stop;
#ifndef __MINGW32__
  uintptr_t affinityMask;
#endif

  StopWatchImpl() :
    start(0), stop(0)
#ifndef __MINGW32__
    , affinityMask(0)
#endif
  {
    // Initialize the performance counter frequency.
    LARGE_INTEGER perfQuery;
#ifndef NDEBUG
    assert(QueryPerformanceFrequency(&perfQuery));
#else
    QueryPerformanceFrequency(&perfQuery);
#endif
    this->frequency = perfQuery.QuadPart;
  }
};

StopWatch::StopWatch() : impl(new StopWatchImpl) { }

StopWatch::StopWatch(const StopWatch &other) {
  impl = new StopWatchImpl();
  memcpy(impl, other.impl, sizeof(StopWatchImpl));
}

StopWatch &StopWatch::operator=(const StopWatch &other) {
  if(impl) {
    delete impl;
  }
  impl = new StopWatchImpl();
  memcpy(impl, other.impl, sizeof(StopWatchImpl));
  return *this;
}

StopWatch::~StopWatch() {
  delete impl;
}

// Start the stopwatch.
void StopWatch::Start()
{
#ifndef __MINGW32__
  // MSDN recommends setting the thread affinity to avoid bugs in the BIOS and HAL.
  // Create an affinity mask for the current processor.
  impl->affinityMask = (DWORD_PTR)1 << GetCurrentProcessorNumber();
  HANDLE currThread = GetCurrentThread();
  DWORD_PTR prevAffinityMask = SetThreadAffinityMask(currThread, impl->affinityMask);
  assert(prevAffinityMask != 0);
#endif

  // Query the performance counter.
  LARGE_INTEGER perfQuery;
#ifndef NDEBUG
  assert(QueryPerformanceCounter(&perfQuery));
#else
  QueryPerformanceCounter(&perfQuery);
#endif
  impl->start = perfQuery.QuadPart;

#ifndef __MINGW32__
  // Restore the thread's affinity mask.
  prevAffinityMask = SetThreadAffinityMask(currThread, prevAffinityMask);
  assert(prevAffinityMask != 0);
#endif
}

// Stop the stopwatch.
void StopWatch::Stop()
{
#ifndef __MINGW32__
  // MSDN recommends setting the thread affinity to avoid bugs in the BIOS and HAL.
  // Use the affinity mask that was created in the Start function.
  HANDLE currThread = GetCurrentThread();
  DWORD_PTR prevAffinityMask = SetThreadAffinityMask(currThread, impl->affinityMask);
  assert(prevAffinityMask != 0);
#endif

  // Query the performance counter.
  LARGE_INTEGER perfQuery;
#ifndef NDEBUG
  assert(QueryPerformanceCounter(&perfQuery));
#else
  QueryPerformanceCounter(&perfQuery);
#endif
  impl->stop = perfQuery.QuadPart;

#ifndef __MINGW32__
  // Restore the thread's affinity mask.
  prevAffinityMask = SetThreadAffinityMask(currThread, prevAffinityMask);
  assert(prevAffinityMask != 0);
#endif
}

// Reset the stopwatch.
void StopWatch::Reset()
{
  impl->start = 0;
  impl->stop = 0;
#ifndef __MINGW32__
  impl->affinityMask = 0;
#endif
}

// Get the elapsed time in seconds.
double StopWatch::TimeInSeconds() const
{
  // Return the elapsed time in seconds.
  assert((impl->stop - impl->start) > 0);
  return double(impl->stop - impl->start) / double(impl->frequency);
}

// Get the elapsed time in milliseconds.
double StopWatch::TimeInMilliseconds() const
{
  // Return the elapsed time in milliseconds.
  assert((impl->stop - impl->start) > 0);
  return double(impl->stop - impl->start) / double(impl->frequency) * 1000.0;
}

// Get the elapsed time in microseconds.
double StopWatch::TimeInMicroseconds() const
{
  // Return the elapsed time in microseconds.
  assert((impl->stop - impl->start) > 0);
  return double(impl->stop - impl->start) / double(impl->frequency) * 1000000.0;
}
