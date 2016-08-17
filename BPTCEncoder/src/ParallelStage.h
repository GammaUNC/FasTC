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

#include "FasTC/TexCompTypes.h"

enum BPTCParallelStage {
  eParallelStage_Uniform,
  eParallelStage_Partitioned,
  eParallelStage_Normal,
  
  kNumParallelStages
};

class ParallelStage {
 public:
  ParallelStage(
    BPTCParallelStage stage,
    const unsigned char *inbuf,
    unsigned char *outbuf,
    uint32 numBlocks,
    uint32 outBlockSz = 16,
    uint32 inBlockSz = 64
  );
  ParallelStage(const ParallelStage &);
  ParallelStage &operator=(const ParallelStage &);
  
  ~ParallelStage();

  const BPTCParallelStage m_Stage;

  // Adds the block number to the list of blocks for this parallel stage
  void AddBlock(uint32 blockNum);

  // Loads the desired number of blocks into the destination buffer. Returns
  // the number of blocks loaded.
  uint32 LoadBlocks(uint32 blockOffset, uint32 numBlocks, unsigned char *dst);

  // Writes the block data from src into numBlocks blocks starting from
  // the block given by blockOffset.
  bool WriteBlocks(uint32 blockOffset, uint32 numBlocks, const unsigned char *src);

 private:

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

  const uint32 m_OutBlockSz;
  const uint32 m_InBlockSz;
};
