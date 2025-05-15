#include "jchuff.h" 
#include <cstring> 
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <csetjmp>
#include "jpeglib.h"
#include "jchuff.h" // Ensure this header provides the declaration for encode_one_block

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < sizeof(JBLOCK)) {
        return 0; // Skip inputs smaller than a single block
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    // Mock compression parameters
    cinfo.image_width = 8;
    cinfo.image_height = 8;
    cinfo.input_components = 1;
    cinfo.in_color_space = JCS_GRAYSCALE;
    jpeg_set_defaults(&cinfo);

    // Prepare a single block of data
    JBLOCK block;
    memcpy(block, data, sizeof(JBLOCK));

    // Mock Huffman tables
    c_derived_tbl dctbl, actbl;
    memset(&dctbl, 0, sizeof(dctbl));
    memset(&actbl, 0, sizeof(actbl));

    // Call encode_one_block directly
    encode_one_block(&cinfo, block, 0, &dctbl, &actbl);

    jpeg_destroy_compress(&cinfo);
    return 0;
}
