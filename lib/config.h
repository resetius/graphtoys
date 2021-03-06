#pragma once

struct Config;

struct Config* cfg_new(const char* file, int argc, char** argv);
void cfg_free(struct Config*);

struct Config* cfg_section(struct Config*, const char* section);
long cfg_geti(struct Config*, const char* name);
long cfg_geti_def(struct Config*, const char* name, long def);
double cfg_getf_def(struct Config*, const char* name, double def);
const char* cfg_gets(struct Config*, const char* name);
const char* cfg_gets_def(struct Config*, const char* name, const char* def);
void cfg_getv4_def(struct Config*, float out[4], const char* name, const float def[4]);
void cfg_print(struct Config*);
