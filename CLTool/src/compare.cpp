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

  srand(time(NULL));
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

  if (diff_images) {
    FasTC::Image<> diff = img1.Diff(&img2, diff_multiplier);

    char fname_buf [5 + 16 + 4]; // "diff-" + hash + ".png"
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
