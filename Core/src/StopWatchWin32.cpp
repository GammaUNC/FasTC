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
