#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <lib/config.h>
#include <lib/verify.h>
#include "particles_data.h"

struct body
{
    double x[3];
    double v[3];
    double c[3];
    double mass;
    int fixed;
    char name[100];
};

static uint32_t rand_(uint32_t* seed) {
    *seed ^= *seed << 13;
    *seed ^= *seed >> 17;
    *seed ^= *seed << 5;
    return *seed;
}

static uint32_t rand_max = UINT_MAX;

void particles_data_init(struct ParticlesData* data, struct Config* cfg) {
    int n_particles = cfg_geti_def(cfg, "particles", 12000);
    double V0 = cfg_getf_def(cfg, "V", sqrt(200));

    int size = n_particles*4*sizeof(float);
    float* coords = malloc(size);
    int* indices = malloc(n_particles*sizeof(int));
    float* vels = calloc(1, size);
    float* accel = calloc(1, size);
    int i,j,n=0;
    //float side = 4.0f;
    float side = cfg_getf_def(cfg, "side", 7.5f);
    uint32_t seed = cfg_geti_def(cfg, "seed", -1);
    memset(data, 0, sizeof(*data));
    if (seed == (uint32_t)-1) {
        seed = (uint32_t) time(NULL);
    }

    const double G = 2.92e-6;
    const char* name = cfg_gets_def(cfg, "name", "solar");
    const char* ics_file = cfg_gets_def(cfg, "ics_file", "");
    int ics_components = cfg_geti_def(cfg, "ics_components", 1);

    if (!strcmp(name, "sunearth")) {

        struct body body[] = {
            {{0, 0, 0}, {0, 0, 0}, {}, 10000, 1, "Sun"},
            {{0, 1, 0}, {-1, 0, 0}, {}, 1, 0, "Earth"},
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
            {{0, 0, 0}, {0, 0, 0}, {}, 333333, 1, "Sun"},
            {{0, 0.39, 0}, {1.58, 0, 0}, {}, 0.038, 0, "Mercury"},
            {{0, 0.72, 0}, {1.17, 0, 0}, {}, 0.82, 0, "Venus"},
            {{0, 1, 0}, {1, 0, 0}, {}, 1, 0, "Earth"},
            {{0, 1.00256, 0}, {1.03, 0, 0}, {}, 0.012, 0, "Moon"},
            {{0, 1.51, 0}, {0.8, 0, 0}, {}, 0.1, 0, "Mars"},
            {{0, 5.2, 0}, {0.43, 0, 0}, {}, 317, 0, "Jupiter"},
            {{0, 9.3, 0}, {0.32, 0, 0}, {}, 95, 0, "Saturn"},
            {{0, 19.3, 0}, {0.23, 0, 0}, {}, 14.5, 0, "Uranus"},
            {{0, 30, 0}, {0.18, 0, 0}, {}, 16.7, 0, "Neptune"}};

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
    } else if (!strcmp(name, "solar_lite")) {
        struct body body[] = {
            {{0, 0, 0}, {0, 0, 0}, {1,0,0}, 333333, 1, "Sun"},
            {{0, 0.39, 0}, {1.58, 0, 0}, {0,1,0}, 0.038, 0, "Mercury"},
            {{0, 0.72, 0}, {1.17, 0, 0}, {0,0,1}, 0.82, 0, "Venus"},
            {{0, 1, 0}, {1, 0, 0}, {1,0,1}, 1, 0, "Earth"},
            {{0, 1.51, 0}, {0.8, 0, 0}, {0,1,1}, 0.1, 0, "Mars"}};

        const int nbodies = 5;

        data->color = calloc(4*nbodies,sizeof(float));

        for (i = 0; i < nbodies; i++) {
            coords[n] = body[i].x[0];
            coords[n+1] = body[i].x[1];
            coords[n+2] = body[i].x[2];
            coords[n+3] = G * body[i].mass;

            vels[n] = body[i].v[0];
            vels[n+1] = body[i].v[1];
            vels[n+2] = body[i].v[2];
            vels[n+3] = 0;

            data->color[n+0] = body[i].c[0];
            data->color[n+1] = body[i].c[1];
            data->color[n+2] = body[i].c[2];
            data->color[n+3] = 1;

            n += 4;
        }
    } else if (!strcmp(name, "cube")) {

        for (i = 0; i < n_particles; i++) {
            for (j = 0; j < 3; j++) {
                coords[n+j] = side * (double)rand_(&seed) / (double)rand_max - side/2;
            }

            coords[n+3] = 0.2 + 1.5*(double)rand_(&seed) / (double)rand_max;

            double R =
                coords[n]*coords[n]+
                coords[n+1]*coords[n+1]+
                coords[n+2]*coords[n+2];
            R = sqrt(R);
            double V = V0/sqrt(R); // sqrt(100/R);

            vels[n] = V*coords[n+1];
            vels[n+1] = -V*coords[n];
            vels[n+2] = 0;

            //vels[n] = rand() / (double) RAND_MAX;
            //vels[n+1] = rand() / (double) RAND_MAX;
            //vels[n+2] = rand() / (double) RAND_MAX;
            vels[n+3] = 0;
            n += 4;
        }
    } else if (!strcmp(name, "square")) {
        for (i = 0; i < n_particles; i++) {
            for (j = 0; j < 2; j++) {
                coords[n+j] = side * (double)rand_(&seed) / (double)rand_max - side/2;
            }
            coords[n+2] = 0;

            coords[n+3] = 0.2 + 1.5*(double)rand_(&seed) / (double)rand_max;

            double R =
                coords[n]*coords[n]+
                coords[n+1]*coords[n+1]+
                coords[n+2]*coords[n+2];
            R = sqrt(R);
            double V = V0/sqrt(R); // sqrt(100/R);

            vels[n] = V*coords[n+1];
            vels[n+1] = -V*coords[n];
            vels[n+2] = 0;

            //vels[n] = rand() / (double) RAND_MAX;
            //vels[n+1] = rand() / (double) RAND_MAX;
            //vels[n+2] = rand() / (double) RAND_MAX;
            vels[n+3] = 0;
            n += 4;
        }
    } else if (!strcmp(name, "ics")) {
        char filename[1024];
        double minx = 1e15;
        double maxx = -1e15;
        double miny = 1e15;
        double maxy = -1e15;
        double minz = 1e15;
        double maxz = -1e15;

        for (int comp = 0; comp < ics_components; comp++) {
            sprintf(filename, "%s.%d", ics_file, comp);
            FILE* f = fopen(filename, "rb");
            if (!f) {
                printf("Cannot open file: %s\n", filename); exit(1);
            }
            int k; double t;
            verify(fscanf(f, "%d%lf", &k, &t) == 2);
            for (int i = 0; i < k; i++) {
                double m,x,y,z,vx,vy,vz;
                verify(fscanf(f, "%lf%lf%lf%lf%lf%lf%lf", &m, &x, &y, &z, &vx, &vy, &vz) == 7);
                coords[n] = x;
                coords[n+1] = y;
                coords[n+2] = z;
                coords[n+3] = m;


                vels[n] = vx;
                vels[n+1] = vy;
                vels[n+2] = vz;
                vels[n+3] = 0;


                double R =
                coords[n]*coords[n]+
                coords[n+1]*coords[n+1];
                R = sqrt(R);

                //double V = V0*R;
                //double V = V0;
                //vels[n] = V*coords[n+1]/R;
                //vels[n+1] = -V*coords[n]/R;

                minx = fmin(minx, x);
                miny = fmin(miny, y);
                minz = fmin(minz, z);

                maxx = fmax(maxx, x);
                maxy = fmax(maxy, y);
                maxz = fmax(maxz, z);

                n += 4;
            }
            fclose(f);
        }

        printf("%lf %lf, %lf %lf, %lf %lf\n", minx, maxx, miny, maxy, minz, maxz);
    } else if (!strcmp(name, "disk1")) {
        //double m = 0.01;
        double m = 10;
        /*
        double k0 = 1.0 / m;
        for (double r = 0.05; r < side; r += 0.05) {
            // 1/2r
            int k = k0/(2.0*r);
            for (int i = 0; i < k; i++) {
                double phi = 2*M_PI * (double)rand_(&seed) / rand_max;
                double x = r*cos(phi);
                double y = r*sin(phi);
                double z = 0.0;

                coords[n] = x;
                coords[n+1] = y;
                coords[n+2] = z;
                coords[n+3] = m;

                vels[n] = vels[n+1] = vels[n+2] = vels[n+3] = 0;
                double R =
                    coords[n]*coords[n]+
                    coords[n+1]*coords[n+1];
                R = sqrt(R);
                double V = 1;

                vels[n] = V*coords[n+1]/R;
                vels[n+1] = -V*coords[n]/R;

                n += 4;
            }
        }
        */
        while (n < 4*n_particles) {
            double r = side * (double)rand_(&seed) / rand_max;
            double phi = 2*M_PI * (double)rand_(&seed) / rand_max;
            double x = r*cos(phi);
            double y = r*sin(phi);
            double z = 0.0;

            coords[n] = x;
            coords[n+1] = y;
            coords[n+2] = z;
            coords[n+3] = m;

            vels[n] = vels[n+1] = vels[n+2] = vels[n+3] = 0;
            double R =
                coords[n]*coords[n]+
                coords[n+1]*coords[n+1];
            R = sqrt(R);

            //double V = V0*R;
            double V = V0;
            vels[n] = V*coords[n+1]/R;
            vels[n+1] = -V*coords[n]/R;

            n += 4;
        }

    } else if (!strcmp(name, "disk")) {
        double alpha = 5;
        double ad = side;
        double as = ad/alpha;

        double rho = 0.0; // 0.2;
        int n_particles_d = n_particles*rho;
        // double md = n_particles_d;
        int n_particles_s = n_particles-n_particles_d;
        double mass = 1;
        double ms = mass*n_particles_s;
#if 1
        // halo
        while (n < 4*n_particles_s) {
            double x = 2*as*(double)rand_(&seed) / rand_max - as;
            double y = 2*as*(double)rand_(&seed) / rand_max - as;
            double z = 2*as*(double)rand_(&seed) / rand_max - as;

            if (x*x+y*y+z*z<as*as) {
                coords[n] = x;
                coords[n+1] = y;
                coords[n+2] = z;
                coords[n+3] = mass; // mass

                vels[n] = vels[n+1] = vels[n+2] = vels[n+3] = 0;
                //vels[n+3] = -1;

                double R =
                    coords[n]*coords[n]+
                    coords[n+1]*coords[n+1]
                    ; // +coords[n+2]*coords[n+2];
                R = sqrt(R)+0.05;

                //double V = 0.5*V0*sqrt(ms/R);
                double V = V0*R;
                //double V = V0;
                vels[n] = V*coords[n+1]/R;
                vels[n+1] = -V*coords[n]/R;

                n += 4;
            }
        }
#endif
#if 1
        while (n < 4*n_particles) {
            double r = (ad-as) * (double)rand_(&seed) / rand_max + as;
            double phi = 2*M_PI * (double)rand_(&seed) / rand_max;
            double x = r*cos(phi);
            double y = r*sin(phi);

            //double x = 2*ad*(double)rand_(&seed) / rand_max - ad;
            //double y = 2*ad*(double)rand_(&seed) / rand_max - ad;
            double z = 0;
            if (x*x+y*y+z*z>as*as && x*x+y*y+z*z<ad*ad) {
                coords[n] = x;
                coords[n+1] = y;
                coords[n+2] = z;
                coords[n+3] = mass;
                double R =
                    coords[n]*coords[n]+
                    coords[n+1]*coords[n+1]+
                    coords[n+2]*coords[n+2];
                R = sqrt(R);

                //double V = V0; // V0*sqrt(ms/R);
                double V = V0*sqrt(ms/R);
                //double V = V0*R;
                //double V = V0/R;
                vels[n] = V*coords[n+1]/R;
                vels[n+1] = -V*coords[n]/R;
                vels[n+2] = 0;
                vels[n+3] = 0;

                n += 4;
            }
        }
#endif
#if 0
        while (n < 4*n_particles) {
            double x = 2*(double)rand_(&seed) / rand_max - 1;
            double y = 2*(double)rand_(&seed) / rand_max - 1;

            double r = x*x+y*y;
            if (r == 0 || r >= 1) {
                continue;
            }

            double c = 0.5*x*sqrt(-2*log(r)/r);
            double phi = (double)rand_(&seed) / rand_max * 2*M_PI;
            x =  side*c*sin(phi);
            y = -side*c*cos(phi);
            double z = 0;

            if (x*x+y*y+z*z<side*side) {
                coords[n] = x;
                coords[n+1] = y;
                coords[n+2] = z;
                coords[n+3] = 1; // 0.2 + 1.5*(double)rand_(&seed) / (double)rand_max;

                double R =
                    coords[n]*coords[n]+
                    coords[n+1]*coords[n+1]+
                    coords[n+2]*coords[n+2];
                R = sqrt(R);
                //double V = V0/sqrt(R); // sqrt(100/R);
                double V = V0; // /R; // sqrt(100/R);

                vels[n] = V*coords[n+1];
                vels[n+1] = -V*coords[n];
                vels[n+2] = 0;

                vels[n+3] = 0;
                n += 4;
            }
        }
#endif
    } else if (!strcmp(name, "sphere1")) {
        while (n < 4*n_particles) {
            double r = side * (double)rand_(&seed) / rand_max;
            double phi = 2*M_PI * (double)rand_(&seed) / rand_max;
            double psi = M_PI * (double)rand_(&seed) / rand_max;
            double x = r*cos(phi)*sin(psi);
            double y = r*sin(phi)*sin(psi);
            double z = r*cos(psi);

            coords[n] = x;
            coords[n+1] = y;
            coords[n+2] = z;
            coords[n+3] = 1;

            double R =
                    coords[n]*coords[n]+
                    coords[n+1]*coords[n+1]+
                    coords[n+2]*coords[n+2];
            R = sqrt(R);
            double V = V0;
            vels[n] = V*coords[n+1];
            vels[n+1] = -V*coords[n];
            vels[n+2] = 0;
            vels[n+3] = 0;
            n += 4;
        }
    } else { // sphere

        while (n < 4*n_particles) {
            double x = 2*side*(double)rand_(&seed) / rand_max - side;
            double y = 2*side*(double)rand_(&seed) / rand_max - side;
            double z = 2*side*(double)rand_(&seed) / rand_max - side;

            if (x*x+y*y+z*z<side*side) {
                coords[n] = x;
                coords[n+1] = y;
                coords[n+2] = z;
                coords[n+3] = 0.2 + 1.5*(double)rand_(&seed) / (double)rand_max;

                double R =
                    coords[n]*coords[n]+
                    coords[n+1]*coords[n+1]+
                    coords[n+2]*coords[n+2];
                R = sqrt(R);
                double V = V0/sqrt(R); // sqrt(100/R);

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

    if (!data->color) {
        data->color = calloc(4*data->n_particles,sizeof(float));
        for (int i = 0; i < data->n_particles; i++) {
            data->color[i*4+3] = -1; // use default color
        }
    }
}

void particles_data_destroy(struct ParticlesData* data) {
    free(data->coords);
    free(data->vels);
    free(data->accel);
    free(data->color);
}
