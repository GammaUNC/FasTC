#ifndef __BLOCK_STATS_H__
#define __BLOCK_STATS_H__

#include "TexCompTypes.h"
#include "ReferenceCounter.h"
#include "Thread.h"

struct BlockStat {
  friend class BlockStatManager;
public:
  BlockStat(const CHAR *statName, uint64 stat);
  BlockStat(const CHAR *statName, double stat);

  BlockStat(const BlockStat &);
  BlockStat &operator=(const BlockStat &);
  
private:
  static const int kStatNameSz = 32;
  char m_StatName[kStatNameSz];
  union {
    uint64 m_IntStat;
    double m_FloatStat;
  };
};

class BlockStatManager {
  
 public:
  BlockStatManager(int nBlocks);
  ~BlockStatManager();

  uint32 BeginBlock();
  void AddStat(uint32 blockIdx, const BlockStat &stat);
  
 private:
  
  class BlockStatList {
  public:
    BlockStatList();
    ~BlockStatList();

    void AddStat(const BlockStat &stat);

  private:
    BlockStatList(const BlockStat &stat);

    BlockStat m_Stat;
    BlockStatList *m_Tail;

    ReferenceCounter m_Counter;
  } *m_BlockStatList;
  uint32 m_BlockStatListSz;

  TCMutex m_Mutex;
  uint32 m_NextBlock;
  ReferenceCounter m_Counter;
};

#endif // __BLOCK_STATS_H__
