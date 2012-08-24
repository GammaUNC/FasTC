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

#pragma once

#include "Windows.h"

// A simple stopwatch class using Windows' high-resolution performance counters.
class StopWatch
{
public:
	StopWatch();

	void Start();
	void Stop();
	void Reset();

	double TimeInSeconds() const;
	double TimeInMilliseconds() const;
	double TimeInMicroseconds() const;

private:
	LONGLONG frequency;
	LONGLONG start;
	LONGLONG stop;
	DWORD_PTR affinityMask;
};
