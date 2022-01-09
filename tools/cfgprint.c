#include <stdio.h>
#include <lib/config.h>

int main(int argc, char** argv) {
    struct Config* cfg;
    if (argc < 2) {
        printf("Usage: %s config.ini\n", argv[0]);
        return -1;
    }
    cfg = cfg_new(argv[1], argc, argv);
    cfg_print(cfg);
    cfg_free(cfg);
    return 0;
}
