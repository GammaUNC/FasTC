#include "StopWatch.h"
#include "TexCompTypes.h"

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
