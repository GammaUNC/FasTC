/* FasTC
 * Copyright (c) 2014 University of North Carolina at Chapel Hill.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for educational, research, and non-profit purposes, without
 * fee, and without a written agreement is hereby granted, provided that the
 * above copyright notice, this paragraph, and the following four paragraphs
 * appear in all copies.
 *
 * Permission to incorporate this software into commercial products may be
 * obtained by contacting the authors or the Office of Technology Development
 * at the University of North Carolina at Chapel Hill <otd@unc.edu>.
 *
 * This software program and documentation are copyrighted by the University of
 * North Carolina at Chapel Hill. The software program and documentation are
 * supplied "as is," without any accompanying services from the University of
 * North Carolina at Chapel Hill or the authors. The University of North
 * Carolina at Chapel Hill and the authors do not warrant that the operation of
 * the program will be uninterrupted or error-free. The end-user understands
 * that the program was developed for research purposes and is advised not to
 * rely exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL OR THE
 * AUTHORS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF
 * THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF NORTH CAROLINA
 * AT CHAPEL HILL OR THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL AND THE AUTHORS SPECIFICALLY
 * DISCLAIM ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND ANY 
 * STATUTORY WARRANTY OF NON-INFRINGEMENT. THE SOFTWARE PROVIDED HEREUNDER IS ON
 * AN "AS IS" BASIS, AND THE UNIVERSITY  OF NORTH CAROLINA AT CHAPEL HILL AND
 * THE AUTHORS HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, 
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
