#include <contrib/astc-codec/include/astc-codec/astc-codec.h>

#include <ktx.h>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <ktxvulkan.h>
#include <lib/verify.h>
#undef NOMINMAX
#include <contrib/ktx/lib/gl_format.h>

using namespace astc_codec;

extern "C" int ASTC_decompress(
    const uint8_t* astc_data, size_t astc_data_size,
    size_t width, size_t height, int footprint,
    uint8_t* out_buffer, size_t out_buffer_size,
    size_t out_buffer_stride)
{
    FootprintType fp_type = static_cast<astc_codec::FootprintType>(footprint);
    return ASTCDecompressToRGBA(astc_data, astc_data_size,
                                width, height, fp_type,
                                out_buffer, out_buffer_size,
                                out_buffer_stride);
}

struct CbData {
    ktxTexture* out;
    FootprintType fp_type;
};

static KTX_error_code iterateCallback(
    int miplevel, int face,
    int width, int height,
    int depth,
    ktx_uint64_t faceLodSize,
    void *pixels, void *userdata
    )
{
    CbData* data = (CbData*)userdata;
    KTX_error_code result = KTX_SUCCESS;
    size_t size = width*height*depth*4;
    uint8_t* img = (uint8_t*)malloc(size);
    size_t stride = width*4;
    verify(ASTCDecompressToRGBA((uint8_t*)pixels, faceLodSize,
                                width, height, data->fp_type,
                                img, size, stride));
    verify(ktxTexture_SetImageFromMemory(data->out, miplevel, 0 /*?*/, face,
                                         img, size) == KTX_SUCCESS);
    free(img);
    return result;
}

static FootprintType get_fp_type(VkFormat format) {
    FootprintType r = FootprintType::k4x4;
    switch (format) {
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: r = FootprintType::k4x4; break;
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: r = FootprintType::k4x4; break;
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: r = FootprintType::k5x4; break;
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: r = FootprintType::k5x4; break;
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: r = FootprintType::k5x5; break;
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: r = FootprintType::k5x5; break;
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: r = FootprintType::k6x5; break;
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: r = FootprintType::k6x5; break;
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: r = FootprintType::k6x6; break;
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: r = FootprintType::k6x6; break;
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: r = FootprintType::k8x5; break;
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: r = FootprintType::k8x5; break;
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: r = FootprintType::k8x6; break;
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: r = FootprintType::k8x6; break;
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: r = FootprintType::k8x8; break;
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: r = FootprintType::k8x8; break;
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: r = FootprintType::k10x5; break;
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: r = FootprintType::k10x5; break;
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: r = FootprintType::k10x6; break;
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: r = FootprintType::k10x6; break;
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: r = FootprintType::k10x8; break;
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: r = FootprintType::k10x8; break;
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: r = FootprintType::k10x10; break;
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: r = FootprintType::k10x10; break;
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: r = FootprintType::k12x10; break;
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: r = FootprintType::k12x10; break;
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: r = FootprintType::k12x12; break;
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: r = FootprintType::k12x12; break;
    default: verify(0); break;
    };
    return r;
}

extern "C" ktxTexture* ktx_ASTC2RGB(ktxTexture* tex) {
    VkFormat vkFormat = ktxTexture_GetVkFormat(tex);
    verify(VK_FORMAT_ASTC_4x4_UNORM_BLOCK <= vkFormat
           && vkFormat <= VK_FORMAT_ASTC_12x12_SRGB_BLOCK);

    ktxTexture1* out;

    ktxTextureCreateInfo info = {
        .glInternalformat = GL_RGBA8,
        .vkFormat = VK_FORMAT_R8G8B8A8_UNORM,
        .baseWidth = tex->baseWidth,
        .baseHeight = tex->baseHeight,
        .baseDepth = tex->baseDepth,
        .numDimensions = tex->numDimensions,
        .numLevels = tex->numLevels,
        .numLayers = tex->numLayers,
        .numFaces = tex->numFaces,
        .isArray = tex->isArray,
        .generateMipmaps = tex->generateMipmaps
    };
    verify(ktxTexture1_Create(&info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &out) == KTX_SUCCESS);

    CbData data = {
        .out = (ktxTexture*)out,
        .fp_type = get_fp_type(vkFormat)
    };
    ktxTexture_IterateLevelFaces(tex, iterateCallback, &data);
    return (ktxTexture*)out;
}
