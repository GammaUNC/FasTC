#include "StopWatch.h"
#include "TexCompTypes.h"

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
