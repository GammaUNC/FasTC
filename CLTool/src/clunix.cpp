#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "BlockStats.h"
#include "TexComp.h"
#include "ImageFile.h"
#include "Image.h"

void PrintUsage() {
   fprintf(stderr, "Usage: tc [-l] [-q <quality>] [-n <num>] [-simd] [-t <threads> [-j <jobs>]] <imagefile>\n");
}

void ExtractBasename(const char *filename, char *buf, int bufSz) {
	int len = strlen(filename);
	const char *end = filename + len;
	while(--end != filename) {
		if(*end == '.')
		{
			int numChars = end - filename + 1;
			int toCopy = (numChars > bufSz)? bufSz : numChars;
			memcpy(buf, filename, toCopy);
			buf[toCopy - 1] = '\0';
			return;
		}
	}
}

int main(int argc, char **argv) {

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

  int numBlocks = (img->GetWidth() * img->GetHeight())/16;
  BlockStatManager *statManager = NULL;
  if(bSaveLog) {
    statManager = new BlockStatManager(numBlocks);
  }
  
  SCompressionSettings settings;
  settings.bUseSIMD = bUseSIMD;
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
    statManager->ToFile(strcat(basename, ".log"));
  }

	Image cImg (*ci);
	ImageFile cImgFile (strcat(basename, "-bc7.png"), eFileFormat_PNG, cImg);
	cImgFile.Write();

  // Cleanup 
  delete ci;
  if(statManager)
    delete statManager;

  return 0;
}
