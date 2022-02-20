#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <lib/formats/base64.h>

static void test_dec(const char* str, const char* dst) {
    int64_t size;
    char* out = base64_decode(str, strlen(str), &size);
    assert_string_equal(out, dst);
    assert_int_equal(size, strlen(dst));
    free(out);
}

static void test_enc(const char* str, const char* dst) {
    int64_t size;
    char* out = base64_encode(str, strlen(str), &size);
    assert_string_equal(out, dst);
    free(out);
}

static void test_decoder(void** state) {
    test_dec("dGVzdA==", "test");
    test_dec("bG9uZyBsb25nIGxvbmcgdGV4dCB0ZXN0", "long long long text test");
    test_dec("0YDRg9GB0YHQutC40Lkg0YLQtdC60YHRgg==", "русский текст");
}

static void test_encoder(void** state) {
    test_enc("русский текст", "0YDRg9GB0YHQutC40Lkg0YLQtdC60YHRgg==");
    test_enc("test", "dGVzdA==");
    test_enc("long long long text test", "bG9uZyBsb25nIGxvbmcgdGV4dCB0ZXN0");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_decoder),
        cmocka_unit_test(test_encoder),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
