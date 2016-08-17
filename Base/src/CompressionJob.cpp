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

#include "FasTC/CompressionJob.h"

#include <new>
#include <cstdlib>
#include <cstring>
#include <cassert>

namespace FasTC {

// Initialize the list by specifying the total number of jobs that it will contain.
// This constructor allocates the necessary memory to hold the array.
CompressionJobList::CompressionJobList(const uint32 nJobs) 
  : m_NumJobs(0)
  , m_TotalNumJobs(nJobs)
  , m_CurrentJobIndex(0)
  , m_CurrentBlockIndex(0)
{
  m_FinishedFlags = new FinishedFlag[nJobs];
  memset(m_FinishedFlags, 0, nJobs * sizeof(m_FinishedFlags[0]));

  m_Jobs = (CompressionJob *)(malloc(nJobs * sizeof(m_Jobs[0])));
}

CompressionJobList::~CompressionJobList() {
  delete [] m_FinishedFlags;
  free(m_Jobs);
}

// Overrides to deal with memory management.
CompressionJobList::CompressionJobList(const CompressionJobList &other)
  : m_NumJobs(other.m_NumJobs)
  , m_TotalNumJobs(other.m_TotalNumJobs)
  , m_CurrentJobIndex(other.m_CurrentJobIndex)
  , m_CurrentBlockIndex(other.m_CurrentBlockIndex)
{
  uint32 arraySz = m_TotalNumJobs * sizeof(m_Jobs[0]);
  m_Jobs = (CompressionJob *)malloc(arraySz);
  memcpy(m_Jobs, other.m_Jobs, arraySz);

  m_FinishedFlags = new FinishedFlag[m_TotalNumJobs];
  memcpy(m_FinishedFlags, other.m_FinishedFlags, m_TotalNumJobs * sizeof(m_FinishedFlags[0]));
}
 
CompressionJobList &CompressionJobList::operator =(const CompressionJobList &other) {
  assert(m_TotalNumJobs == other.m_TotalNumJobs);

  m_NumJobs = other.m_NumJobs;
  m_CurrentJobIndex = other.m_CurrentJobIndex;
  m_CurrentBlockIndex = other.m_CurrentBlockIndex;

  // Get rid of old variables...
  free(m_Jobs);
  delete [] m_FinishedFlags;

  // Make new variables.
  uint32 arraySz = m_TotalNumJobs * sizeof(m_Jobs[0]);
  m_Jobs = (CompressionJob *)malloc(arraySz);
  memcpy(m_Jobs, other.m_Jobs, arraySz);

  m_FinishedFlags = new FinishedFlag[m_TotalNumJobs];
  memcpy(m_FinishedFlags, other.m_FinishedFlags, m_TotalNumJobs * sizeof(m_FinishedFlags[0]));
  return *this;
}

// Add a job to the list. This function returns false on failure.
bool CompressionJobList::AddJob(const CompressionJob &cj) {
  if(m_NumJobs == m_TotalNumJobs) {
    return false;
  }

  CompressionJob *cjPtr = &(m_Jobs[m_NumJobs++]);
  return 0 != (new (cjPtr) CompressionJob(cj));
}

const CompressionJob *CompressionJobList::GetJob(uint32 idx) const {
  if(idx >= m_NumJobs) {
    return NULL;
  }

  return &(m_Jobs[m_CurrentJobIndex]);
}

uint32 *CompressionJobList::GetFinishedFlag(uint32 idx) const {
  if(idx >= m_NumJobs) {
    return NULL;
  }

  return &(m_FinishedFlags[idx].m_flag);
}

}  // namespace FasTC
