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

void PrintUsage() {
  fprintf(stderr, "Usage: decomp <in_img> <out_img>\n");
  fprintf(stderr, "\tIf in_img is not a compressed image, then this tool simply copies the image.\n");
}

int main(int argc, char **argv) {
  if(argc != 3) {
    return 1;
  }

  ImageFile imgf (argv[1]);
  if(!imgf.Load()) {
    return 1;
  }

  FasTC::Image<> *img = imgf.GetImage();

  EImageFileFormat fmt = ImageFile::DetectFileFormat(argv[2]);
  ImageFile cImgFile (argv[2], fmt, *img);
  cImgFile.Write();

  return 0;
}
