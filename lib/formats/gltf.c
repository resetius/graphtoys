#include "gltf.h"
#include "base64.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <contrib/json/json.h>
#include <lib/verify.h>

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
        }
    }
}

static void load_scenes(struct Gltf* gltf, json_value* value) {
    for (json_value** entry = value->u.array.values;
         entry != value->u.array.values+value->u.array.length; entry++)
    {
        if ((*entry)->type == json_object) {
            load_scene(&gltf->scenes[gltf->n_scenes ++], *entry);
        }
    }
}

static void load_node(struct GltfNode* node, json_value* value) {
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "mesh") && entry->value->type == json_integer) {
            node->mesh = entry->value->u.integer;
        } else if (!strcmp(entry->name, "name") && entry->value->type == json_string) {
            strncpy(node->name, entry->value->u.string.ptr, sizeof(node->name)-1);
        } else if (!strcmp(entry->name, "rotation") && entry->value->type == json_array && entry->value->u.array.length == 4) {
            node->rotation[0] = entry->value->u.array.values[0]->u.dbl;
            node->rotation[1] = entry->value->u.array.values[1]->u.dbl;
            node->rotation[2] = entry->value->u.array.values[2]->u.dbl;
            node->rotation[3] = entry->value->u.array.values[3]->u.dbl;
        } else {
            // TODO
        }
    }
}

static void load_nodes(struct Gltf* gltf, json_value* value) {
    for (json_value** entry = value->u.array.values;
         entry != value->u.array.values+value->u.array.length; entry++)
    {
        if ((*entry)->type == json_object) {
            load_node(&gltf->nodes[gltf->n_nodes++], *entry);
        }
    }
}

static char* load_uri(json_value* value, int64_t* size) {
    const char* base64_type = "data:application/octet-stream;base64,";
    int base64_type_len = strlen(base64_type);
    *size = 0;
    if (!strncmp(value->u.string.ptr, base64_type, base64_type_len)) {
        return base64_decode(
            value->u.string.ptr+base64_type_len,
            value->u.string.length-base64_type_len,
            size);
    } else {
        return NULL;
    }
}

static void load_buffer(struct GltfBuffer* buffer, json_value* value) {
    for (json_object_entry* entry = value->u.object.values;
         entry != value->u.object.values+value->u.object.length; entry++)
    {
        if (!strcmp(entry->name, "byteLength") && entry->value->type == json_integer) {
            buffer->size = entry->value->u.integer;
        } else if (!strcmp(entry->name, "uri") && entry->value->type == json_string) {
            int64_t size;
            buffer->data = load_uri(entry->value, &size);
            verify(size == buffer->size);
        }
    }
}

static void load_buffers(struct Gltf* gltf, json_value* value) {
    for (json_value** entry = value->u.array.values;
         entry != value->u.array.values+value->u.array.length; entry++)
    {
        if ((*entry)->type == json_object) {
            load_buffer(&gltf->buffers[gltf->n_buffers++], *entry);
        }
    }
}

void gltf_load(struct Gltf* gltf, const char* fn) {
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
        } else if (!strcmp(entry->name, "extensions")) {
            // TODO
        } else if (!strcmp(entry->name, "buffers") && entry->value->type == json_array) {
            load_buffers(gltf, entry->value);
        } else if (!strcmp(entry->name, "bufferViews")) {

        } else if (!strcmp(entry->name, "asset")) {
            // skip
        } else if (!strcmp(entry->name, "accessors")) {
        }
    }

end:
    json_value_free(value);

    free(buf);
}

void gltf_destroy(struct Gltf* gltf) {

}
