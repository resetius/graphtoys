#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <lib/config.h>

static void get_config_fname(char* out, int size, const char* prog, const char* config) {
    memset(out, 0, size);
    strncpy(out, prog, size-1);
    char* pos = strrchr(out, '/');
    if (pos) {
        pos ++;
    } else {
        pos = out;
    }
    strncpy(pos, config, size-1-(pos-out));
}

static void test_parse(void** state)
{
    char buf[10240];
    get_config_fname(buf, sizeof(buf), (char*)*state, "config1.ini");
    struct Config* cfg = cfg_new(buf, 0, NULL);
    //cfg_print(cfg);
    assert_string_equal(cfg_gets(cfg, "section1:key1"), "value");
    assert_int_equal(cfg_geti(cfg, "section1:key2"), 1234);
    assert_float_equal(cfg_getf_def(cfg, "section1:key3", 0), 1234.567, 1e-15);
    assert_string_equal(cfg_gets_def(cfg, "section2:key0", "value1"), "value1");
    assert_int_equal(cfg_geti(cfg, "section2:key2"), 4321);
    assert_int_equal(cfg_geti_def(cfg, "section2:key0", -1), -1);
    cfg_free(cfg);
}

static void test_rewrite(void** state)
{
    char buf[10240];
    char* argv[] = {"--section1:key1=value2", "--section1:key3=5431.1"};
    get_config_fname(buf, sizeof(buf), (char*)*state, "config1.ini");
    struct Config* cfg = cfg_new(buf, sizeof(argv)/sizeof(argv[0]), argv);
    //cfg_print(cfg);
    assert_string_equal(cfg_gets(cfg, "section1:key1"), "value2");
    assert_float_equal(cfg_getf_def(cfg, "section1:key3", 0), 5431.1, 1e-15);
    cfg_free(cfg);
}

static void test_newkey(void** state)
{
    char buf[10240];
    char* argv[] = {"--section1:key6=value2", "--section1:key7=5431.1"};
    get_config_fname(buf, sizeof(buf), (char*)*state, "config1.ini");
    struct Config* cfg = cfg_new(buf, sizeof(argv)/sizeof(argv[0]), argv);
    //cfg_print(cfg);
    assert_string_equal(cfg_gets(cfg, "section1:key6"), "value2");
    assert_float_equal(cfg_getf_def(cfg, "section1:key7", 0), 5431.1, 1e-15);
    cfg_free(cfg);
}

int main(int argc, char** argv) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_prestate(test_parse, argv[0]),
        cmocka_unit_test_prestate(test_rewrite, argv[0]),
        cmocka_unit_test_prestate(test_newkey, argv[0]),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
