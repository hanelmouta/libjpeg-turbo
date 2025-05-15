
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <csetjmp>
#include "jpeglib.h"

struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

METHODDEF(void) my_error_exit(j_common_ptr cinfo) {
    my_error_mgr* myerr = (my_error_mgr*) cinfo->err;
    longjmp(myerr->setjmp_buffer, 1);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 4) { // Skip small inputs
        return 0;
    }

    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        return 0;
    }

    jpeg_Create_Decompress(&cinfo);
    jpeg_mem_src(&cinfo, data, size);

  
    if (jpeg_read_header(&cinfo, TRUE) == JPEG_HEADER_OK) {
        jpeg_save_markers(&cinfo, JPEG_APP0 + 0xDB, 0xFFFF); // Save marker 0xDB
    }

    jpeg_destroy_decompress(&cinfo);
    return 0;
}
