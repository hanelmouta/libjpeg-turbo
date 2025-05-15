#include <turbojpeg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size < 100) return 0;  // avoid too small inputs

  tjhandle handle = tj3Init(TJINIT_DECOMPRESS);
  if (!handle) return 0;

  // Parse JPEG header (even if invalid)
  tj3DecompressHeader(handle, data, size);

  // Try to extract ICC profile from the JPEG
  size_t iccSize = 0;
  unsigned char *iccData = tj3GetICCProfile(handle, &iccSize);
  if (iccData && iccSize > 0) {
    volatile unsigned char tmp = iccData[0]; // prevent optimization
  }

  // Set ICC profile from fuzz input
  tj3SetICCProfile(handle, data, size);  // fuzz input as ICC profile

  // Get image params
  int width = tj3Get(handle, TJPARAM_JPEGWIDTH);
  int height = tj3Get(handle, TJPARAM_JPEGHEIGHT);
  int subsamp = tj3Get(handle, TJPARAM_SUBSAMP);

  // Avoid massive or invalid images
  if (width <= 0 || height <= 0 || width * height > 1000000)
    goto done;

  // Manually compute YUV plane sizes using tj3YUVPlaneSize
  size_t ySize = tj3YUVPlaneSize(0, width, 1, height, subsamp);
  size_t uSize = tj3YUVPlaneSize(1, width, 1, height, subsamp);
  size_t vSize = tj3YUVPlaneSize(2, width, 1, height, subsamp);
  size_t totalYUVSize = ySize + uSize + vSize;

  unsigned char *yuvBuf = (unsigned char *)tj3Alloc(totalYUVSize);
  unsigned char *rgbBuf = (unsigned char *)tj3Alloc(width * height * tjPixelSize[TJPF_RGB]);

  if (!yuvBuf || !rgbBuf) goto done;

  // Run through full decompress pipeline
  if (tj3DecompressToYUV8(handle, data, size, yuvBuf, 1) == 0) {
    tj3DecodeYUV8(handle, yuvBuf, 1, rgbBuf, width, 0, height, TJPF_RGB);
  }

done:
  tj3Destroy(handle);
  return 0;
}
