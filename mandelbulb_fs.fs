#version 410
// precision highp float;
uniform mat4 Rot;
uniform vec3 T;

subroutine vec3 next_point_type(vec3 p, vec3 p0);
subroutine uniform next_point_type next_point;

in vec2 coord;
out vec4 fragColor;

subroutine(next_point_type)
vec3 next_quadratic(vec3 p, vec3 p0) {
    return vec3(
        p.x*p.x-p.y*p.y-p.z*p.z,
        2*p.x*p.z,
        2*p.x*p.y) + p0;
}

subroutine(next_point_type)
vec3 next_cubic(vec3 p, vec3 p0) {
    return vec3(
        p.x*p.x*p.x-3*p.x*(p.y*p.y+p.z*p.z),
        -p.y*p.y*p.y+3*p.y*p.x*p.x-p.y*p.z*p.z,
        p.z*p.z*p.z-3*p.z*p.x*p.x+p.z*p.y*p.y) + p0;
}

subroutine(next_point_type)
vec3 next_nine(vec3 p, vec3 p0) {
    float x = p.x; float y = p.y; float z = p.z;
    float x2 = x*x, x3 = x*x2, x5 = x3*x2, x7 = x5*x2;
    float y2 = y*y, y3 = y*y2, y5 = y3*y2, y7 = y5*y2;
    float z2 = z*z, z3 = z*z2, z5 = z3*z2, z7 = z5*z2;

    float xn = x7*x2 - 36*x7*(y2+z2) + 126*x5*(y2+z2)*(y2+z2)-84*x3*(y2+z2)*(y2+z2)*(y2+z2)
        +9*x*(y2+z2)*(y2+z2)*(y2+z2)*(y2+z2);
    float yn = y7*y2 - 36*y7*(z2+x2) + 126*y5*(z2+x2)*(z2+x2)-84*y3*(z2+x2)*(z2+x2)*(z2+x2)
        +9*y*(z2+x2)*(z2+x2)*(z2+x2)*(z2+x2);
    float zn = z7*z2 - 36*z7*(x2+y2) + 126*z5*(x2+y2)*(x2+y2)-84*z3*(x2+y2)*(x2+y2)*(x2+y2)
        +9*z*(x2+y2)*(x2+y2)*(x2+y2)*(x2+y2);

    return vec3(xn, yn, zn) + p0;
}

subroutine(next_point_type)
vec3 next_quintic(vec3 p, vec3 p0) {
    float A, B, C, D;
    A = B = C = D = 0;
    C = 2;

    float x = p.x; float y = p.y; float z = p.z;

    float x2 = x*x, x3 = x*x2, x4 = x*x3;
    float y2 = y*y, y3 = y*y2, y4 = y*y3;
    float z2 = z*z, z3 = z*z2, z4 = z*z3;

    float xn = x*x4 - 10*x3*(y2 + A*y*z + z2) +5*x*(y4 + B*y3*z + C*y2*z2 + B*y*z3 + z4)
        + D*x2*y*z*(y+z);
    float yn = y*y4 - 10*y3*(z2 + A*x*z + x2) +5*y*(z4 + B*z3*x + C*z2*x2 + B*z*x3 + x4)
        + D*y2*z*x*(z+x);
    float zn = z*z4 - 10*z3*(x2 + A*x*y + y2) +5*z*(x4 + B*x3*y + C*x2*y2 + B*x*y3 + y4)
        + D*z2*x*y*(x+y);

    return vec3(xn, yn, zn) + p0;
}

bool iterate(vec3 p0) {
    vec3 p = vec3(0,0,0);
    bool ans = true;
    for (int i = 1; i < 32; i++) {
        //p = next_quadratic(p, p0);
        //p = next_cubic(p, p0);
        //p = next_nine(p, p0);
        p = next_point(p, p0);
        if (dot(p, p) > 4) {
            ans = false;
            break;
        }
    }
    return ans;
}

vec2 check_point(vec2 p0) {
    bool flag = false;
    float z0 = -1;
    float dist = 0;
    vec4 p1 = vec4(0,0,0,0);
    for (z0 = 1.0; z0 >= -1.0 && !flag; z0 = z0-0.01) {
        p1 = Rot*vec4(p0, z0, 1.0);
        flag = iterate(vec3(p1));
    }
    dist = length(p1);
    return vec2(z0, dist);
}

void main() {
    vec2 p = coord;
    vec2 p1 = check_point(p);
    float z = p1.x;
    float l = p1.y;
    if (z >= -1.0) {
        //float colorRegulator = 20*l;
        //vec3 color = vec3(0.95 + .012*colorRegulator , 1.0, .2+.4*(1.0+sin(.3*colorRegulator)));
        //vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
        //vec3 m = abs(fract(color.xxx + K.xyz) * 6.0 - K.www);

        //float r = 0.5*(z+1.0);
        //float b = 0.5*(z+1.0);
        //fragColor = 0.5*(z+1.0)*vec4(r, 1.0, b, 1.0);
        fragColor = 0.5*(z+1.0)*vec4(1.0, 1.0, 0, 1.0);
        //float light = 0.5*(z+1.0);
        //float light = l;
        //fragColor = light*vec4(color.z * mix(K.xxx, clamp(m - K.xxx, 0.0, 1.0), color.y), 1.0);


    } else {
        fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
