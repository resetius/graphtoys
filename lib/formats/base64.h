#pragma once

#include <stdint.h>

char* base64_decode(const char* input, int64_t size, int64_t* output_size);
char* base64_encode(const char* input, int64_t size, int64_t* output_size);

int base64_test(int argc, char** argv);
