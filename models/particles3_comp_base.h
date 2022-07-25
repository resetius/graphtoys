layout(std140, binding=0) uniform Settings {
    vec4 origin;
    int particles;
    int stage;
    int nn;  // nx=ny=nz=nn
    int n;   // log(nn)
    int nlists;
    float h;
    float l;
    float rho;
    float rcrit;
};

const float M_PI = 3.14159265359;
const float G = 1; // TODO

uint xsize = gl_NumWorkGroups.x*gl_WorkGroupSize.x;
uint ysize = gl_NumWorkGroups.y*gl_WorkGroupSize.y;
uint zsize = gl_NumWorkGroups.z*gl_WorkGroupSize.z;

uint globalIndex =
    gl_GlobalInvocationID.z*xsize*ysize+
    gl_GlobalInvocationID.y*xsize+
    gl_GlobalInvocationID.x;

#define off(i,k,j) ((i)*nn*nn+(k)*nn+(j))
#define poff(i,k,j) (((i+nn)%nn)*nn*nn+((k+nn)%nn)*nn+((j+nn)%nn))
