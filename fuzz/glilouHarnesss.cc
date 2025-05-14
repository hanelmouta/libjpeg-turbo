#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <stdint.h>

struct my_error_mgr {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

METHODDEF(void) my_error_exit(j_common_ptr cinfo) {
  struct my_error_mgr* myerr = (struct my_error_mgr*) cinfo->err;
  longjmp(myerr->setjmp_buffer, 1);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;

  if (size < 10) return 0;

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    return 0;
  }

  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, data, size);

  jpeg_read_header(&cinfo, TRUE);
  jpeg_destroy_decompress(&cinfo);
  return 0;
}
