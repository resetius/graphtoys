#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include "config.h"

static struct Config* cfg_key_node(struct Config* root, const char* name);

struct Config {
    char* section;
    char* key;
    char* value;

    struct Config* next_key;
    struct Config* next_section;
};

static int is_comment(const char* s) {
    while (*s && isspace(*s)) {
        s++;
    }
    return *s == ';' || *s == 0;
}

static const char* get_section(char* s) {
    const char* ret;
    while (*s && isspace(*s)) {
        s++;
    }
    if (*s != '[') {
        return NULL;
    }
    s++;
    while (*s && isspace(*s)) {
        s++;
    }
    if (!*s) {
        return NULL;
    }
    ret = s;
    while (*s && !isspace(*s) && *s != ']') {
        s++;
    }
    if (!*s) {
        return NULL;
    }
    *s = 0;

    return ret;
}

static int get_key_value(char* s, char const** k, char const** v) {
    const char* sep = "; =\n\t";
    const char* key;
    const char* val;
    if (!(key = strtok(s, sep))) {
        return 0;
    }
    if (!(val = strtok(NULL, sep))) {
        return 0;
    }
    *k = key;
    *v = val;
    return 1;
}

static void cfg_rewrite(struct Config* root, int argc, char** argv) {
    int i;
    const char* colon, *eq;
    char* sect_name = NULL, *key = NULL, *val = NULL;
    struct Config* sect, *key_node;
    for (i = 0; i < argc; i++) {
        if (strstr(argv[i], "--") == argv[i] && (colon = strchr(argv[i], ':'))) {
            if ((eq = strchr(argv[i], '=')) && (eq > colon)) {
                sect_name = strdup(argv[i]+2);
                sect_name[colon - argv[i] - 2] = 0;
                key = strdup(colon + 1);
                key[eq - colon - 1] = 0;
                val = strdup(eq+1);

                sect = cfg_section(root, sect_name);
                if (!sect) {
                    sect = calloc(1, sizeof(*sect));
                    sect->section = sect_name; sect_name = NULL;
                    sect->next_section = root->next_section;
                    root->next_section = sect;
                }
                key_node = cfg_key_node(sect, key);
                if (!key_node) {
                    key_node = calloc(1, sizeof(*key_node));
                    key_node->next_key = sect->next_key;
                    sect->next_key = key_node;
                }
                free(key_node->key); key_node->key = key; key = NULL;
                free(key_node->value); key_node->value = val; val = NULL;
            }
        }

        free(sect_name); free(key); free(val);
        sect_name = key = val = NULL;
    }
}

struct Config* cfg_new(const char* file, int argc, char** argv) {
    FILE* f = fopen(file, "rb");
    char buf[32768];
    struct Config* root;
    struct Config* section = NULL;
    struct Config* kv = NULL;
    const char* k, * v;
    const char* section_name;
    if (!f) {
        printf("Cannot open config '%s'\n", file);
    }
    root = calloc(1, sizeof(*root));
    while (f && fgets(buf, sizeof(buf), f)) {
        if (is_comment(buf)) {
            continue;
        }
        if ((section_name = get_section(buf))) {
            section = calloc(1, sizeof(*section));
            section->section = strdup(section_name);
            section->next_section = root->next_section;
            root->next_section = section;
        } else if (get_key_value(buf, &k, &v) && section) {
            kv = calloc(1, sizeof(*kv));
            kv->key = strdup(k);
            kv->value = strdup(v);
            kv->next_key = section->next_key;
            section->next_key = kv;
        } else {
            printf("Bad config syntax near '%s'\n", buf);
            exit(1);
        }
    }
    if (f) {
        fclose(f);
    }

    cfg_rewrite(root, argc, argv);

    return root;
}

void cfg_free(struct Config* c) {
    if (c) {
        free(c->section);
        free(c->key);
        free(c->value);
        cfg_free(c->next_key);
        cfg_free(c->next_section);
        free(c);
    }
}

void cfg_print(struct Config* c) {
    if (c) {
        if (c->section) {
            printf("['%s']\n", c->section);
        }
        if (c->key) {
            printf("'%s' = '%s'\n", c->key, c->value);
        }
        cfg_print(c->next_key);
        cfg_print(c->next_section);
    }
}

static struct Config* cfg_key_node(struct Config* root, const char* name) {
    struct Config* node = root ? root->next_key : NULL;
    while (node && strcmp(node->key, name)) {
        node = node->next_key;
    }
    return node;
}

struct Config* cfg_section(struct Config* root, const char* section) {
    struct Config* node = root->next_section;
    struct Config* prev = NULL;
    struct Config* sect = NULL;
    while (node) {
        struct Config* next = node->next_section;
        if (!strcmp(node->section, section)) {
            if (sect == NULL) {
                prev = node;
                sect = node;
            } else {
                // merge
                struct Config* last = sect->next_key;
                if (!last) {
                    printf("1\n");
                    sect->next_key = node->next_key;
                } else {
                    while (last->next_key) {
                        last = last->next_key;
                    }
                    last->next_key = node->next_key;
                }
                prev->next_section = node->next_section;
                free(node->section);
                free(node);
            }
        } else {
            prev = node;
        }
        node = next;
    }
    return sect;
}

long cfg_geti(struct Config* root, const char* name) {
    const char* s = cfg_gets(root, name);
    char* end;
    long res;
    if (!s) {
        return LONG_MIN;
    }
    res = strtol(s, &end, 10);
    if (s == end) {
        return LONG_MIN;
    }
    return res;
}

static const char* cfg_get_key(struct Config* root, const char* name) {
    struct Config* node = cfg_key_node(root, name);
    return node ? node->value : NULL;
}

const char* cfg_gets(struct Config* root, const char* name) {
    const char* sep = ": \t\n";
    const char* section_name;
    const char* key;
    char* copy;
    const char* result = NULL;
    struct Config* section;
    if (root->section) {
        return cfg_get_key(root, name);
    }

    copy = strdup(name);
    if (!(section_name = strtok(copy, sep))) {
        goto noresult;
    }
    if (!(key = strtok(NULL, sep))) {
        goto noresult;
    }
    if (!(section = cfg_section(root, section_name))) {
        goto noresult;
    }
    result = cfg_get_key(section, key);

noresult:
    free(copy);
    return result;
}

const char* cfg_gets_def(struct Config* root, const char* name, const char* def) {
    const char* r = root ? cfg_gets(root, name) : NULL;
    return r ? r : def;
}

long cfg_geti_def(struct Config* root, const char* name, long def) {
    if (!root) {
        return def;
    }
    long r = cfg_geti(root, name);
    return r == LONG_MIN ? def : r;
}

double cfg_getf_def(struct Config* root, const char* name, double def) {
    char buf[1024];
    snprintf(buf, sizeof(buf), "%.16le", def);
    const char* str = cfg_gets_def(root, name, buf);
    double ret;
    sscanf(str, "%lf", &ret);
    return ret;
}

void cfg_getv4_def(struct Config* c, float out[4], const char* name, const float def[4])
{
    char buf[1024];
    snprintf(buf, sizeof(buf)-1, "%.8f,%.8f,%.8f,%.8f", def[0], def[1], def[2], def[3]);
    char* str = strdup(cfg_gets_def(c, name, buf));
    const char* sep = ",";
    int i = 0;
    for (char* tok = strtok(str, sep); tok; tok = strtok(NULL, sep)) {
        float num = atof(tok);
        out[i++] = num;
    }
    free(str);
}
