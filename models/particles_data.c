#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <lib/config.h>
#include "particles_data.h"

struct body
{
    double x[3];
    double v[3];
    double mass;
    int fixed;
    char name[100];
};

void particles_data_init(struct ParticlesData* data, struct Config* cfg) {
    int n_x = cfg_geti_def(cfg, "nx", 23);
    int n_y = cfg_geti_def(cfg, "ny", 23);
    int n_z = cfg_geti_def(cfg, "nz", 23);
    int n_particles = n_x*n_y*n_z;
    int size = n_particles*4*sizeof(float);
    float* coords = malloc(size);
    int* indices = malloc(n_particles*sizeof(int));
    float* vels = calloc(1, size);
    float* accel = calloc(1, size);
    int i,j,k,n=0;
    //float side = 4.0f;
    float side = cfg_getf_def(cfg, "side", 7.5f);
    float dx = side/(n_x-1);
    float dy = side/(n_y-1);
    float dz = side/(n_z-1);

    srand(time(NULL));

    const double G = 2.92e-6;
    const char* name = cfg_gets_def(cfg, "name", "solar");

    if (!strcmp(name, "sunearth")) {

        struct body body[] = {
            {{0, 0, 0}, {0, 0, 0}, 10000, 1, "Sun"},
            {{0, 1, 0}, {-1, 0, 0}, 1, 0, "Earth"},
        };
        const int nbodies = 2;

        for (i = 0; i < nbodies; i++) {
            coords[n] = body[i].x[0];
            coords[n+1] = body[i].x[1];
            coords[n+2] = body[i].x[2];
            coords[n+3] = G * body[i].mass;

            vels[n] = body[i].v[0];
            vels[n+1] = body[i].v[1];
            vels[n+2] = body[i].v[2];
            vels[n+3] = 0;

            n += 4;
        }
    } else if (!strcmp(name, "solar")) {

        struct body body[] = {
            {{0, 0, 0}, {0, 0, 0}, 333333, 1, "Sun"},
            {{0, 0.39, 0}, {1.58, 0, 0}, 0.038, 0, "Mercury"},
            {{0, 0.72, 0}, {1.17, 0, 0}, 0.82, 0, "Venus"},
            {{0, 1, 0}, {1, 0, 0}, 1, 0, "Earth"},
            {{0, 1.00256, 0}, {1.03, 0, 0}, 0.012, 0, "Moon"},
            {{0, 1.51, 0}, {0.8, 0, 0}, 0.1, 0, "Mars"},
            {{0, 5.2, 0}, {0.43, 0, 0}, 317, 0, "Jupiter"},
            {{0, 9.3, 0}, {0.32, 0, 0}, 95, 0, "Saturn"},
            {{0, 19.3, 0}, {0.23, 0, 0}, 14.5, 0, "Uranus"},
            {{0, 30, 0}, {0.18, 0, 0}, 16.7, 0, "Neptune"}};

        const int nbodies = 10;

        for (i = 0; i < nbodies; i++) {
            coords[n] = body[i].x[0];
            coords[n+1] = body[i].x[1];
            coords[n+2] = body[i].x[2];
            coords[n+3] = G * body[i].mass;

            vels[n] = body[i].v[0];
            vels[n+1] = body[i].v[1];
            vels[n+2] = body[i].v[2];
            vels[n+3] = 0;

            n += 4;
        }
    } else if (!strcmp(name, "cube")) {

        for (i = 0; i < n_x; i++) {
            for (j = 0; j < n_y; j++) {
                for (k = 0; k < n_z; k++) {
                    coords[n] = dx * i - side/2 + (0.5 * (double)rand() / (double)RAND_MAX - 0.25);
                    coords[n+1] = dy * j - side/2 + (0.5 * (double)rand() / (double)RAND_MAX - 0.25);
                    coords[n+2] = dz * k - side/2 + (0.5 * (double)rand() / (double)RAND_MAX - 0.25);
                    coords[n+3] = 0.2 + 1.5*(double)rand() / (double)RAND_MAX;

                    double R =
                        coords[n]*coords[n]+
                        coords[n+1]*coords[n+1]+
                        coords[n+2]*coords[n+2];
                    R = sqrt(R);
                    double V = sqrt(1000)/sqrt(R); // sqrt(100/R);

                    vels[n] = V*coords[n+1];
                    vels[n+1] = -V*coords[n];
                    vels[n+2] = 0;

                    //vels[n] = rand() / (double) RAND_MAX;
                    //vels[n+1] = rand() / (double) RAND_MAX;
                    //vels[n+2] = rand() / (double) RAND_MAX;
                    vels[n+3] = 0;
                    n += 4;
                }
            }
        }
    } else { // sphere

        while (n < 4*n_x*n_y*n_z) {
            double x = 2*side*(double)rand() / RAND_MAX - side;
            double y = 2*side*(double)rand() / RAND_MAX - side;
            double z = 2*side*(double)rand() / RAND_MAX - side;

            if (x*x+y*y+z*z<side*side) {
                coords[n] = x;
                coords[n+1] = y;
                coords[n+2] = z;
                coords[n+3] = 0.2 + 1.5*(double)rand() / (double)RAND_MAX;

                double R =
                    coords[n]*coords[n]+
                    coords[n+1]*coords[n+1]+
                    coords[n+2]*coords[n+2];
                R = sqrt(R);
                double V = sqrt(200)/sqrt(R); // sqrt(100/R);

                vels[n] = V*coords[n+1];
                vels[n+1] = -V*coords[n];
                vels[n+2] = 0;

                vels[n+3] = 0;
                n += 4;
            }
        }

    }

    data->n_particles = n/4;

    for (i = 0; i < data->n_particles; i++) {
        indices[i] = i;
    }

    data->vels = vels;
    data->coords = coords;
    data->accel = accel;
    data->indices = indices;
}

void particles_data_destroy(struct ParticlesData* data) {
    free(data->coords);
    free(data->vels);
    free(data->accel);
}
