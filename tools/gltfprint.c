#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <lib/formats/gltf.h>

int main(int argc, char** argv) {
    struct Gltf* gltf;

    if (argc < 2) {
        printf("usage: %s filename\n", argv[0]);
        return -1;
    }

    gltf = gltf_load(argv[1]);

    for (int i = 0; i < gltf->n_scenes; i++) {
        printf("Scene name: '%s'\n", gltf->scenes[i].name);
        for (int j = 0; j < gltf->scenes[i].n_nodes; j++) {
            printf("Node: %d\n", gltf->scenes[i].nodes[j]);
        }
        for (int j = 0; j < gltf->n_buffers; j++) {
            printf("Buffer: %d %p %ld\n", j, gltf->buffers[j].data, (long)gltf->buffers[j].size);
        }
        for (int j = 0; j < gltf->n_views; j++) {
            printf("View: %d %d %p %ld\n", j, gltf->views[j].buffer, gltf->views[j].data, (long)gltf->views[j].size);
        }
        for (int j = 0; j < gltf->n_nodes; j++) {
            printf("Node: %d '%s'\n", j, gltf->nodes[j].name);
        }
        for (int j = 0; j < gltf->n_accessors; j++) {
            printf("Accessor: %d %d %d\n", j, gltf->accessors[j].view, gltf->accessors[j].count);
        }
    }
    printf("Meshes: %d\n", gltf->n_meshes);

    gltf_destroy(gltf);
    return 0;
}
