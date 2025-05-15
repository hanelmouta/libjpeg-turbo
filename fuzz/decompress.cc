#include <turbojpeg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  tjhandle handle = NULL;
  unsigned char *dstBufs[1] = { NULL };
  size_t dstSizes[1] = { 0 }, maxBufSize, i;
  int width = 0, height = 0, jpegSubsamp;
  tjtransform transforms[1];

  if ((handle = tj3Init(TJINIT_TRANSFORM)) == NULL)
    goto bailout;

  tj3DecompressHeader(handle, data, size);
  width = tj3Get(handle, TJPARAM_JPEGWIDTH);
  height = tj3Get(handle, TJPARAM_JPEGHEIGHT);
  jpegSubsamp = tj3Get(handle, TJPARAM_SUBSAMP);

  tj3Set(handle, TJPARAM_ARITHMETIC, 0);
  tj3Set(handle, TJPARAM_PROGRESSIVE, 0);
  tj3Set(handle, TJPARAM_OPTIMIZE, 0);

  if (width < 1 || height < 1 || (uint64_t)width * height > 1048576)
    goto bailout;

  tj3Set(handle, TJPARAM_SCANLIMIT, 500);

  if (jpegSubsamp < 0 || jpegSubsamp >= TJ_NUMSAMP)
    jpegSubsamp = TJSAMP_444;

  memset(&transforms[0], 0, sizeof(tjtransform));

  // Set transform parameters
  transforms[0].op = TJXOP_ROT90;
  transforms[0].options = TJXOPT_CROP | TJXOPT_TRIM | TJXOPT_ARITHMETIC;
  transforms[0].r.x = 0;
  transforms[0].r.y = 0;
  transforms[0].r.w = width / 2;
  transforms[0].r.h = height / 2;

  // Estimate maximum buffer size safely using tjBufSize
  maxBufSize = tjBufSize(width, height, jpegSubsamp);
  dstSizes[0] = maxBufSize;

  dstBufs[0] = (unsigned char *)tj3Alloc(maxBufSize);
  if (!dstBufs[0])
    goto bailout;

  tj3Set(handle, TJPARAM_NOREALLOC, 1);
  if (tj3Transform(handle, data, size, 1, dstBufs, dstSizes, transforms) == 0) {
    size_t sum = 0;
    for (i = 0; i < dstSizes[0]; i++)
      sum += dstBufs[0][i];

    if (sum > 255 * maxBufSize)
      goto bailout;
  }

bailout:
  free(dstBufs[0]);
  tj3Destroy(handle);
  return 0;
}
