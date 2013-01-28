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

#include "BlockStats.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "FileStream.h"
#include "Thread.h"

template <typename T>
static T max(const T &a, const T &b) {
  return (a > b)? a : b;
}

////////////////////////////////////////////////////////////////////////////////
//
// BlockStat implementation
//
////////////////////////////////////////////////////////////////////////////////

BlockStat::BlockStat(const CHAR *statName, int stat) 
  : m_Type(eType_Int)
  , m_IntStat(stat)
{
#ifdef _MSC_VER
  strncpy_s(m_StatName, statName, kStatNameSz);
#else
  strncpy(m_StatName, statName, kStatNameSz);
#endif
}

BlockStat::BlockStat(const CHAR *statName, double stat) 
  : m_Type(eType_Float)
  , m_FloatStat(stat)
{
#ifdef _MSC_VER
  strncpy_s(m_StatName, statName, kStatNameSz);
#else
  strncpy(m_StatName, statName, kStatNameSz);
#endif
}

BlockStat::BlockStat(const BlockStat &other) : m_Type(other.m_Type) {
  memcpy(this, &other, sizeof(*this));
}

BlockStat &BlockStat::operator=(const BlockStat &other) {
  memcpy(this, &other, sizeof(*this));
  return *this;
}

void BlockStat::ToString(CHAR *buf, int bufSz) const {
  switch(m_Type) {
    case BlockStat::eType_Float:
#ifdef _MSC_VER
      _sntprintf_s(buf, bufSz, _TRUNCATE, "%s,%f", m_StatName, m_FloatStat);
#else
      snprintf(buf, bufSz, "%s,%f", m_StatName, m_FloatStat);
#endif
    break;

    case BlockStat::eType_Int:
#ifdef _MSC_VER
      _sntprintf_s(buf, bufSz, _TRUNCATE, "%s,%lu", m_StatName, m_IntStat);
#else
      snprintf(buf, bufSz, "%s,%lu", m_StatName, m_IntStat);
#endif
    break;

    default:
      assert(!"Unknown stat type!");
    break;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// BlockStat Manager Implementation
//
////////////////////////////////////////////////////////////////////////////////

void BlockStatManager::Copy(const BlockStatManager &other) {
  // This is a bug. If we copy the manager then all of the lists and pointers
  // become shared and can cause dereferencing issues. Check to see where you're
  // copying this class and make sure to actually create a new instance.
  assert(!"We shouldn't be copying these in this manner!");

  m_BlockStatList = new BlockStatList(*other.m_BlockStatList);
  m_BlockStatListSz = other.m_BlockStatListSz;
  m_NextBlock = other.m_NextBlock;

  // If we do copy them, then make sure that we are actually using the exact same
  // pointers for our synchronization primitives... otherwise we could run into
  // deadlock issues.
  m_Mutex = other.m_Mutex;
}

BlockStatManager::BlockStatManager(const BlockStatManager &other) {
	Copy(other);
}

BlockStatManager &BlockStatManager::operator=(const BlockStatManager &other) {
	m_Counter = other.m_Counter;
	Copy(other);
	return *this;
}

BlockStatManager::BlockStatManager(int nBlocks) 
  : m_BlockStatListSz(max(nBlocks, 0))
  , m_NextBlock(0)
  , m_Mutex(new TCMutex)
{
  m_BlockStatList = new BlockStatList[m_BlockStatListSz];
  if(!m_BlockStatList) {
    fprintf(stderr, "Out of memory!\n");
    assert(false);
    exit(1);
  }
}

BlockStatManager::~BlockStatManager() {
  if(m_Counter.GetRefCount() == 0) {
    delete [] m_BlockStatList;
  }

  if(m_Mutex)
  {
	delete m_Mutex;
	m_Mutex = 0;
  }
}

uint32 BlockStatManager::BeginBlock() {
  if(m_NextBlock == m_BlockStatListSz) {
    fprintf(stderr, "WARNING -- BlockStatManager::BeginBlock(), reached end of block list.\n");
    assert(false);
    return m_NextBlock-1;
  }

  TCLock lock (*m_Mutex);
  return m_NextBlock++;
}

void BlockStatManager::AddStat(uint32 blockIdx, const BlockStat &stat) {
  if(blockIdx >= m_BlockStatListSz) {
    fprintf(stderr, "WARNING -- BlockStatManager::AddStat(), block index out of bounds!\n");
    assert(false);
    return;
  }

  TCLock lock (*m_Mutex);
  m_BlockStatList[blockIdx].AddStat(stat);
}

void BlockStatManager::ToFile(const CHAR *filename) {
  
  FileStream fstr (filename, eFileMode_Write);

  for(uint32 i = 0; i < m_BlockStatListSz; i++) {
    const BlockStatList *head = &(m_BlockStatList[i]);
    while(head) {
      BlockStat s = head->GetStat();

      CHAR statStr[256];
      s.ToString(statStr, 256);

      CHAR str[256];
#ifdef _MSC_VER
	  _sntprintf_s(str, 256, _TRUNCATE, "%d,%s\n", i, statStr);
#else
      snprintf(str, 256, "%d,%s\n", i, statStr);
#endif
      
      uint32 strLen = uint32(strlen(str));
      if(strLen > 255) {
	    str[255] = '\n';
	    strLen = 256;
      }
 
      fstr.Write((uint8 *)str, strLen);

      head = head->GetTail();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// BlockStat List Implementation
//
////////////////////////////////////////////////////////////////////////////////
static const CHAR *kNullBlockString = "NULL_BLOCK_STAT";
static const uint32 kNullBlockStringLength = uint32(strlen(kNullBlockString));

BlockStatManager::BlockStatList::BlockStatList() 
  : m_Tail(0)
  , m_Stat(kNullBlockString, 0.0)
{ }

BlockStatManager::BlockStatList::BlockStatList(const BlockStat &stat) 
  : m_Tail(0)
  , m_Stat(stat)
{
	assert(!"If you're copying a block stat list then you're probably not using them properly.");
}

BlockStatManager::BlockStatList::BlockStatList(const BlockStatList &other)
	: m_Tail(new BlockStatList(*other.m_Tail))
	, m_Stat(other.m_Stat)
{}

BlockStatManager::BlockStatList::~BlockStatList() {
  if(m_Counter.GetRefCount() == 0 && m_Tail) {
    delete m_Tail;
  }
}

void BlockStatManager::BlockStatList::AddStat(const BlockStat &stat) {

  if(strncmp(stat.m_StatName, m_Stat.m_StatName, BlockStat::kStatNameSz) == 0) {
    m_Stat = stat;
  }
  else if(m_Tail) {
    m_Tail->AddStat(stat);
  }
  else {
    if(strncmp(m_Stat.m_StatName, kNullBlockString, kNullBlockStringLength) == 0) {
      m_Stat = stat;
    }
    else {
      m_Tail = new BlockStatList(stat);
    }
  }
}
