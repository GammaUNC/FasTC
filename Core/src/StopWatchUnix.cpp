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

#include "FasTC/StopWatch.h"
#include "FasTC/TexCompTypes.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

class StopWatchImpl {
public:
  timespec ts;

  double timer;
  double duration;
};

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

StopWatch::StopWatch() : impl(new StopWatchImpl) {
  Reset();
}

void StopWatch::Start() {
  clock_gettime(CLOCK_REALTIME, &(impl->ts));
  impl->timer = double(impl->ts.tv_sec) + 1e-9 * double(impl->ts.tv_nsec);
}

void StopWatch::Stop() {
  clock_gettime(CLOCK_REALTIME, &(impl->ts));
  impl->duration = -(impl->timer) + (double(impl->ts.tv_sec) + 1e-9 * double(impl->ts.tv_nsec));
}

void StopWatch::Reset() {
  impl->timer = impl->duration = 0.0;
  memset(&(impl->ts), 0, sizeof(timespec));
}

double StopWatch::TimeInSeconds() const {
  return impl->duration;
}

double StopWatch::TimeInMilliseconds() const {
  return impl->duration * 1000;
}

double StopWatch::TimeInMicroseconds() const {
  return impl->duration * 1000000;
}
