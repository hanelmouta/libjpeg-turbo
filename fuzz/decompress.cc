#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "jpeglib.h"  // Include the libjpeg-turbo header

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Create a memory source manager for libjpeg
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, data, size);

    // Try reading markers
    if (jpeg_read_header(&cinfo, TRUE) == JPEG_HEADER_OK) {
        jpeg_start_decompress(&cinfo);
        jpeg_finish_decompress(&cinfo);
    }

    jpeg_destroy_decompress(&cinfo);
    return 0;
}
