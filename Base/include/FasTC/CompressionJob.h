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

#ifndef __COMPRESSION_JOBS_H__
#define __COMPRESSION_JOBS_H__

#include "FasTC/TexCompTypes.h"
#include "FasTC/CompressionFormat.h"

#ifdef _MSC_VER
#   define ALIGN(x) __declspec( align(x) )
#else
#   define ALIGN(x) __attribute__((aligned(x)))
#endif
#define ALIGN_SSE ALIGN(16)

namespace FasTC {

  // This structure defines a compression job. Here, width and height are the dimensions
  // of the image in pixels. inBuf contains the R8G8B8A8 data that is to be compressed, and
  // outBuf will contain the compressed BPTC data.
  //
  // Implicit sizes:
  //    inBuf - (width * height * 4) bytes
  //    outBuf - (width * height) bytes
  class CompressionJob {
   private:
    ECompressionFormat m_Format;
    const uint8 *m_InBuf;
    uint8 *m_OutBuf;
    uint32 m_Width;
    uint32 m_Height;
    uint32 m_XStart, m_XEnd;
    uint32 m_YStart, m_YEnd;

   public:
    ECompressionFormat Format() const { return m_Format; }
    const uint8 *InBuf() const { return m_InBuf; }
    uint8 *OutBuf() const { return m_OutBuf; }
    uint32 Width() const { return m_Width; }
    uint32 Height() const { return m_Height; }
    uint32 XStart() const { return m_XStart; }
    uint32 XEnd() const { return m_XEnd; }
    uint32 YStart() const { return m_YStart; }
    uint32 YEnd() const { return m_YEnd; }

    CompressionJob(
      ECompressionFormat _fmt,
      const uint8 *_inBuf,
      unsigned char *_outBuf,
      const uint32 _width,
      const uint32 _height)
      : m_Format(_fmt)
      , m_InBuf(_inBuf)
      , m_OutBuf(_outBuf)
      , m_Width(_width)
      , m_Height(_height)
      , m_XStart(0), m_XEnd(_width)
      , m_YStart(0), m_YEnd(_height)
    { }

    CompressionJob(
      ECompressionFormat _fmt,
      const uint8 *_inBuf,
      unsigned char *_outBuf,
      const uint32 _width,
      const uint32 _height,
      const uint32 _xOffset,
      const uint32 _yOffset)
      : m_Format(_fmt)
      , m_InBuf(_inBuf)
      , m_OutBuf(_outBuf)
      , m_Width(_width)
      , m_Height(_height)
      , m_XStart(_xOffset), m_XEnd(_width)
      , m_YStart(_yOffset), m_YEnd(_height)
    { }

    CompressionJob(
      ECompressionFormat _fmt,
      const uint8 *_inBuf,
      unsigned char *_outBuf,
      const uint32 _width,
      const uint32 _height,
      const uint32 _xOffset,
      const uint32 _yOffset,
      const uint32 _xEndpoint,
      const uint32 _yEndpoint)
      : m_Format(_fmt)
      , m_InBuf(_inBuf)
      , m_OutBuf(_outBuf)
      , m_Width(_width)
      , m_Height(_height)
      , m_XStart(_xOffset), m_XEnd(_xEndpoint)
      , m_YStart(_yOffset), m_YEnd(_yEndpoint)
    { }

    // Returns the x and y coordinates of the pixels that corresponds to the block
    // index for the given format.
    void BlockIdxToCoords(uint32 blockIdx, uint32 (&out)[2]) const {
      uint32 blockDim[2];
      GetBlockDimensions(Format(), blockDim);

      const uint32 kNumBlocksX = Width() / blockDim[0];

      const uint32 blockX = blockIdx % kNumBlocksX;
      const uint32 blockY = blockIdx / kNumBlocksX;

      out[0] = blockX * blockDim[0];
      out[1] = blockY * blockDim[1];
    }

    // Returns the x and y coordinates of the pixels that corresponds to the block
    // index for the given format.
    uint32 CoordsToBlockIdx(uint32 x, uint32 y) const {
      uint32 blockDim[2];
      GetBlockDimensions(Format(), blockDim);

      const uint32 kNumBlocksX = Width() / blockDim[0];

      const uint32 blockX = x / blockDim[0];
      const uint32 blockY = y / blockDim[1];

      return blockY * kNumBlocksX + blockX;
    }
  };
  
  // This struct mirrors that for a compression job, but is used to decompress a BPTC stream. Here, inBuf
  // is a buffer of BPTC data, and outBuf is the destination where we will copy the decompressed R8G8B8A8 data
  class DecompressionJob {
   private:
    const ECompressionFormat m_Format;
    const uint8 *m_InBuf;
    uint8 *m_OutBuf;
    const uint32 m_Width;
    const uint32 m_Height;

   public:
    const uint8 *InBuf() const { return m_InBuf; }
    uint8 *OutBuf() const { return m_OutBuf; }
    uint32 Width() const { return m_Width; }
    uint32 Height() const { return m_Height; }
    ECompressionFormat Format() const { return m_Format; }

    DecompressionJob(
      ECompressionFormat _fmt,
      const uint8 *_inBuf, uint8 *_outBuf,
      uint32 _width, uint32 _height)
      : m_Format(_fmt)
      , m_InBuf(_inBuf)
      , m_OutBuf(_outBuf)
      , m_Width(_width)
      , m_Height(_height)
      { }
  };

  // A structure for maintaining a list of textures to compress.
  class CompressionJobList {
   public:

    // Initialize the list by specifying the total number of jobs that it will contain.
    // This constructor allocates the necessary memory to hold the array.
    CompressionJobList(const uint32 nJobs);
    ~CompressionJobList();

    // Overrides to deal with memory management.
    CompressionJobList(const CompressionJobList &);
    CompressionJobList &operator =(const CompressionJobList &);

    // Add a job to the list. This function returns false on failure.
    bool AddJob(const CompressionJob &);

    // Get the maximum number of jobs that this list can hold.
    uint32 GetTotalNumJobs() const { return m_TotalNumJobs; }

    // Get the current number of jobs in the list.
    uint32 GetNumJobs() const { return m_NumJobs; }

    // Returns the specified job.
    const CompressionJob *GetJob(uint32 idx) const;
    uint32 *GetFinishedFlag(uint32 idx) const;
    
   private:
    CompressionJob *m_Jobs;
    uint32 m_NumJobs;
    const uint32 m_TotalNumJobs;

    struct FinishedFlag{
      ALIGN(32) uint32 m_flag;
    } *m_FinishedFlags;

   public:
    ALIGN(32) uint32 m_CurrentJobIndex;
    ALIGN(32) uint32 m_CurrentBlockIndex;
  };

}  // namespace FasTC
#endif // __COMPRESSION_JOBS_H__
