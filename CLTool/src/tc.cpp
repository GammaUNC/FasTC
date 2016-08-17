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

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <algorithm>
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

#include "FasTC/Image.h"
#include "FasTC/ImageFile.h"
#include "FasTC/TexComp.h"
#include "FasTC/ThreadSafeStreambuf.h"

void PrintUsage() {
  fprintf(stderr, "Usage: tc [OPTIONS] imagefile\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "\t-h|--help\tPrint this help.\n");
  fprintf(stderr, "\t-v\t\tVerbose mode: prints out Entropy, Mean Local Entropy, and MSSIM\n");
  fprintf(stderr, "\t-f <fmt>\tFormat to use. Either \"BPTC\", \"ETC1\", \"DXT1\", \"DXT5\", or \"PVRTC\". Default: BPTC\n");
  fprintf(stderr, "\t-l\t\tSave an output log.\n");
  fprintf(stderr, "\t-d <file>\tSpecify decompressed output (default: basename-<fmt>.png)\n");
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

  if (!base) {
    base = end;
  }

  size_t numChars = ext - base + 1;
  size_t toCopy = ::std::min(numChars, bufSz);
  memcpy(buf, base, toCopy);
  buf[toCopy - 1] = '\0';
  return;
}

int main(int argc, char **argv) {

  int fileArg = 1;
  if (fileArg == argc) {
    PrintUsage();
    exit(1);
  }

  char decompressedOutput[256];
  decompressedOutput[0] = '\0';
  bool bDecompress = true;
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

    if (strcmp(argv[fileArg], "-n") == 0) {
      fileArg++;

      if (fileArg == argc || (numCompressions = atoi(argv[fileArg])) < 0) {
        PrintUsage();
        exit(1);
      }

      fileArg++;
      knowArg = true;
      continue;
    }

    if (strcmp(argv[fileArg], "-f") == 0) {
      fileArg++;

      if (fileArg == argc) {
        PrintUsage();
        exit(1);
      } else {
        if (!strcmp(argv[fileArg], "PVRTC")) {
          format = FasTC::eCompressionFormat_PVRTC4;
        } else if (!strcmp(argv[fileArg], "PVRTCLib")) {
          format = FasTC::eCompressionFormat_PVRTC4;
          bUsePVRTexLib = true;
        } else if (!strcmp(argv[fileArg], "BPTCLib")) {
          format = FasTC::eCompressionFormat_BPTC;
          bUseNVTT = true;
        } else if (!strcmp(argv[fileArg], "ETC1")) {
          format = FasTC::eCompressionFormat_ETC1;
        } else if (!strcmp(argv[fileArg], "DXT1")) {
          format = FasTC::eCompressionFormat_DXT1;
        } else if (!strcmp(argv[fileArg], "DXT5")) {
          format = FasTC::eCompressionFormat_DXT5;
        }
      }

      fileArg++;
      knowArg = true;
      continue;
    }

    if (strcmp(argv[fileArg], "-h") == 0 || strcmp(argv[fileArg], "--help") == 0) {
      PrintUsage();
      exit(0);
    }

    if (strcmp(argv[fileArg], "-d") == 0) {
      fileArg++;

      if (fileArg == argc) {
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

    if (strcmp(argv[fileArg], "-nd") == 0) {
      fileArg++;
      bDecompress = false;
      knowArg = true;
      continue;
    }

    if (strcmp(argv[fileArg], "-l") == 0) {
      fileArg++;
      bSaveLog = true;
      knowArg = true;
      continue;
    }

    if (strcmp(argv[fileArg], "-v") == 0) {
      fileArg++;
      bVerbose = true;
      knowArg = true;
      continue;
    }

    if (strcmp(argv[fileArg], "-simd") == 0) {
      fileArg++;
      bUseSIMD = true;
      knowArg = true;
      continue;
    }

    if (strcmp(argv[fileArg], "-t") == 0) {
      fileArg++;

      if (fileArg == argc || (numThreads = atoi(argv[fileArg])) < 1) {
        PrintUsage();
        exit(1);
      }

      fileArg++;
      knowArg = true;
      continue;
    }

    if (strcmp(argv[fileArg], "-q") == 0) {
      fileArg++;

      if (fileArg == argc || (quality = atoi(argv[fileArg])) < 0) {
        PrintUsage();
        exit(1);
      }

      fileArg++;
      knowArg = true;
      continue;
    }

    if (strcmp(argv[fileArg], "-j") == 0) {
      fileArg++;

      if (fileArg == argc || (numJobs = atoi(argv[fileArg])) < 0) {
        PrintUsage();
        exit(1);
      }

      fileArg++;
      knowArg = true;
      continue;
    }

    if (strcmp(argv[fileArg], "-a") == 0) {
      fileArg++;
      bUseAtomics = true;
      knowArg = true;
      continue;
    }

  } while (knowArg && fileArg < argc);

  if (fileArg == argc) {
    PrintUsage();
    exit(1);
  }

  char basename[256];
  ExtractBasename(argv[fileArg], basename, 256);

  ImageFile file(argv[fileArg]);
  if (!file.Load()) {
    return 1;
  }

  FasTC::Image<> img(*file.GetImage());

  if (bVerbose) {
    fprintf(stdout, "Entropy: %.5f\n", img.ComputeEntropy());
    fprintf(stdout, "Mean Local Entropy: %.5f\n", img.ComputeMeanLocalEntropy());
  }

  std::ofstream logFile;
  ThreadSafeStreambuf streamBuf(logFile);
  std::ostream logStream(&streamBuf);
  if (bSaveLog) {
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
  if (bSaveLog) {
    settings.logStream = &logStream;
  } else {
    settings.logStream = NULL;
  }

  CompressedImage *ci = CompressImage(&img, settings);
  if (NULL == ci) {
    return 1;
  }

  if (ci->GetWidth() != img.GetWidth() ||
      ci->GetHeight() != img.GetHeight()) {
    fprintf(stderr, "Cannot compute image metrics: compressed and uncompressed dimensions differ.\n");
  } else {
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
  }

  if(bDecompress) {
    if(decompressedOutput[0] != '\0') {
      memcpy(basename, decompressedOutput, 256);
    } else if(format == FasTC::eCompressionFormat_BPTC) {
      strcat(basename, "-bptc.png");
    } else if(format == FasTC::eCompressionFormat_PVRTC4) {
      strcat(basename, "-pvrtc-4bpp.png");
    } else if(format == FasTC::eCompressionFormat_DXT1) {
      strcat(basename, "-dxt1.png");
    } else if(format == FasTC::eCompressionFormat_ETC1) {
      strcat(basename, "-etc1.png");
    }

    EImageFileFormat fmt = ImageFile::DetectFileFormat(basename);
    ImageFile cImgFile (basename, fmt, *ci);
    cImgFile.Write();
  }

  // Cleanup 
  delete ci;
  if(bSaveLog) {
    logFile.close();
  }
  return 0;
}
