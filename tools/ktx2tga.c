#include <ktx.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s input.ktx output.tga\n", argv[0]);
        return -1;
    }

    ktxTexture* texture;
    KTX_error_code result;
    ktx_size_t offset;
    ktx_uint8_t* image;
    ktx_uint32_t level, layer, faceSlice;

    result = ktxTexture_CreateFromNamedFile(
        argv[1],
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &texture);

    ktx_uint32_t numLevels = texture->numLevels;
    ktx_uint32_t baseWidth = texture->baseWidth;
    ktx_bool_t isArray = texture->isArray;

    printf("levels: %d, width: %d, array: %d\n",
           numLevels, baseWidth, isArray);

    level = 1; layer = 0; faceSlice = 3;
    result = ktxTexture_GetImageOffset(texture, level, layer, faceSlice, &offset);
    image = ktxTexture_GetData(texture) + offset;

    ktxTexture_Destroy(texture);
    return 0;
}
