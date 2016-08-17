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

#include "ParallelStage.h"

#include <assert.h>
#include <string.h>

/*
 const BPTCParallelStage stage;
 
 // This is the stream of data that will be used to read the block data.
 const unsigned char *const m_InBuf;
 
 // This is the destination buffer to which the block data will be written to.
 unsigned char *const m_OutBuf;
 
 // This is the array of block offsets that belong to this stage.
 uint32 *m_Blocks;
 
 // This is the total number of blocks in the given image.
 const uint32 m_TotalNumBlocks;
 
 // This is the total number of blocks in this particular stage.
 uint32 m_NumBlocks;
 */    
ParallelStage::ParallelStage(
  BPTCParallelStage stage,
  const unsigned char *inbuf,
  unsigned char *outbuf,
  uint32 numBlocks,
  uint32 outBlockSz,
  uint32 inBlockSz
)
  : m_Stage(stage)
  , m_InBuf(inbuf)
  , m_OutBuf(outbuf)
  , m_Blocks(new uint32[numBlocks])
  , m_TotalNumBlocks(numBlocks)
  , m_NumBlocks(0)
  , m_OutBlockSz(outBlockSz)
  , m_InBlockSz(inBlockSz)
{
  assert(numBlocks > 0);
}

ParallelStage::ParallelStage(const ParallelStage &other)
  : m_Stage(other.m_Stage)
  , m_InBuf(other.m_InBuf)
  , m_OutBuf(other.m_OutBuf)
  , m_Blocks(new uint32[other.m_NumBlocks])
  , m_TotalNumBlocks(other.m_TotalNumBlocks)
  , m_NumBlocks(other.m_NumBlocks)
  , m_OutBlockSz(other.m_OutBlockSz)
  , m_InBlockSz(other.m_InBlockSz)
{
  memcpy(m_Blocks, other.m_Blocks, m_NumBlocks * sizeof(m_Blocks[0]));
}

ParallelStage &ParallelStage::operator=(const ParallelStage &other) {
  assert(m_Stage == other.m_Stage);
  assert(m_InBuf == other.m_InBuf);
  assert(m_OutBuf == other.m_OutBuf);
  assert(m_TotalNumBlocks == other.m_TotalNumBlocks);
  assert(m_NumBlocks == other.m_NumBlocks);
  assert(m_OutBlockSz == other.m_OutBlockSz);
  assert(m_InBlockSz == other.m_InBlockSz);
  
  memcpy(m_Blocks, other.m_Blocks, m_NumBlocks * sizeof(m_Blocks[0]));
  return *this;
}

ParallelStage::~ParallelStage() {
  if(m_Blocks) {
    delete [] m_Blocks;
    m_Blocks = 0;
  }
}

void ParallelStage::AddBlock(uint32 blockNum) {
  assert(m_NumBlocks < m_TotalNumBlocks);
  
  m_Blocks[m_NumBlocks++] = blockNum;
}

uint32 ParallelStage::LoadBlocks(uint32 blockOffset, uint32 numBlocks, unsigned char *dst) {

  if(!dst)
    return 0;

  if(blockOffset + numBlocks > m_NumBlocks)
    return 0;

  int lastBlock = blockOffset + numBlocks;
  for(int i = blockOffset; i < lastBlock; i++)
  {
    uint32 block = m_Blocks[i];
    uint32 bOffset = block * m_InBlockSz;
    memcpy(dst + ((i - blockOffset) * m_InBlockSz), m_InBuf + bOffset, m_InBlockSz);
  }

  return 0;
}

bool ParallelStage::WriteBlocks(uint32 blockOffset, uint32 numBlocks, const unsigned char *src) {
  if(!src)
    return false;

  if(blockOffset + numBlocks > m_NumBlocks)
    return false;

  int lastBlock = blockOffset + numBlocks;
  for(int i = blockOffset; i < lastBlock; i++) {
    uint32 block = m_Blocks[i];
    uint32 bOffset = block * m_OutBlockSz;
    memcpy(m_OutBuf + bOffset, src + ((i-blockOffset) * m_OutBlockSz), m_OutBlockSz);
  }

  return true;
}
