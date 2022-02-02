#include "base64.h"

#include <lib/verify.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char enc_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};

static unsigned char dec_table[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,62,0,0,0,63,
    52,53,54,55,56,57,58,59,60,61,0,0,0,0,0,0,
    0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
    15,16,17,18,19,20,21,22,23,24,25,0,0,0,0,0,
    0,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48,49,50,51,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

char* base64_decode(const char* input, int64_t size, int64_t* output_size) {
    uint32_t part = 0;
    int i=3,j;
    char* output = malloc((size + 3) / 4 * 3);
    int64_t len = 0;
    for (j = 0; j < size && input[j] != '='; j++) {
        part |= ((uint32_t)dec_table[(uint8_t)input[j]]) << (i*6);
        if (i == 0) {
            output[len++] = (part >> 2*8) & 0xFF;
            output[len++] = (part >> 1*8) & 0xFF;
            output[len++] = (part >> 0*8) & 0xFF;
            part = 0;
        }
        i = (4+i-1)%4;
    }

    if (i != 3) {
        output[len++] = (part >> 2*8) & 0xFF;
        output[len++] = (part >> 1*8) & 0xFF;
        output[len++] = (part >> 0*8) & 0xFF;
    }

    while (size > 0 && input[size-1] == '=') {
        len--; size --;
    }

    *output_size = len;
    return output;
}

char* base64_encode(const char* input, int64_t size, int64_t* output_size) {
    int i,j=0;
    char* output = malloc(4 * ((size + 2) / 3));
    uint32_t part = 0;
    int len = 0;
    j = 2;
    for (i = 0; i < size; i++) {
        part |= ((uint8_t)input[i]) << (j*8);
        if (j == 0) {
            output[len++] = enc_table[(part >> 3*6) & 0x3F];
            output[len++] = enc_table[(part >> 2*6) & 0x3F];
            output[len++] = enc_table[(part >> 1*6) & 0x3F];
            output[len++] = enc_table[(part >> 0*6) & 0x3F];
            part = 0;
        }
        j = (3+j-1)%3;
    }
    if (j != 2) {
        output[len++] = enc_table[(part >> 3*6) & 0x3F];
        output[len++] = enc_table[(part >> 2*6) & 0x3F];
        output[len++] = enc_table[(part >> 1*6) & 0x3F];
        output[len++] = enc_table[(part >> 0*6) & 0x3F];
    }

    if (size % 3 == 1) {
        output[len - 1] = output[len - 2] = '=';
    } else if (size % 3 == 2) {
        output[len - 1] = '=';
    }

    *output_size = len;
    return output;
}

static void test_dec(const char* str, const char* dst) {
    int64_t size;
    char* out = base64_decode(str, strlen(str), &size);
    printf("'%s'\n", out);
    verify(!strcmp(out, dst));
    verify(size == strlen(dst));
    free(out);
}

static void test_enc(const char* str, const char* dst) {
    int64_t size;
    char* out = base64_encode(str, strlen(str), &size);
    printf("'%s'\n", out);
    verify(!strcmp(out, dst), out);
    free(out);
}

int base64_test(int argc, char** argv) {
    test_dec("dGVzdA==", "test");
    test_dec("bG9uZyBsb25nIGxvbmcgdGV4dCB0ZXN0", "long long long text test");
    test_dec("0YDRg9GB0YHQutC40Lkg0YLQtdC60YHRgg==", "русский текст");

    test_enc("русский текст", "0YDRg9GB0YHQutC40Lkg0YLQtdC60YHRgg==");
    test_enc("test", "dGVzdA==");
    test_enc("long long long text test", "bG9uZyBsb25nIGxvbmcgdGV4dCB0ZXN0");
    return 0;
}

#ifdef BASE64_TEST
int main(int argc, char** argv) {
    return base64_test(argc, argv);
}
#endif
