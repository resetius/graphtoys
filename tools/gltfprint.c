#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <lib/formats/gltf.h>

int main(int argc, char** argv) {
    struct Gltf gltf;

    if (argc < 2) {
        printf("usage: %s filename\n", argv[0]);
        return -1;
    }

    gltf_load(&gltf, argv[1]);

    for (int i = 0; i < gltf.n_scenes; i++) {
        printf("Scene name: '%s'\n", gltf.scenes[i].name);
        for (int j = 0; j < gltf.scenes[i].n_nodes; j++) {
            printf("Node: %d\n", gltf.scenes[i].nodes[j]);
        }
        for (int j = 0; j < gltf.n_buffers; j++) {
            printf("Buffer: %d\n", gltf.buffers[j].size);
        }
    }

    gltf_destroy(&gltf);
    return 0;
}
