#include <cstddef>
#include <cstdint>
#include <csetjmp>
#include "jpeglib.h"

struct my_error_mgr {
    struct jpeg_error_mgr pub;    // "public" fields
    jmp_buf setjmp_buffer;        // pour gérer erreurs sans exit
};

METHODDEF(void) my_error_exit(j_common_ptr cinfo) {
    my_error_mgr* myerr = (my_error_mgr*) cinfo->err;
    (*cinfo->err->output_message)(cinfo); // affiche message erreur (optionnel)
    longjmp(myerr->setjmp_buffer, 1);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 4)  // Ignore trop petites entrées
        return 0;

    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        // Gestion d'erreur libjpeg : on nettoie et sort proprement
        jpeg_destroy_decompress(&cinfo);
        return 0;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, data, size);

    if (jpeg_read_header(&cinfo, TRUE) == JPEG_HEADER_OK) {
        jpeg_start_decompress(&cinfo);
        jpeg_finish_decompress(&cinfo);
    }

    jpeg_destroy_decompress(&cinfo);
    return 0;
}
