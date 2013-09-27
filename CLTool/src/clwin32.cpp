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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDKDDKVer.h>
#include <Windows.h>

#include "BlockStats.h"
#include "TexComp.h"
#include "ImageFile.h"
#include "Image.h"

void PrintUsage() {
  fprintf(stderr, "Usage: tc [OPTIONS] imagefile\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "\t-f\t\tFormat to use. Either \"BPTC\" or \"PVRTC\". Default: BPTC\n");
  fprintf(stderr, "\t-l\t\tSave an output log.\n");
  fprintf(stderr, "\t-q <quality>\tSet compression quality level. Default: 50\n");
  fprintf(stderr, "\t-n <num>\tCompress the image num times and give the average time and PSNR. Default: 1\n");
  fprintf(stderr, "\t-simd\t\tUse SIMD compression path\n");
  fprintf(stderr, "\t-t <num>\tCompress the image using <num> threads. Default: 1\n");
  fprintf(stderr, "\t-a \t\tCompress the image using synchronization via atomic operations. Default: Off\n");
  fprintf(stderr, "\t-j <num>\tUse <num> blocks for each work item in a worker queue threading model. Default: (Blocks / Threads)\n");
}

void ExtractBasename(const char *filename, char *buf, uint32 bufSz) {
  size_t len = strlen(filename);
  const char *end = filename + len;
  while(--end != filename) {
    if(*end == '.')
    {
      uint32 numChars = int32(end - filename + 1);
      uint32 toCopy = (numChars > bufSz)? bufSz : numChars;
      memcpy(buf, filename, toCopy);
      buf[toCopy - 1] = '\0';
      return;
    }
  }
}

int _tmain(int argc, _TCHAR* argv[])
{
  int fileArg = 1;
  if(fileArg == argc) {
    PrintUsage();
    exit(1);
  }

  int numJobs = 0;
  int quality = 50;
  int numThreads = 1;
  int numCompressions = 1;
  bool bUseSIMD = false;
  bool bSaveLog = false;
  bool bUseAtomics = false;
  ECompressionFormat format = eCompressionFormat_BPTC;
  
  bool knowArg = false;
  do {
    knowArg = false;

    if(strcmp(argv[fileArg], "-f") == 0) {
      fileArg++;

      if(fileArg == argc) {
        PrintUsage();
        exit(1);
      } else {
        if(!strcmp(argv[fileArg], "PVRTC")) {
          format = eCompressionFormat_PVRTC;
        }
      }

      fileArg++;
      knowArg = true;
      continue;
    }

    if(strcmp(argv[fileArg], "-n") == 0) {
      fileArg++;

      if(fileArg == argc || (numCompressions = atoi(argv[fileArg])) < 0) {
        PrintUsage();
        exit(1);
      }

      fileArg++;
      knowArg = true;
      continue;
    }

    if(strcmp(argv[fileArg], "-l") == 0) {
      fileArg++;
      bSaveLog = true;
      knowArg = true;
      continue;
    }
    
    if(strcmp(argv[fileArg], "-simd") == 0) {
      fileArg++;
      bUseSIMD = true;
      knowArg = true;
      continue;
    }

    if(strcmp(argv[fileArg], "-t") == 0) {
      fileArg++;
      
      if(fileArg == argc || (numThreads = atoi(argv[fileArg])) < 1) {
        PrintUsage();
        exit(1);
      }

      fileArg++;
      knowArg = true;
      continue;
    }

    if(strcmp(argv[fileArg], "-q") == 0) {
      fileArg++;
      
      if(fileArg == argc || (quality = atoi(argv[fileArg])) < 0) {
        PrintUsage();
        exit(1);
      }

      fileArg++;
      knowArg = true;
      continue;
    }

    if(strcmp(argv[fileArg], "-j") == 0) {
      fileArg++;
      
      if(fileArg == argc || (numJobs = atoi(argv[fileArg])) < 0) {
        PrintUsage();
        exit(1);
      }

      fileArg++;
      knowArg = true;
      continue;
    }

    if(strcmp(argv[fileArg], "-a") == 0) {
      fileArg++;
      bUseAtomics = true;
      knowArg = true;
      continue;
    }
  } while(knowArg && fileArg < argc);

  if(numThreads > 1 && bSaveLog) {
    bSaveLog = false;
    fprintf(stderr, "WARNING: Will not save log because implementation is not thread safe.\n"
      "If you'd like, send a complaint to pavel@cs.unc.edu to get this done faster.\n");
  }

  if(fileArg == argc) {
    PrintUsage();
    exit(1);
  }

  char basename[256];
  ExtractBasename(argv[fileArg], basename, 256);

  ImageFile file (argv[fileArg]);
  if(!file.Load()) {
    fprintf(stderr, "Error loading file: %s\n", argv[fileArg]);
    return 1;
  }

  const Image *img = file.GetImage();
  if(format == eCompressionFormat_PVRTC) {
    const_cast<Image *>(img)->SetBlockStreamOrder(false);
  }

  int numBlocks = (img->GetWidth() * img->GetHeight())/16;
  BlockStatManager *statManager = NULL;
  if(bSaveLog) {
    statManager = new BlockStatManager(numBlocks);
  }
  
  SCompressionSettings settings;
  settings.format = format;
  settings.bUseSIMD = bUseSIMD;
  settings.bUseAtomics = bUseAtomics;
  settings.iNumThreads = numThreads;
  settings.iQuality = quality;
  settings.iNumCompressions = numCompressions;
  settings.iJobSize = numJobs;
  settings.pStatManager = statManager;

  CompressedImage *ci = img->Compress(settings);
  if(NULL == ci) {
    fprintf(stderr, "Error compressing image!\n");
    return 1;
  }

  double PSNR = img->ComputePSNR(*ci);
  if(PSNR > 0.0) {
    fprintf(stdout, "PSNR: %.3f\n", PSNR);
  }
  else {
    fprintf(stderr, "Error computing PSNR\n");
  }

  if(bSaveLog) {
    strcat_s(basename, ".log");
    statManager->ToFile(basename);
    basename[strlen(basename) - 4] = '\0';
  }

  Image cImg (*ci);
  if(format == eCompressionFormat_BPTC) {
    strcat_s(basename, "-bc7.png");
  } else if(format == eCompressionFormat_PVRTC) {
    strcat_s(basename, "-pvrtc.png");
  }

  ImageFile cImgFile (basename, eFileFormat_PNG, cImg);
  cImgFile.Write();

  // Cleanup 
  delete ci;
  if(statManager)
    delete statManager;

  return 0;
}
