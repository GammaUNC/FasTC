/* FasTC
 * Copyright (c) 2013 University of North Carolina at Chapel Hill.
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

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#ifdef _MSC_VER
#  include <SDKDDKVer.h>
#  include <Windows.h>
#  undef min
#  undef max
#endif

#include "Image.h"
#include "ImageFile.h"
#include "TexComp.h"
#include "ThreadSafeStreambuf.h"

void PrintUsage() {
  fprintf(stderr, "Usage: tc [OPTIONS] imagefile\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "\t-v\t\tVerbose mode: prints out Entropy, Mean Local Entropy, and MSSIM\n");
  fprintf(stderr, "\t-f <fmt>\tFormat to use. Either \"BPTC\", \"ETC1\", \"DXT1\", \"DXT5\", or \"PVRTC\". Default: BPTC\n");
  fprintf(stderr, "\t-l\t\tSave an output log.\n");
  fprintf(stderr, "\t-d <file>\tSpecify decompressed output (currently only png files supported, default: basename-<fmt>.png)\n");
  fprintf(stderr, "\t-nd\t\tSuppress decompressed output\n");
  fprintf(stderr, "\t-q <quality>\tSet compression quality level. Default: 50\n");
  fprintf(stderr, "\t-n <num>\tCompress the image num times and give the average time and PSNR. Default: 1\n");
  fprintf(stderr, "\t-simd\t\tUse SIMD compression path\n");
  fprintf(stderr, "\t-t <num>\tCompress the image using <num> threads. Default: 1\n");
  fprintf(stderr, "\t-a \t\tCompress the image using synchronization via atomic operations. Default: Off\n");
  fprintf(stderr, "\t-j <num>\tUse <num> blocks for each work item in a worker queue threading model. Default: (Blocks / Threads)\n");
}

void ExtractBasename(const char *filename, char *buf, size_t bufSz) {
  size_t len = strlen(filename);
  const char *end = filename + len;
  const char *ext = end;
  const char *base = NULL;
  while(--end != filename && !base) {
    if(*end == '.') {
      ext = end;
    } else if(*end == '\\' || *end == '/') {
      base = end + 1;
    }
  }

  size_t numChars = ext - base + 1;
  size_t toCopy = ::std::min(numChars, bufSz);
  memcpy(buf, base, toCopy);
  buf[toCopy - 1] = '\0';
  return;
}

int main(int argc, char **argv) {

  int fileArg = 1;
  if(fileArg == argc) {
    PrintUsage();
    exit(1);
  }

  char decompressedOutput[256]; decompressedOutput[0] = '\0';
  bool bNoDecompress = false;
  int numJobs = 0;
  int quality = 50;
  int numThreads = 1;
  int numCompressions = 1;
  bool bUseSIMD = false;
  bool bSaveLog = false;
  bool bUseAtomics = false;
  bool bUsePVRTexLib = false;
  bool bUseNVTT = false;
  bool bVerbose = false;
  FasTC::ECompressionFormat format = FasTC::eCompressionFormat_BPTC;

  bool knowArg = false;
  do {
    knowArg = false;

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

    if(strcmp(argv[fileArg], "-f") == 0) {
      fileArg++;

      if(fileArg == argc) {
        PrintUsage();
        exit(1);
      } else {
        if(!strcmp(argv[fileArg], "PVRTC")) {
          format = FasTC::eCompressionFormat_PVRTC;
        } else if(!strcmp(argv[fileArg], "PVRTCLib")) {
          format = FasTC::eCompressionFormat_PVRTC;
          bUsePVRTexLib = true;
        } else if(!strcmp(argv[fileArg], "BPTCLib")) {
          format = FasTC::eCompressionFormat_BPTC;
          bUseNVTT = true;
        } else if(!strcmp(argv[fileArg], "ETC1")) {
          format = FasTC::eCompressionFormat_ETC1;
        } else if(!strcmp(argv[fileArg], "DXT1")) {
          format = FasTC::eCompressionFormat_DXT1;
        } else if(!strcmp(argv[fileArg], "DXT5")) {
          format = FasTC::eCompressionFormat_DXT5;
        }
      }

      fileArg++;
      knowArg = true;
      continue;
    }

    if(strcmp(argv[fileArg], "-d") == 0) {
      fileArg++;
      
      if(fileArg == argc) {
        PrintUsage();
        exit(1);
      } else {
        size_t sz = 255;
        sz = ::std::min(sz, static_cast<size_t>(strlen(argv[fileArg])));
        memcpy(decompressedOutput, argv[fileArg], sz + 1);
      }

      fileArg++;
      knowArg = true;
      continue;
    }

    if(strcmp(argv[fileArg], "-nd") == 0) {
      fileArg++;
      bNoDecompress = true;
      knowArg = true;
      continue;
    }

    if(strcmp(argv[fileArg], "-l") == 0) {
      fileArg++;
      bSaveLog = true;
      knowArg = true;
      continue;
    }
    
    if(strcmp(argv[fileArg], "-v") == 0) {
      fileArg++;
      bVerbose = true;
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

  FasTC::Image<> img(*file.GetImage());

  if(bVerbose) {
    fprintf(stdout, "Entropy: %.5f\n", img.ComputeEntropy());
    fprintf(stdout, "Mean Local Entropy: %.5f\n", img.ComputeMeanLocalEntropy());
  }

  std::ofstream logFile;
  ThreadSafeStreambuf streamBuf(logFile);
  std::ostream logStream(&streamBuf);
  if(bSaveLog) {
    char logname[256];
    sprintf(logname, "%s.log", basename);
    logFile.open(logname);
  }
  
  SCompressionSettings settings;
  settings.format = format;
  settings.bUseSIMD = bUseSIMD;
  settings.bUseAtomics = bUseAtomics;
  settings.iNumThreads = numThreads;
  settings.iQuality = quality;
  settings.iNumCompressions = numCompressions;
  settings.iJobSize = numJobs;
  settings.bUsePVRTexLib = bUsePVRTexLib;
  settings.bUseNVTT = bUseNVTT;
  if(bSaveLog) {
    settings.logStream = &logStream;
  } else {
    settings.logStream = NULL;
  }

  CompressedImage *ci = CompressImage(&img, settings);
  if(NULL == ci) {
    fprintf(stderr, "Error compressing image!\n");
    return 1;
  }

  double PSNR = img.ComputePSNR(ci);
  if(PSNR > 0.0) {
    fprintf(stdout, "PSNR: %.3f\n", PSNR);
  }
  else {
    fprintf(stderr, "Error computing PSNR\n");
  }

  if(bVerbose) {
    double SSIM = img.ComputeSSIM(ci);
    if(SSIM > 0.0) {
      fprintf(stdout, "SSIM: %.9f\n", SSIM);
    } else {
      fprintf(stderr, "Error computing SSIM\n");
    }
  }

  if(!bNoDecompress) {
    if(decompressedOutput[0] != '\0') {
      memcpy(basename, decompressedOutput, 256);
    } else if(format == FasTC::eCompressionFormat_BPTC) {
      strcat(basename, "-bc7.png");
    } else if(format == FasTC::eCompressionFormat_PVRTC) {
      strcat(basename, "-pvrtc.png");
    } else if(format == FasTC::eCompressionFormat_DXT1) {
      strcat(basename, "-dxt1.png");
    } else if(format == FasTC::eCompressionFormat_ETC1) {
      strcat(basename, "-etc1.png");
    }

    ImageFile cImgFile (basename, eFileFormat_PNG, *ci);
    cImgFile.Write();
  }

  // Cleanup 
  delete ci;
  if(bSaveLog) {
    logFile.close();
  }
  return 0;
}
