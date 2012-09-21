#ifndef __TEXCOMP_STOP_WATCH_H__
#define __TEXCOMP_STOP_WATCH_H__

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

// Forward declare the private implementation of the class that will actually implement
// the timing features. This class is defined in each module depending on the platform...
class StopWatchImpl;

// A simple stopwatch class using Windows' high-resolution performance counters.
class StopWatch
{
public:
	StopWatch();
	StopWatch(const StopWatch &);

	~StopWatch();

	StopWatch &operator=(const StopWatch &);

	void Start();
	void Stop();
	void Reset();

	double TimeInSeconds() const;
	double TimeInMilliseconds() const;
	double TimeInMicroseconds() const;

private:
	StopWatchImpl *impl;
};

#endif // __TEXCOMP_STOP_WATCH_H__
