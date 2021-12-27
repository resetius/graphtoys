#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void usage(char* name) {
    printf("%s text.txt [-o output_name]\n", name);
}

int main(int argc, char** argv) {
    int i;
    char* input = NULL;
    char* output = NULL;
    char* varname = NULL;
    FILE* fin;
    FILE* fout;
    char* dot;
    int ch;
    int count = 0;
    int size = 0;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o")) {
            if (i == argc - 1) {
                usage(argv[0]);
                return -1;
            }
            output = strdup(argv[++i]);
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
    fprintf(stderr, "generating '%s' from '%s'\n", output, input);

    varname = strdup(output);
    if ((dot = strchr(output, '.'))) {
        varname[dot-output] = 0;
    }
    dot = varname;
    while (*dot) {
        if (strchr("/-", *dot)) {
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
            case 0:
                fputs("\000", fout);
            default:
                fputc(ch, fout);
                break;
            }
        } else {
            fprintf(fout, "\\%03hho", ch);
        }
        count = (count+1)%10;
        size ++;
    }
    fprintf(fout, "\";\n");
    fprintf(fout, "static int %s_size = %d;\n", varname, size);

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
