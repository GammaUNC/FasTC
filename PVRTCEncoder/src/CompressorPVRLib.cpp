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

// Our library include...
#include "FasTC/PVRTCCompressor.h"

// PVRLib library include...
#include "PVRTextureUtilities.h"

#include <cassert>

namespace PVRTCC {

  void CompressPVRLib(const FasTC::CompressionJob &cj,
                      bool bTwoBitMode,
                      const EWrapMode) {
    pvrtexture::CPVRTextureHeader pvrTexHdr;
    pvrTexHdr.setPixelFormat(pvrtexture::PVRStandard8PixelType);
    pvrTexHdr.setWidth(cj.Width());
    pvrTexHdr.setHeight(cj.Height());
    pvrTexHdr.setIsFileCompressed(false);
    pvrTexHdr.setIsPreMultiplied(false);

    pvrtexture::CPVRTexture pvrTex = pvrtexture::CPVRTexture(pvrTexHdr, cj.InBuf());
    bool result = pvrtexture::Transcode(pvrTex,
                                        ePVRTPF_PVRTCI_4bpp_RGBA,
                                        ePVRTVarTypeUnsignedByte,
                                        ePVRTCSpacelRGB,
                                        pvrtexture::ePVRTCFast);
    assert(result);
    (void)result;

    memcpy(cj.OutBuf(), static_cast<uint8 *>(pvrTex.getDataPtr()), cj.Width() * cj.Height() / 2);
  }

}  // namespace PVRTCC
