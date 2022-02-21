#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void usage(char* name) {
    printf("%s text.txt [-o output_name] [--strip prefix] [-q]\n", name);
}

int main(int argc, char** argv) {
    int i;
    char* input = NULL;
    char* output = NULL;
    char* varname = NULL;
    FILE* fin = NULL;
    FILE* fout = NULL;
    char* dot;
    int ch;
    int count = 0;
    int size = 0;
    char* prefix = NULL;
    int quiet = 0;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o")) {
            if (i == argc - 1) {
                usage(argv[0]);
                return -1;
            }
            output = strdup(argv[++i]);
        } else if (!strcmp(argv[i], "--strip")) {
            if (i == argc - 1) {
                usage(argv[0]);
                return -1;
            }
            prefix = argv[++i];
        } else if (!strcmp(argv[i], "-q")) {
            quiet = 1;
        } else {
            input = strdup(argv[i]);
        }
    }
    if (!input) {
        usage(argv[0]);
        return -1;
    }

    if (!output) {
        output = malloc(strlen(input) + 20);
        strcpy(output, input);
        strcat(output, "_gen.c");
    }
    if (!quiet) {
        fprintf(stderr, "generating '%s' from '%s'\n", output, input);
    }

    char* varstart = output;
    if (prefix && strstr(output, prefix) == output) {
        varstart = &output[strlen(prefix)];
    }
    varname = strdup(varstart);
    if ((dot = strrchr(varstart, '.'))) {
        varname[dot-varstart] = 0;
    }
    dot = varname;
    while (*dot) {
        if (strchr("/-.", *dot)) {
            *dot = '_';
        }
        dot++;
    }

    fin = fopen(input, "rb");
    if (!fin) {
        fprintf(stderr, "cannot open '%s' for reading\n", input);
        goto end;
    }
    fout = fopen(output, "wb");
    if (!fout) {
        fprintf(stderr, "cannot open '%s' for writing\n", output);
        goto end;
    }
    fprintf(fout, "static const char* %s = \"", varname);
    while ((ch = fgetc(fin)) != EOF) {
        if (count == 0) {
            fprintf(fout, "\\\n");
        }

        if (isalnum(ch)) {
            switch (ch) {
            case '"':
                fputc('\\', fout);
                fputc('"', fout);
                break;
            case '\t':
                fputc('\\', fout);
                fputc('t', fout);
                break;
            case '\n':
                fputc('\\', fout);
                fputc('n', fout);
                break;
            default:
                fputc(ch, fout);
                break;
            }
        } else {
            fprintf(fout, "\\%03hho", (char) ch);
        }
        count = (count+1)%10;
        size ++;
    }
    fprintf(fout, "\";\n");
    fprintf(fout, "static int %s_size __attribute__((unused)) = %d;\n", varname, size);

end:
    if (fin) {
        fclose(fin);
    }
    if (fout) {
        fclose(fout);
    }
    free(input);
    free(output);
    free(varname);
    return 0;
}
