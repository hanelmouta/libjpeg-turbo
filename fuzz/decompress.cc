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
    if (size < 64) { // Skip inputs smaller than a single 8x8 block
        return 0;
    }

    struct jpeg_compress_struct cinfo;
    struct my_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_compress(&cinfo);
        return 0;
    }

    jpeg_create_compress(&cinfo);

    // Set up a dummy destination manager
    unsigned char dummy_output[1024];
    struct jpeg_destination_mgr dest_mgr;
    dest_mgr.init_destination = [](j_compress_ptr cinfo) {};
    dest_mgr.empty_output_buffer = [](j_compress_ptr cinfo) -> boolean { return TRUE; };
    dest_mgr.term_destination = [](j_compress_ptr cinfo) {};
    dest_mgr.next_output_byte = dummy_output;
    dest_mgr.free_in_buffer = sizeof(dummy_output);
    cinfo.dest = &dest_mgr;

    // Set up compression parameters
    cinfo.image_width = 8;  // Width of one block
    cinfo.image_height = 8; // Height of one block
    cinfo.input_components = 1; // Grayscale
    cinfo.in_color_space = JCS_GRAYSCALE;
    jpeg_set_defaults(&cinfo);

    // Start compression
    jpeg_start_compress(&cinfo, TRUE);

    // Prepare a single 8x8 block of data
    JSAMPLE row[8];
    JSAMPROW row_pointer[1];
    for (int i = 0; i < 8; ++i) {
        row[i] = data[i % size]; // Fill row with fuzzed data
    }
    row_pointer[0] = row;

    // Write the block (indirectly calls `encode_one_block`)
    for (int i = 0; i < 8; ++i) {
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    // Finish compression
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    return 0;
}
