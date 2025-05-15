#include <turbojpeg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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

  // Trigger custom filter region by enabling crop and setting unusual region values
  transforms[0].r.x = 0;
  transforms[0].r.y = 0;
  transforms[0].r.w = (width / 4) * 4;  // Ensure MCU alignment
  transforms[0].r.h = (height / 4) * 4;
  transforms[0].op = TJXOP_NONE;
  transforms[0].options = TJXOPT_CROP | TJXOPT_COPYNONE | TJXOPT_OPTIMIZE | TJXOPT_PROGRESSIVE;
  
  // Allocate enough memory for destination buffer
  dstSizes[0] = maxBufSize = tj3TransformBufSize(handle, &transforms[0]);
  if (dstSizes[0] == 0 ||
      (dstBufs[0] = (unsigned char *)tj3Alloc(dstSizes[0])) == NULL)
    goto bailout;

  // Enable progressive and arithmetic coding to reach respective branches
  tj3Set(handle, TJPARAM_PROGRESSIVE, 1);
  tj3Set(handle, TJPARAM_ARITHMETIC, 1);
  tj3Set(handle, TJPARAM_OPTIMIZE, 1);

  // Execute transform
  if (tj3Transform(handle, data, size, 1, dstBufs, dstSizes, transforms) == 0) {
    size_t sum = 0;
    for (size_t i = 0; i < dstSizes[0]; i++) sum += dstBufs[0][i];
    if (sum > 255 * maxBufSize)
      goto bailout;
  }

bailout:
  free(dstBufs[0]);
  tj3Destroy(handle);
  return 0;
}
