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

#ifndef CORE_INCLUDE_THREADSAFESTREAMBUF_H_
#define CORE_INCLUDE_THREADSAFESTREAMBUF_H_

// Forward Declarations
class TCMutex;

#include <streambuf>
#include <iosfwd>

class ThreadSafeStreambuf : public ::std::streambuf {
 public:
  ThreadSafeStreambuf(std::ostream &sink);
  virtual ~ThreadSafeStreambuf();

 protected:
  virtual std::streamsize xsputn(const char_type *s, std::streamsize count);
 private:
  // Not implemented -- not allowed...
  ThreadSafeStreambuf(const ThreadSafeStreambuf &);
  ThreadSafeStreambuf &operator=(const ThreadSafeStreambuf &);
  std::ostream &m_Sink;
  TCMutex *m_Mutex;
};

#endif  // CORE_INCLUDE_THREADSAFESTREAMBUF_H_
