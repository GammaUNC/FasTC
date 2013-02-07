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

#include "ParallelStage.h"

#include <assert.h>
#include <string.h>

/*
 const BC7ParallelStage stage;
 
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
  BC7ParallelStage stage,
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
