#pragma once

struct ParticlesData {
    float* coords;
    float* vels;
    float* accel;
    float* color;
    int* indices;
    int n_particles;
};

struct Config;
void particles_data_init(struct ParticlesData* data, struct Config* cfg);
void particles_data_destroy(struct ParticlesData* data);
