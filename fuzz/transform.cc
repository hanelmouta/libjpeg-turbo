#include <jpeglib.h>   
#include <turbojpeg.h> 
#include <stdint.h>
#include <string.h>
#include <stdio.h>

extern "C" {

// Dummy custom filter for testing
int myCustomFilter(JCOEFPTR coeff_array, tjregion arrayRegion,
                   tjregion planeRegion, int componentIndex,
                   int transformIndex, tjtransform *transform) {
  // Light mutation to ensure memory is accessed
  coeff_array[0] ^= 0xA5;
  return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
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

  transforms[0].op = TJXOP_NONE;
  transforms[0].options = TJXOPT_PROGRESSIVE | TJXOPT_COPYNONE;
  dstSizes[0] = maxBufSize = tj3TransformBufSize(handle, &transforms[0]);
  if (dstSizes[0] == 0 || (dstBufs[0] = (unsigned char *)tj3Alloc(dstSizes[0])) == NULL)
    goto bailout;

  tj3Set(handle, TJPARAM_NOREALLOC, 1);
  if (tj3Transform(handle, data, size, 1, dstBufs, dstSizes, transforms) == 0) {
    size_t sum = 0;
    for (i = 0; i < dstSizes[0]; i++) sum += dstBufs[0][i];
    if (sum > 255 * maxBufSize) goto bailout;
  }

  free(dstBufs[0]);
  dstBufs[0] = NULL;

  transforms[0].r.w = (height + 1) / 2;
  transforms[0].r.h = (width + 1) / 2;
  transforms[0].op = TJXOP_TRANSPOSE;
  transforms[0].options = TJXOPT_GRAY | TJXOPT_CROP | TJXOPT_COPYNONE | TJXOPT_OPTIMIZE;
  dstSizes[0] = maxBufSize = tj3TransformBufSize(handle, &transforms[0]);
  if (dstSizes[0] == 0 || (dstBufs[0] = (unsigned char *)tj3Alloc(dstSizes[0])) == NULL)
    goto bailout;

  if (tj3Transform(handle, data, size, 1, dstBufs, dstSizes, transforms) == 0) {
    size_t sum = 0;
    for (i = 0; i < dstSizes[0]; i++) sum += dstBufs[0][i];
    if (sum > 255 * maxBufSize) goto bailout;
  }

  free(dstBufs[0]);
  dstBufs[0] = NULL;

  transforms[0].op = TJXOP_ROT90;
  transforms[0].options = TJXOPT_TRIM | TJXOPT_ARITHMETIC;
  dstSizes[0] = maxBufSize = tj3TransformBufSize(handle, &transforms[0]);
  if (dstSizes[0] == 0 || (dstBufs[0] = (unsigned char *)tj3Alloc(dstSizes[0])) == NULL)
    goto bailout;

  if (tj3Transform(handle, data, size, 1, dstBufs, dstSizes, transforms) == 0) {
    size_t sum = 0;
    for (i = 0; i < dstSizes[0]; i++) sum += dstBufs[0][i];
    if (sum > 255 * maxBufSize) goto bailout;
  }

  free(dstBufs[0]);
  dstBufs[0] = NULL;

  transforms[0].op = TJXOP_NONE;
  transforms[0].options = TJXOPT_PROGRESSIVE;
  dstSizes[0] = 0;

  tj3Set(handle, TJPARAM_NOREALLOC, 0);
  if (tj3Transform(handle, data, size, 1, dstBufs, dstSizes, transforms) == 0) {
    size_t sum = 0;
    for (i = 0; i < dstSizes[0]; i++) sum += dstBufs[0][i];
    if (sum > 255 * maxBufSize) goto bailout;
  }

  // 💥 Custom filter test block (ensures the condition `if (t[i].customFilter)` is triggered)
  {
    memset(&transforms[0], 0, sizeof(tjtransform));
    transforms[0].op = TJXOP_NONE;
    transforms[0].options = TJXOPT_CROP | TJXOPT_COPYNONE;
    transforms[0].customFilter = myCustomFilter;

    int mcuW = tjMCUWidth[jpegSubsamp];
    int mcuH = tjMCUHeight[jpegSubsamp];
    transforms[0].r.x = 0;
    transforms[0].r.y = 0;
    transforms[0].r.w = (width / mcuW) * mcuW;
    transforms[0].r.h = (height / mcuH) * mcuH;

    dstSizes[0] = maxBufSize = tj3TransformBufSize(handle, &transforms[0]);
    if (dstSizes[0] > 0 && (dstBufs[0] = (unsigned char *)tj3Alloc(dstSizes[0])) != NULL) {
      tj3Set(handle, TJPARAM_NOREALLOC, 1);
      if (tj3Transform(handle, data, size, 1, dstBufs, dstSizes, transforms) == 0) {
        size_t sum = 0;
        for (i = 0; i < dstSizes[0]; i++) sum += dstBufs[0][i];
        if (sum > 255 * maxBufSize) goto bailout;
      }
      free(dstBufs[0]);
    }
  }

bailout:
  free(dstBufs[0]);
  tj3Destroy(handle);
  return 0;
}
}
