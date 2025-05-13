/*
 * Copyright (C)2021-2024 D. R. Commander.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the libjpeg-turbo Project nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS",
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <turbojpeg.h>
#include <stdlib.h>
#include <stdint.h>

#include <stdio.h>  //modify
#include <string.h> //modify
#include <unistd.h> //modify     

#define NUMPF  4

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  tjhandle handle = NULL;
  void *dstBuf = NULL;
  int width = 0, height = 0, precision, sampleSize, pfi;
  enum TJPF pixelFormats[NUMPF] = { TJPF_RGB, TJPF_BGRX, TJPF_GRAY, TJPF_CMYK };
  char tmpFilename[] = "/tmp/decompress2_fuzz_XXXXXX"; //modify
  int fd = -1; //modify

  if ((handle = tj3Init(TJINIT_DECOMPRESS)) == NULL)
    goto bailout;

  tj3DecompressHeader(handle, data, size);
  width = tj3Get(handle, TJPARAM_JPEGWIDTH);
  height = tj3Get(handle, TJPARAM_JPEGHEIGHT);
  precision = tj3Get(handle, TJPARAM_PRECISION);
  sampleSize = (precision > 8 ? 2 : 1);

  if (width < 1 || height < 1 || (uint64_t)width * height > 1048576)
    goto bailout;

  tj3Set(handle, TJPARAM_SCANLIMIT, 500);

  for (pfi = 0; pfi < NUMPF; pfi++) {
    int w = width, h = height;
    int pf = pixelFormats[pfi], i;
    int64_t sum = 0;

    tj3Set(handle, TJPARAM_BOTTOMUP, pfi == 0);
    tj3Set(handle, TJPARAM_FASTUPSAMPLE, pfi == 0);

    if (!tj3Get(handle, TJPARAM_LOSSLESS)) {
      tj3Set(handle, TJPARAM_FASTDCT, pfi == 0);
      if (pfi == 1) {
        tjscalingfactor sf = { 1, 2 };
        tj3SetScalingFactor(handle, sf);
        w = TJSCALED(width, sf);
        h = TJSCALED(height, sf);
      } else {
        tj3SetScalingFactor(handle, TJUNSCALED);
      }

      if (pfi == 3 && w >= 97 && h >= 75) {
        tjregion cr = { 32, 16, 65, 59 };
        tj3SetCroppingRegion(handle, cr);
      } else {
        tj3SetCroppingRegion(handle, TJUNCROPPED);
      }
    }

    if ((dstBuf = tj3Alloc(w * h * tjPixelSize[pf] * sampleSize)) == NULL)
      goto bailout;

    if (precision == 8) {
      if (tj3Decompress8(handle, data, size, (unsigned char *)dstBuf, 0, pf) == 0) {
        for (i = 0; i < w * h * tjPixelSize[pf]; i++)
          sum += ((unsigned char *)dstBuf)[i];

        // modification : Save decompressed image to file
        fd = mkstemp(tmpFilename);
        if (fd >= 0) {
          close(fd);  // We just want the unique name
          tj3SaveImage8(handle, tmpFilename, (unsigned char *)dstBuf, w, 0, h, pf);
          unlink(tmpFilename);  // Clean up file after saving
        }
      } else {
        goto bailout;
      }
    } else if (precision == 12) {
      if (tj3Decompress12(handle, data, size, (short *)dstBuf, 0, pf) == 0) {
        for (i = 0; i < w * h * tjPixelSize[pf]; i++)
          sum += ((short *)dstBuf)[i];

        /* modification : Save decompressed image to file
        fd = mkstemp(tmpFilename);
        if (fd >= 0) {
          close(fd);  // We just want the unique name
          tj3SaveImage12(handle, tmpFilename, (unsigned char *)dstBuf, w, 0, h, pf);
          unlink(tmpFilename);  // Clean up file after saving
        }*/
      } else {
        goto bailout;
      }
    } else {
      if (tj3Decompress16(handle, data, size, (unsigned short *)dstBuf, 0, pf) == 0) {
        for (i = 0; i < w * h * tjPixelSize[pf]; i++)
          sum += ((unsigned short *)dstBuf)[i];
        
        /* modification : Save decompressed image to file
        fd = mkstemp(tmpFilename);
        if (fd >= 0) {
          close(fd);  // We just want the unique name
          tj3SaveImage16(handle, tmpFilename, (unsigned char *)dstBuf, w, 0, h, pf);
          unlink(tmpFilename);  // Clean up file after saving
        }*/
      } else {
        goto bailout;
      }
    }

    free(dstBuf);
    dstBuf = NULL;

    if (sum > ((1LL << precision) - 1LL) * 1048576LL * tjPixelSize[pf])
      goto bailout;
  }

bailout:
  free(dstBuf);
  tj3Destroy(handle);
  return 0;
}
