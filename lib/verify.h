#pragma once

#include <stdio.h>
#include <stdlib.h>

#ifdef verify
#undef verify
#endif

#define verify_1(expr)                              \
    if (!(expr)) {                                  \
        fprintf(                                    \
            stderr,                                 \
            "verify(%s) failed at %s:%d\n"          \
            , #expr                                 \
            , __FILE__, __LINE__);                  \
        abort();                                    \
    }

#define verify_2(expr, opts)                        \
    if (!(expr)) {                                  \
        fprintf(                                    \
            stderr,                                 \
            "%s: verify(%s) failed at %s:%d\n"      \
            , opts                                 \
            , #expr                                 \
            , __FILE__, __LINE__);                  \
        abort();                                    \
    }

#define get_3d_arg(arg1, arg2, arg3, ...) arg3

#define verify_chooser(...)                         \
    get_3d_arg(__VA_ARGS__, verify_2, verify_1, )

#define verify(...) verify_chooser(__VA_ARGS__)(__VA_ARGS__)
