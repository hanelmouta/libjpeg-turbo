/*
 * Copyright (C)2021-2025 D. R. Commander.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 */
extern "C" {
#include <jpeglib.h>
#include <jtransform.h>
}

#include <turbojpeg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// ✅ Custom filter function
int myCustomFilter(JCOEFPTR coeff_array, tjregion arrayRegion,
                   tjregion planeRegion, int componentIndex, int transformIndex,
                   tjtransform *transform)
{
  // Dummy read/write to exercise the code path
  coeff_array[0] = coeff_array[0];
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  tjhandle handle = NULL;
  unsigned char *dstBufs[1] = { NULL };
  size_t dstSizes[1] = { 0 }, maxBufSize;
  int width = 0, height = 0, jpegSubsamp;
  tjtransform transforms[1];

  if ((handle = tj3Init(TJINIT_TRANSFORM)) == NULL)
    return 0;

  tj3DecompressHeader(handle, data, size);
  width = tj3Get(handle, TJPARAM_JPEGWIDTH);
  height = tj3Get(handle, TJPARAM_JPEGHEIGHT);
  jpegSubsamp = tj3Get(handle, TJPARAM_SUBSAMP);

  if (width < 1 || height < 1 || (uint64_t)width * height > 1048576)
    goto bailout;

  tj3Set(handle, TJPARAM_SCANLIMIT, 500);

  if (jpegSubsamp < 0 || jpegSubsamp >= TJ_NUMSAMP)
    jpegSubsamp = TJSAMP_444;

  memset(&transforms[0], 0, sizeof(tjtransform));

  // ✅ Trigger customFilter path
  transforms[0].r.x = 0;
  transforms[0].r.y = 0;
  transforms[0].r.w = (width / 4) * 4;  // MCU-aligned
  transforms[0].r.h = (height / 4) * 4;
  transforms[0].op = TJXOP_NONE;
  transforms[0].options = TJXOPT_CROP | TJXOPT_COPYNONE | TJXOPT_OPTIMIZE | TJXOPT_PROGRESSIVE;

  // ✅ Set the custom filter
  transforms[0].customFilter = myCustomFilter;

  // Allocate destination buffer
  dstSizes[0] = maxBufSize = tj3TransformBufSize(handle, &transforms[0]);
  if (dstSizes[0] == 0 ||
      (dstBufs[0] = (unsigned char *)tj3Alloc(dstSizes[0])) == NULL)
    goto bailout;

  tj3Set(handle, TJPARAM_PROGRESSIVE, 1);
  tj3Set(handle, TJPARAM_ARITHMETIC, 1);
  tj3Set(handle, TJPARAM_OPTIMIZE, 1);

  if (tj3Transform(handle, data, size, 1, dstBufs, dstSizes, transforms) == 0) {
    size_t sum = 0;
    for (size_t i = 0; i < dstSizes[0]; i++)
      sum += dstBufs[0][i];

    if (sum > 255 * maxBufSize)
      goto bailout;
  }

bailout:
  free(dstBufs[0]);
  tj3Destroy(handle);
  return 0;
}
