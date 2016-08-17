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
#include <sys/time.h>

class StopWatchImpl {
public:
  uint64 start;
  uint64 duration;
};

static uint64 Now() {
  timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_usec + (1e6 * tv.tv_sec);
}

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
  impl->start = Now();
}

void StopWatch::Stop() {
  impl->duration = Now() - impl->start;
}

void StopWatch::Reset() {
  impl->start = impl->duration = 0.0;
}

double StopWatch::TimeInSeconds() const {
  return double(impl->duration) / 1e6;
}

double StopWatch::TimeInMilliseconds() const {
  return double(impl->duration) / 1e3;
}

double StopWatch::TimeInMicroseconds() const {
  return double(impl->duration);
}
