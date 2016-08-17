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

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <fstream>
#ifdef _MSC_VER
#  include <SDKDDKVer.h>
#  include <Windows.h>
#endif

#include "FasTC/Image.h"
#include "FasTC/ImageFile.h"
#include "FasTC/TexComp.h"
#include "FasTC/ThreadSafeStreambuf.h"

static void PrintUsageAndExit() {
  fprintf(stderr, "Usage: compare [-d] <img1> <img2>\n");
  exit(1);
}

void gen_random(char *s, const int len) {
  static const char alphanum[] =
  "0123456789"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz";

  srand(static_cast<unsigned int>(time(NULL)));
  for (int i = 0; i < len; ++i) {
    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  s[len] = 0;
}

int main(int argc, char **argv) {
  if(argc < 3 || argc > 5) {
    PrintUsageAndExit();
  }

  bool diff_images = false;
  float diff_multiplier = 1.0;
  int arg = 1;
  if (strncmp(argv[arg], "-d", 2) == 0) {
    diff_images = true;
    arg++;

    if (argc == 5) {
      diff_multiplier = static_cast<float>(atoi(argv[arg]));
      if (diff_multiplier < 0) {
        PrintUsageAndExit();
      }
      arg++;
    }
  }

  ImageFile img1f (argv[arg]);
  if(!img1f.Load()) {
    fprintf(stderr, "Error loading file: %s\n", argv[arg]);
    return 1;
  }
  arg++;

  ImageFile img2f (argv[arg]);
  if(!img2f.Load()) {
    fprintf(stderr, "Error loading file: %s\n", argv[arg]);
    return 1;
  }
  arg++;

  FasTC::Image<> &img1 = *img1f.GetImage();
  FasTC::Image<> &img2 = *img2f.GetImage();

  if (img1.GetWidth() != img2.GetWidth() ||
      img1.GetHeight() != img2.GetHeight()) {
    std::cerr << "Images differ in dimension!" << std::endl;
    return 1;
  }

  if (diff_images) {
    FasTC::Image<> diff = img1.Diff(&img2, diff_multiplier);

    char fname_buf [5 + 16 + 4 + 1]; // "diff-" + hash + ".png" + null
    memset(fname_buf, 0, sizeof(fname_buf));
    strncat(fname_buf, "diff-", 5);
    gen_random(fname_buf + 5, 16);
    strncat(fname_buf + 5 + 16, ".png", 4);

    EImageFileFormat fmt = ImageFile::DetectFileFormat(fname_buf);
    ImageFile cImgFile (fname_buf, fmt, diff);
    cImgFile.Write();
  }

  double PSNR = img1.ComputePSNR(&img2);
  if(PSNR > 0.0) {
    fprintf(stdout, "PSNR: %.3f\n", PSNR);
  }
  else {
    fprintf(stderr, "Error computing PSNR\n");
  }

  double SSIM = img1.ComputeSSIM(&img2);
  if(SSIM > 0.0) {
    fprintf(stdout, "SSIM: %.9f\n", SSIM);
  } else {
    fprintf(stderr, "Error computing MSSIM\n");
  }

  return 0;
}
