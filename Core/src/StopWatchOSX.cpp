#include "StopWatch.h"
#include "TexCompTypes.h"

#include <stdlib.h>
#include <string.h>
#include <mach/mach_time.h>

class StopWatchImpl {
public:
  uint64 start;
  
  double resolution;
  double duration;

  StopWatchImpl() {
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);

    resolution = double(info.numer) / double(info.denom);
    resolution *= 1e-9;
  }
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
  impl->start = mach_absolute_time();
}

void StopWatch::Stop() {
  impl->duration = impl->resolution * (double(mach_absolute_time()) - impl->start);
}

void StopWatch::Reset() {
  impl->start = impl->duration = 0.0;
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
