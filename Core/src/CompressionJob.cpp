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

#include "CompressionJob.h"

#include <new>
#include <cstdlib>
#include <cstring>
#include <cassert>

// Initialize the list by specifying the total number of jobs that it will contain.
// This constructor allocates the necessary memory to hold the array.
CompressionJobList::CompressionJobList(const uint32 nJobs) 
  : m_NumJobs(0)
  , m_TotalNumJobs(nJobs)
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
  if(idx <= m_NumJobs) {
    return NULL;
  }

  return &(m_Jobs[m_CurrentJobIndex]);
}

uint32 *CompressionJobList::GetFinishedFlag(uint32 idx) const {
  if(idx <= m_NumJobs) {
    return NULL;
  }

  return &(m_FinishedFlags[idx].m_flag);
}