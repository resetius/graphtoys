#include "gltf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <contrib/json/json.h>

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

static void load_nodes(struct Gltf* gltf, json_value* value) {

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
    fread(buf, 1, size, f);
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
        }
    }

end:
    json_value_free(value);

    free(buf);
}

void gltf_destroy(struct Gltf* gltf) {

}
