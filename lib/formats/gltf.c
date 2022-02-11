#include "gltf.h"
#include "base64.h"

#include <ktx.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <contrib/json/json.h>
#include <lib/verify.h>

#define BACK(vec, cap, count) \
    count>=cap ? ( cap=(1+cap)*2, vec = realloc(vec, cap*sizeof(vec[0])), memset(&vec[count], 0, sizeof(vec[count])), &vec[count++] ) : ( memset(&vec[count], 0, sizeof(vec[count])), &vec[count++] )

static void load_scene(struct GltfScene* scene, json_value* value) {
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "name") && entry->value->type == json_string) {
            strncpy(scene->name, entry->value->u.string.ptr, sizeof(scene->name)-1);
        } else if (!strcmp(entry->name, "nodes") && entry->value->type == json_array) {
            for (json_value** node = entry->value->u.array.values;
                 node != entry->value->u.array.values+entry->value->u.array.length; node++)
            {
                scene->nodes[scene->n_nodes++] = (*node)->u.integer;
            }
        } else {
            printf("Unknown scene key: '%s'\n", entry->name);
        }
    }
}

static void load_scenes(struct Gltf* gltf, json_value* value) {
    for (json_value** entry = value->u.array.values;
         entry != value->u.array.values+value->u.array.length; entry++)
    {
        if ((*entry)->type == json_object) {
            load_scene(BACK(gltf->scenes, gltf->cap_scenes, gltf->n_scenes), *entry);
        } else {
            printf("Bad scene type\n");
        }
    }
}

static void load_vec(json_value* value, float* vec, int size) {
    int i;
    for (i = 0; i < size; i++) {
        vec[i] = value->u.array.values[i]->u.dbl;
    }
}

static void load_node(struct GltfNode* node, json_value* value) {
    quat_identity(node->rotation);
    node->scale[0] = node->scale[1] = node->scale[2] = 1.0;
    mat4x4_identity(node->matrix);
    node->camera = node->mesh = -1;
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "mesh") && entry->value->type == json_integer) {
            node->mesh = entry->value->u.integer;
        } else if (!strcmp(entry->name, "name") && entry->value->type == json_string) {
            strncpy(node->name, entry->value->u.string.ptr, sizeof(node->name)-1);
        } else if (!strcmp(entry->name, "rotation") && entry->value->type == json_array && entry->value->u.array.length == 4) {
            load_vec(entry->value, node->rotation, 4);
        } else if (!strcmp(entry->name, "scale") && entry->value->type == json_array && entry->value->u.array.length == 3) {
            load_vec(entry->value, node->scale, 3);
        } else if (!strcmp(entry->name, "translation") && entry->value->type == json_array && entry->value->u.array.length == 3) {
            load_vec(entry->value, node->translation, 3);
        } else if (!strcmp(entry->name, "camera")) {
            node->camera = entry->value->u.integer;
        } else {
            // TODO
            printf("Unknown node key: '%s'\n", entry->name);
        }
    }

    mat4x4_from_quat(node->matrix, node->rotation);
    mat4x4_translate_in_place(node->matrix,
                              node->translation[0],
                              node->translation[1],
                              node->translation[2]);
    mat4x4_scale_aniso(node->matrix, node->matrix,
                       node->scale[0], node->scale[1], node->scale[2]);
    mat4x4 norm_matrix;
    mat4x4_invert(norm_matrix, node->matrix);
    mat4x4_transpose(node->norm_matrix, norm_matrix);
}

static void load_nodes(struct Gltf* gltf, json_value* value) {
    for (json_value** entry = value->u.array.values;
         entry != value->u.array.values+value->u.array.length; entry++)
    {
        if ((*entry)->type == json_object) {
            load_node(BACK(gltf->nodes, gltf->cap_nodes, gltf->n_nodes), *entry);
        } else {
            printf("Bad nodes type\n");
        }
    }
}

static char* load_uri(struct Gltf* gltf, json_value* value, int64_t* size) {
    const char* base64_type = "data:application/octet-stream;base64,";
    int base64_type_len = strlen(base64_type);
    *size = 0;
    if (!strncmp(value->u.string.ptr, base64_type, base64_type_len)) {
        return base64_decode(
            value->u.string.ptr+base64_type_len,
            value->u.string.length-base64_type_len,
            size);
    } else {
        int l = strlen(gltf->fsbase);
        FILE* f;
        char* output;
        strncat(gltf->fsbase, value->u.string.ptr, sizeof(gltf->fsbase)-l-1);
        verify(f = fopen(gltf->fsbase, "rb"));
        fseek(f, 0, SEEK_END);
        *size = ftell(f);
        fseek(f, 0, SEEK_SET);
        output = malloc(*size);
        verify(fread(output, 1, *size, f) == *size);
        fclose(f);
        gltf->fsbase[l] = 0;
        return output;
    }
}

static void load_buffer(struct Gltf* gltf, struct GltfBuffer* buffer, json_value* value) {
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "byteLength") && entry->value->type == json_integer) {
            buffer->size = entry->value->u.integer;
        } else if (!strcmp(entry->name, "uri") && entry->value->type == json_string) {
            // later
        } else {
            printf("Unknown buffer key: '%s'\n", entry->name);
        }
    }

    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "uri") && entry->value->type == json_string) {
            int64_t size;
            buffer->data = load_uri(gltf, entry->value, &size);
            verify(size == buffer->size);
        }
    }
}

static void load_buffers(struct Gltf* gltf, json_value* value) {
    for (json_value** entry = value->u.array.values;
         entry != value->u.array.values+value->u.array.length; entry++)
    {
        if ((*entry)->type == json_object) {
            load_buffer(gltf, BACK(gltf->buffers, gltf->cap_buffers, gltf->n_buffers), *entry);
        } else {
            printf("Bad buffers type\n");
        }
    }
}

static void load_accessor(struct GltfAccessor* acc, json_value* value) {
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "bufferView") && entry->value->type == json_integer) {
            acc->view = entry->value->u.integer;
        } else if (!strcmp(entry->name, "componentType") && entry->value->type == json_integer) {
            acc->component_type = entry->value->u.integer;
        } else if (!strcmp(entry->name, "byteOffset") && entry->value->type == json_integer) {
            acc->offset = entry->value->u.integer;
        } else if (!strcmp(entry->name, "count") && entry->value->type == json_integer) {
            acc->count = entry->value->u.integer;
        } else if (!strcmp(entry->name, "type") && entry->value->type == json_string) {
            if (!strcmp(entry->value->u.string.ptr, "SCALAR")) {
                acc->components = 1;
            } else if (!strcmp(entry->value->u.string.ptr, "VEC2")) {
                acc->components = 2;
            } else if (!strcmp(entry->value->u.string.ptr, "VEC3")) {
                acc->components = 3;
            } else if (!strcmp(entry->value->u.string.ptr, "VEC4")) {
                acc->components = 4;
            } else {
                printf("Unknown type: %s\n", entry->value->u.string.ptr);
            }
        } else if (!strcmp(entry->name, "name") && entry->value->type == json_string) {
            strncpy(acc->name, entry->value->u.string.ptr, sizeof(acc->name)-1);
        } else if (!strcmp(entry->name, "max") && entry->value->type == json_array && entry->value->u.array.length <= 4) {
            load_vec(entry->value, acc->max, entry->value->u.array.length);
        } else if (!strcmp(entry->name, "min") && entry->value->type == json_array && entry->value->u.array.length <= 4) {
            load_vec(entry->value, acc->min, entry->value->u.array.length);
        } else {
            printf("Unknown accessor key: '%s'\n", entry->name);
        }
    }
}

static void load_accessors(struct Gltf* gltf, json_value* value) {
    for (json_value** entry = value->u.array.values;
         entry != value->u.array.values+value->u.array.length; entry++)
    {
        if ((*entry)->type == json_object) {
            load_accessor(BACK(gltf->accessors, gltf->cap_accessors, gltf->n_accessors), *entry);
        } else {
            printf("Unknown accessor type\n");
        }
    }
}

static void load_view(struct GltfView* buf, json_value* value) {
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "buffer") && entry->value->type == json_integer) {
            buf->buffer = entry->value->u.integer;
        } else if (!strcmp(entry->name, "byteLength") && entry->value->type == json_integer) {
            buf->size = entry->value->u.integer;
        } else if (!strcmp(entry->name, "byteOffset") && entry->value->type == json_integer) {
            buf->data = (char*)entry->value->u.integer;
        } else if (!strcmp(entry->name, "byteStride") && entry->value->type == json_integer) {
            buf->stride = entry->value->u.integer;
        } else if (!strcmp(entry->name, "name") && entry->value->type == json_string) {
            strncpy(buf->name, entry->value->u.string.ptr, sizeof(buf->data)-1);
        } else {
            printf("Unknown view key: '%s'\n", entry->name);
        }
    }
}

static void load_views(struct Gltf* gltf, json_value* value) {
    for (json_value** entry = value->u.array.values;
         entry != value->u.array.values+value->u.array.length; entry++)
    {
        if ((*entry)->type == json_object) {
            load_view(BACK(gltf->views, gltf->cap_views, gltf->n_views), *entry);
        } else {
            printf("Unknown views type\n");
        }
    }
}

static void load_attributes(struct GltfPrimitive* primitive, json_value* value) {
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "NORMAL") && entry->value->type == json_integer) {
            primitive->normal = entry->value->u.integer;
        } else if (!strcmp(entry->name, "POSITION") && entry->value->type == json_integer) {
            primitive->position = entry->value->u.integer;
        } else if (!strcmp(entry->name, "TEXCOORD_0") && entry->value->type == json_integer) {
            primitive->texcoord[0] = entry->value->u.integer;
        } else if (!strcmp(entry->name, "TANGENT") && entry->value->type == json_integer) {
            primitive->tangent = entry->value->u.integer;
        } else {
            printf("Unsupported attribute: '%s'\n", entry->name);
        }
    }
}

static void load_primitive(struct GltfPrimitive* primitive, json_value* value) {
    primitive->normal = primitive->position = primitive->tangent = -1;
    for (int i = 0; i < sizeof(primitive->texcoord)/sizeof(int); i++) {
        primitive->texcoord[i] = -1;
    }
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "indices") && entry->value->type == json_integer) {
            primitive->indices = entry->value->u.integer;
        } else if (!strcmp(entry->name, "material") && entry->value->type == json_integer) {
            primitive->material = entry->value->u.integer;
        } else if (!strcmp(entry->name, "attributes") && entry->value->type == json_object) {
            load_attributes(primitive, entry->value);
        } else {
            printf("Unsupported primitive key: '%s'\n", entry->name);
        }
    }
}

static void load_primitives(struct GltfMesh* mesh, json_value* value) {
    for (json_value** entry = value->u.array.values;
         entry != value->u.array.values+value->u.array.length; entry++)
    {
        if ((*entry)->type == json_object) {
            load_primitive(BACK(mesh->primitives, mesh->cap_primitives, mesh->n_primitives), *entry);
        } else {
            printf("Unknown primitives  type\n");
        }
    }
}

static void load_mesh(struct GltfMesh* mesh, json_value* value) {
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "name") && entry->value->type == json_string) {
            strncpy(mesh->name, entry->value->u.string.ptr, sizeof(mesh->name)-1);
        } else if (!strcmp(entry->name, "primitives") && entry->value->type == json_array) {
            load_primitives(mesh, entry->value);
        } else {
            printf("Unsupported mesh key: '%s'\n", entry->name);
        }
    }
}

static void load_meshes(struct Gltf* gltf, json_value* value) {
    for (json_value** entry = value->u.array.values;
         entry != value->u.array.values+value->u.array.length; entry++)
    {
        if ((*entry)->type == json_object) {
            load_mesh(BACK(gltf->meshes, gltf->cap_meshes, gltf->n_meshes), *entry);
        } else {
            printf("Unknown meshes type\n");
        }
    }
}

static void load_perspective(struct GltfCameraPerspective* cam, json_value* value) {
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "aspectRatio")) {
            cam->aspect = entry->value->u.dbl;
        } else if (!strcmp(entry->name, "yfov")) {
            cam->yfov = entry->value->u.dbl;
        } else if (!strcmp(entry->name, "zfar")) {
            cam->zfar = entry->value->u.dbl;
        } else if (!strcmp(entry->name, "znear")) {
            cam->znear = entry->value->u.dbl;
        }
    }
}

static void load_orthographic(struct GltfCameraOrthographic* cam, json_value* value) {
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "xmag")) {
            cam->xmag = entry->value->u.dbl;
        } else if (!strcmp(entry->name, "ymag")) {
            cam->ymag = entry->value->u.dbl;
        } else if (!strcmp(entry->name, "zfar")) {
            cam->zfar = entry->value->u.dbl;
        } else if (!strcmp(entry->name, "znear")) {
            cam->znear = entry->value->u.dbl;
        }
    }
}

static void load_camera(struct GltfCamera* camera, json_value* value) {
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "name") && entry->value->type == json_string) {
            strncpy(camera->name, entry->value->u.string.ptr, sizeof(camera->name)-1);
        } else if (!strcmp(entry->name, "type") && entry->value->type == json_string) {
            if (!strcmp(entry->value->u.string.ptr, "perspective")) {
                camera->is_perspective = 1;
            }
        } else if (!strcmp(entry->name, "perspective") && entry->value->type == json_object) {
            load_perspective(&camera->perspective, entry->value);
        } else if (!strcmp(entry->name, "orthographic") && entry->value->type == json_object) {
            load_orthographic(&camera->orthographic, entry->value);
        }
    }
}

static void load_cameras(struct Gltf* gltf, json_value* value) {
    for (json_value** entry = value->u.array.values;
         entry != value->u.array.values+value->u.array.length; entry++)
    {
        if ((*entry)->type == json_object) {
            load_camera(BACK(gltf->cameras, gltf->cap_cameras, gltf->n_cameras), *entry);
        } else {
            printf("Unknown camera type\n");
        }
    }
}

static void* load_image_uri(struct Gltf* gltf, json_value* value) {
    const char* base64_type = "data:application/octet-stream;base64,";
    int base64_type_len = strlen(base64_type);
    int64_t size;
    ktxTexture* tex = NULL;
    if (!strncmp(value->u.string.ptr, base64_type, base64_type_len)) {
        char* data = base64_decode(
            value->u.string.ptr+base64_type_len,
            value->u.string.length-base64_type_len,
            &size);
        verify(ktxTexture_CreateFromMemory((const ktx_uint8_t *)data, size, 0, &tex) == KTX_SUCCESS);
    } else {
        int l = strlen(gltf->fsbase);
        strncat(gltf->fsbase, value->u.string.ptr, sizeof(gltf->fsbase)-l-1);
        printf("Loading '%s'\n", gltf->fsbase);
        verify(ktxTexture_CreateFromNamedFile(gltf->fsbase, 0, &tex) == KTX_SUCCESS);
        gltf->fsbase[l] = 0;
    }
    return tex;
}

static void load_image(struct Gltf* gltf, struct GltfImage* image, json_value* value) {
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "uri") && entry->value->type == json_string) {
            image->texture = load_image_uri(gltf, entry->value);
        } else if (!strcmp(entry->name, "name") && entry->value->type == json_string) {
            strncat(image->name, value->u.string.ptr, sizeof(image->name)-1);
        } else {
            printf("Unknown image key: '%s'\n", entry->name);
        }
    }
}

static void load_images(struct Gltf* gltf, json_value* value) {
    for (json_value** entry = value->u.array.values;
         entry != value->u.array.values+value->u.array.length; entry++)
    {
        if ((*entry)->type == json_object) {
            load_image(gltf, BACK(gltf->images, gltf->cap_images, gltf->n_images), *entry);
        } else {
            printf("Unknown image type\n");
        }
    }
}

static void load_texture(struct GltfTexture* texture, json_value* value) {
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "source") && entry->value->type == json_integer) {
            texture->source = entry->value->u.integer;
        }
    }
}

static void load_textures(struct Gltf* gltf, json_value* value) {
    for (json_value** entry = value->u.array.values;
         entry != value->u.array.values+value->u.array.length; entry++)
    {
        if ((*entry)->type == json_object) {
            load_texture(BACK(gltf->textures, gltf->cap_textures, gltf->n_textures), *entry);
        } else {
            printf("Unknown texture type\n");
        }
    }
}

void gltf_ctor(struct Gltf* gltf, const char* fn) {
    FILE* f = fopen(fn, "rb");
    char* buf;
    int64_t size;
    memset(gltf, 0, sizeof(*gltf));
    if (!f) {
        printf("Cannot open '%s'\n", fn);
        return;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    buf = malloc(size+1);
    fseek(f, 0, SEEK_SET);
    verify(fread(buf, 1, size, f) == size);
    buf[size] = 0;

    fclose(f);

    strncpy(gltf->fsbase, fn, sizeof(gltf->fsbase)-1);
    char* p = strrchr(gltf->fsbase, '/');
    if (*p) {
        *(p+1) = 0;
    }

    json_value* value = json_parse(buf, size);
    if (value->type != json_object) {
        printf("Bad json format\n");
        goto end;
    }

    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "scene")) {
            // TODO: check integer type
            gltf->def = entry->value->u.integer;
        } else if (!strcmp(entry->name, "scenes") && entry->value->type == json_array) {
            load_scenes(gltf, entry->value);
        } else if (!strcmp(entry->name, "nodes") && entry->value->type == json_array) {
            load_nodes(gltf, entry->value);
        } else if (!strcmp(entry->name, "materials")) {
            // TODO
            printf("Materials unsupported yet\n");
        } else if (!strcmp(entry->name, "extensions")) {
            // TODO
            printf("Extensions unsupported yet\n");
        } else if (!strcmp(entry->name, "buffers") && entry->value->type == json_array) {
            load_buffers(gltf, entry->value);
        } else if (!strcmp(entry->name, "bufferViews") && entry->value->type == json_array) {
            load_views(gltf, entry->value);
        } else if (!strcmp(entry->name, "asset")) {
            // skip
            printf("Asset\n");
        } else if (!strcmp(entry->name, "accessors") && entry->value->type == json_array) {
            load_accessors(gltf, entry->value);
        } else if (!strcmp(entry->name, "meshes") && entry->value->type == json_array) {
            load_meshes(gltf, entry->value);
        } else if (!strcmp(entry->name, "cameras") && entry->value->type == json_array) {
            load_cameras(gltf, entry->value);
        } else if (!strcmp(entry->name, "images") && entry->value->type == json_array) {
            load_images(gltf, entry->value);
        } else if (!strcmp(entry->name, "textures") && entry->value->type == json_array) {
            load_textures(gltf, entry->value);
        } else {
            printf("Unsupported key: '%s'\n", entry->name);
        }
    }

    for (int i = 0; i < gltf->n_views; i++) {
        verify(gltf->views[i].buffer < gltf->n_buffers);
        gltf->views[i].data += (int64_t)gltf->buffers[gltf->views[i].buffer].data;
    }

end:
    json_value_free(value);

    free(buf);
}

void gltf_dtor(struct Gltf* gltf) {
    int i;
    for (i = 0; i < gltf->n_buffers; i++) {
        free(gltf->buffers[i].data);
    }
    for (i = 0; i < gltf->n_meshes; i++) {
        free(gltf->meshes[i].primitives);
    }
    for (i = 0; i < gltf->n_images; i++) {
        ktxTexture_Destroy((ktxTexture*)gltf->images[i].texture);
    }
    free(gltf->accessors);
    free(gltf->scenes);
    free(gltf->nodes);
    free(gltf->meshes);
    free(gltf->views);
    free(gltf->buffers);
    free(gltf->images);
    free(gltf->textures);
}

struct Gltf* gltf_alloc() {
    return calloc(1, sizeof(sizeof(struct Gltf)));
}

void gltf_free(struct Gltf* gltf) {
    free(gltf);
}
