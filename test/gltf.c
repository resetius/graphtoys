#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <lib/formats/gltf.h>

static void get_config_fname(char* out, int size, const char* dir, const char* config) {
    memset(out, 0, size);
    strncpy(out, dir, size-256);
    strcat(out, config);
}

static void test_cube(void** state) {
    char buf[10240];
    get_config_fname(buf, sizeof(buf), (char*)*state, "assets/khr/scenes/cube.gltf");
    struct Gltf gltf;
    gltf_ctor(&gltf, buf);
    assert_int_equal(gltf.n_nodes, 1);
    assert_int_equal(gltf.n_meshes, 1);
    assert_int_equal(gltf.n_materials, 1);
    assert_string_equal(gltf.materials[0].name, "Material");
    assert_int_equal(gltf.n_buffers, 1);
    assert_int_equal(gltf.buffers[0].size, 1188);
    assert_int_equal(gltf.n_accessors, 4);
    gltf_dtor(&gltf);
}

static void test_teapot(void** state) {
    char buf[10240];
    get_config_fname(buf, sizeof(buf), (char*)*state, "assets/khr/scenes/teapot.gltf");
    struct Gltf gltf;
    gltf_ctor(&gltf, buf);
    assert_int_equal(gltf.n_nodes, 1);
    assert_int_equal(gltf.n_meshes, 1);
    assert_int_equal(gltf.n_materials, 1);
    assert_string_equal(gltf.materials[0].name, "Material");
    assert_int_equal(gltf.n_buffers, 1);
    assert_int_equal(gltf.buffers[0].size, 167328);
    assert_int_equal(gltf.n_accessors, 3);
    gltf_dtor(&gltf);
}

static void test_sponza(void** state) {
    char buf[10240];
    get_config_fname(buf, sizeof(buf), (char*)*state, "assets/khr/scenes/sponza/Sponza01.gltf");
    struct Gltf gltf;
    gltf_ctor(&gltf, buf);
    assert_int_equal(gltf.n_nodes, 0x1b);
    assert_int_equal(gltf.n_meshes, 0x19);
    assert_int_equal(gltf.n_materials, 0x19);
    assert_string_equal(gltf.materials[0].name, "flagpole");
    assert_int_equal(gltf.n_buffers, 1);
    assert_int_equal(gltf.buffers[0].size, 0xa37e74);
    assert_int_equal(gltf.n_accessors, 0x7d);
    gltf_dtor(&gltf);
}

int main(int argc, char** argv) {
    char dir[10240];
    if (argc > 1) {
        strncpy(dir, argv[1], sizeof(dir)-1);
        strcat(dir, "/");
    } else {
        char* pos;
        strncpy(dir, argv[0], sizeof(dir)-10);
        pos = strrchr(dir, '/');
        if (pos) {
            *pos = 0;
        }
        strcat(dir, "/../");
    }
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_prestate(test_cube, dir),
        cmocka_unit_test_prestate(test_teapot, dir),
        cmocka_unit_test_prestate(test_sponza, dir)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
