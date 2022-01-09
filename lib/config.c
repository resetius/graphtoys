#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include "config.h"

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
    return *s == ';';
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
        exit(1);
    }
    root = calloc(1, sizeof(*root));
    while (fgets(buf, sizeof(buf), f)) {
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
            printf("Bad config syntax\n");
            exit(1);
        }
    }
    fclose(f);
    // replace argc/argv here
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

struct Config* cfg_section(struct Config* root, const char* section) {
    struct Config* node = root->next_section;
    while (node && strcmp(node->section, section)) {
        node = node->next_section;
    }
    return node;
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
    struct Config* node = root->next_key;
    while (node && strcmp(node->key, name)) {
        node = node->next_key;
    }
    return node ? node->value : NULL;
}

const char* cfg_gets(struct Config* root, const char* name) {
    const char* sep = ": \t\n";
    const char* section_name;
    const char* key;
    char* copy;
    struct Config* section;
    if (!root->section) {
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
    free(copy);
    return cfg_get_key(section, key);

noresult:
    free(copy);
    return NULL;
}
