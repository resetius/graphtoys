#include <stdlib.h>
#include <stdio.h>
#include <ktx.h>
#include <vulkan/vk.h>
#include <ktxvulkan.h>
#include <contrib/astc-codec/include/astc-codec/astc-codec.h>

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
    FILE* fp = fopen(argv[2], "wb");

    result = ktxTexture_CreateFromNamedFile(
        argv[1],
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &texture);

    ktx_uint32_t numLevels = texture->numLevels;
    ktx_uint32_t baseWidth = texture->baseWidth;
    ktx_uint32_t baseHeight = texture->baseHeight;
    ktx_bool_t isArray = texture->isArray;
    ktx_uint32_t format = 0; // ktxTexture_GetVkFormat(texture);

    printf("classId: %d\n", texture->classId);
    printf("isArray: %d\n", texture->isArray);
    printf("isCubemap: %d\n", texture->isCubemap);
    printf("isCompressed: %d\n", texture->isCompressed);
    printf("generateMipmaps: %d\n", texture->generateMipmaps);
    printf("baseWidth: %d\n", texture->baseWidth);
    printf("baseHeight: %d\n", texture->baseHeight);
    printf("baseDepth: %d\n", texture->baseDepth);
    printf("numDimensions: %d\n", texture->numDimensions);
    printf("numLevels: %d\n", texture->numLevels);
    printf("numLayers: %d\n", texture->numLayers);
    printf("numFaces: %d\n", texture->numFaces);
    printf("dataSize: %ld\n", texture->dataSize);
    //texture->glFormat;


    VkFormat vkFormat = ktxTexture_GetVkFormat(texture);
    printf("vkFormat: %d, %x\n", vkFormat, vkFormat);

    printf("levels: %d, width: %d, height: %d, array: %d, format: %d\n",
           numLevels, baseWidth, baseHeight, isArray, format);

    level = 0; layer = 0; faceSlice = 0;
    result = ktxTexture_GetImageOffset(texture, level, layer, faceSlice, &offset);
    image = ktxTexture_GetData(texture) + offset;
    int size =  ktxTexture_GetImageSize(texture, level);

    int width = baseWidth / (1<<level);
    int height = baseHeight / (1<<level);
    int buffer_size = 4*width*height;
    uint8_t* buffer = (uint8_t*)malloc(buffer_size);

    if (ASTCDecompressToRGBA(
        image, size,
        width, height,
        astc_codec::FootprintType::k8x8, // TODO: see vkFormat
        buffer, buffer_size, 4*baseWidth))
    {
        printf("Success!\n");
    }

    int fileChannels = 4;

    uint8_t header[18] = {
        0,0,2,0,0,0,0,0,0,0,0,0,
        (uint8_t)(width%256), (uint8_t)(width/256),
        (uint8_t)(height%256), (uint8_t)(height/256),
        (uint8_t)(fileChannels*8), 0x20
    };
	fwrite(&header, 18, 1, fp);

    //printf("Size: %d %d\n", width*height*fileChannels, size);
    fwrite(buffer, buffer_size, 1, fp);

    ktxTexture_Destroy(texture);
    free(buffer);
    fclose(fp);
    return 0;
}
