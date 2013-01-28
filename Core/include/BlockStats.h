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

#ifndef __BLOCK_STATS_H__
#define __BLOCK_STATS_H__

#include "TexCompTypes.h"
#include "ReferenceCounter.h"

///////////////////////////////////////////////////////////////////////////////
//
// Forward declarations
//
///////////////////////////////////////////////////////////////////////////////
class TCMutex;

///////////////////////////////////////////////////////////////////////////////
//
// class BlockStat
//
///////////////////////////////////////////////////////////////////////////////

struct BlockStat {
  friend class BlockStatManager;
public:
  BlockStat(const CHAR *statName, int);
  BlockStat(const CHAR *statName, double stat);

  BlockStat(const BlockStat &);
  BlockStat &operator=(const BlockStat &);

  void ToString(CHAR *buf, int bufSz) const;
  
private:
  const enum Type {
    eType_Float,
    eType_Int,
    
    kNumTypes
  } m_Type;

  static const int kStatNameSz = 32;
  CHAR m_StatName[kStatNameSz];
  union {
    uint64 m_IntStat;
    double m_FloatStat;
  };
};

class BlockStatManager {
  
 public:
  BlockStatManager(int nBlocks);
  BlockStatManager(const BlockStatManager &);
  BlockStatManager &operator=(const BlockStatManager &);
  ~BlockStatManager();

  uint32 BeginBlock();
  void AddStat(uint32 blockIdx, const BlockStat &stat);
  void ToFile(const CHAR *filename);
  
 private:
  
  class BlockStatList {
  public:
    BlockStatList();
    BlockStatList(const BlockStatList &other);
    ~BlockStatList();

    void AddStat(const BlockStat &stat);
    BlockStat GetStat() const { return m_Stat; }
    const BlockStatList *GetTail() const { return m_Tail; }

  private:
    BlockStatList(const BlockStat &stat);

    BlockStatList *m_Tail;
    BlockStat m_Stat;

    ReferenceCounter m_Counter;
  } *m_BlockStatList;
  uint32 m_BlockStatListSz;

  uint32 m_NextBlock;
  ReferenceCounter m_Counter;

  TCMutex *m_Mutex;

  // Note: we probably shouldn't call this...
  void Copy(const BlockStatManager &);
};

#endif // __BLOCK_STATS_H__
