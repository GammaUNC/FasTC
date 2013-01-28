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

    BlockStat m_Stat;
    BlockStatList *m_Tail;

    ReferenceCounter m_Counter;
  } *m_BlockStatList;
  uint32 m_BlockStatListSz;

  TCMutex *m_Mutex;
  uint32 m_NextBlock;
  ReferenceCounter m_Counter;

  // Note: we probably shouldn't call this...
  void Copy(const BlockStatManager &);
};

#endif // __BLOCK_STATS_H__
