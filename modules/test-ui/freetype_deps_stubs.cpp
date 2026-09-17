// Satisfies optional FreeType deps (PNG/gzip/bzip2/brotli) that are
// referenced by deps/FreeType2/lib/Windows/freetype.lib but not vendored.
// Outline TTF loading does not call these paths.

#include <cstddef>

extern "C"
{
    void* png_create_read_struct(const char*, void*, void*, void*) { return nullptr; }
    void* png_set_longjmp_fn(void*, void*, size_t) { return nullptr; }
    void* png_create_info_struct(void*) { return nullptr; }
    void png_read_info(void*, void*) {}
    void png_set_expand_gray_1_2_4_to_8(void*) {}
    void png_set_palette_to_rgb(void*) {}
    void png_set_tRNS_to_alpha(void*) {}
    void png_set_gray_to_rgb(void*) {}
    void png_set_filler(void*, unsigned, int) {}
    void png_set_packing(void*) {}
    int png_set_interlace_handling(void*) { return 1; }
    void png_set_strip_16(void*) {}
    void png_read_update_info(void*, void*) {}
    void png_read_image(void*, unsigned char**) {}
    void png_read_end(void*, void*) {}
    void png_destroy_read_struct(void**, void**, void**) {}
    void* png_get_error_ptr(void*) { return nullptr; }
    void png_set_read_fn(void*, void*, void*) {}
    void* png_get_io_ptr(void*) { return nullptr; }
    void png_set_read_user_transform_fn(void*, void*) {}
    void png_error(void*, const char*) {}
    unsigned png_get_valid(void*, void*, unsigned) { return 0; }
    unsigned png_get_IHDR(void*, void*, void*, void*, void*, void*, void*, void*, void*) { return 0; }

    int BrotliDecoderDecompress(size_t, const unsigned char*, size_t*, unsigned char*) { return 0; }

    int inflate(void*, int) { return -5; }
    int inflateEnd(void*) { return 0; }
    int inflateReset(void*) { return -5; }
    int inflateInit2_(void*, int, const char*, int) { return -5; }

    int BZ2_bzDecompressInit(void*, int, int) { return -1; }
    int BZ2_bzDecompress(void*) { return -1; }
    int BZ2_bzDecompressEnd(void*) { return -1; }
}
