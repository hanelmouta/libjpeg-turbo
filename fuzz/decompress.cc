#include <csetjmp>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "jpeglib.h"

struct my_error_mgr {
    struct jpeg_error_mgr pub;    // champs "public" de libjpeg
    jmp_buf setjmp_buffer;        // buffer pour gérer erreurs sans exit
};

METHODDEF(void) my_error_exit(j_common_ptr cinfo) {
    my_error_mgr* myerr = (my_error_mgr*) cinfo->err;
    // Affiche le message d'erreur (optionnel)
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Ignorer les entrées trop petites ou trop grandes
    if (size < 4 || size > 1024 * 1024)
        return 0;

    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;

    // Setup gestion d'erreur personnalisée
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    // Si erreur lors de la décompression, on retourne proprement
    if (setjmp(jerr.setjmp_buffer)) {
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
