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

#ifndef __COMPRESSION_JOBS_H__
#define __COMPRESSION_JOBS_H__

#include "TexCompTypes.h"

#ifdef _MSC_VER
#   define ALIGN(x) __declspec( align(x) )
#else
#   define ALIGN(x) __attribute__((aligned(x)))
#endif
#define ALIGN_SSE ALIGN(16)

// This structure defines a compression job. Here, width and height are the dimensions
// of the image in pixels. inBuf contains the R8G8B8A8 data that is to be compressed, and
// outBuf will contain the compressed BC7 data.
//
// Implicit sizes:
//    inBuf - (width * height * 4) bytes
//    outBuf - (width * height) bytes
struct CompressionJob {
  const unsigned char *inBuf;
  unsigned char *outBuf;
  const uint32 width;
  const uint32 height;

  CompressionJob(
    const unsigned char *_inBuf,
    unsigned char *_outBuf,
    const uint32 _width,
    const uint32 _height
  ) :
  inBuf(_inBuf),
  outBuf(_outBuf),
  width(_width),
  height(_height)
  { }
};
  
// This struct mirrors that for a compression job, but is used to decompress a BC7 stream. Here, inBuf
// is a buffer of BC7 data, and outBuf is the destination where we will copy the decompressed R8G8B8A8 data
struct DecompressionJob {
  const unsigned char *const inBuf;
  unsigned char *const outBuf;
  const uint32 width;
  const uint32 height;

  DecompressionJob(
    const unsigned char *_inBuf,
    unsigned char *_outBuf,
    const uint32 _width,
    const uint32 _height
  ) :
  inBuf(_inBuf),
  outBuf(_outBuf),
  width(_width),
  height(_height)
  { }
};

// A structure for maintaining a list of textures to compress.
struct CompressionJobList {
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
    const uint32 GetTotalNumJobs() const { return m_TotalNumJobs; }

    // Get the current number of jobs in the list.
    const uint32 GetNumJobs() const { return m_NumJobs; }

    // Returns the specified job.
    const CompressionJob *GetJob(uint32 idx) const;
    uint32 *GetFinishedFlag(uint32 idx) const;
    
    ALIGN(32) uint32 m_CurrentBlockIndex;
    ALIGN(32) uint32 m_CurrentJobIndex;

  private:
    CompressionJob *m_Jobs;
    uint32 m_NumJobs;
    const uint32 m_TotalNumJobs;

    struct FinishedFlag{
      ALIGN(32) uint32 m_flag;
    } *m_FinishedFlags;
};

#endif // __COMPRESSION_JOBS_H__