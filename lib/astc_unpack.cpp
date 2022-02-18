#include <contrib/astc-codec/include/astc-codec/astc-codec.h>

extern "C" int ASTC_decompress(
    const uint8_t* astc_data, size_t astc_data_size,
    size_t width, size_t height, int footprint,
    uint8_t* out_buffer, size_t out_buffer_size,
    size_t out_buffer_stride)
{
    astc_codec::FootprintType fp_type = static_cast<astc_codec::FootprintType>(footprint);
    return ASTCDecompressToRGBA(astc_data, astc_data_size,
                                width, height, fp_type,
                                out_buffer, out_buffer_size,
                                out_buffer_stride);
}
