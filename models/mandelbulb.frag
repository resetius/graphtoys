#version 410
// precision highp float;

layout (std140) uniform MatrixBlock {
    uniform mat4 Rot;
    uniform mat4 InvRot;
    uniform vec4 T;
    uniform int NextType;
    uniform float Ratio;
    uniform int Camera;
    uniform float Dist;
};

layout (location = 0) in vec2 coord;
layout (location = 0) out vec4 fragColor;

vec3 next_quadratic(vec3 p, vec3 p0, inout float dr) {
    float power = 2;
    dr = pow(length(p), power-1.0)*power*dr + 1.0;
    return vec3(
        p.x*p.x-p.y*p.y-p.z*p.z,
        2*p.x*p.z,
        2*p.x*p.y) + p0;
}

vec3 next_cubic(vec3 p, vec3 p0, inout float dr) {
    float power = 3;
    dr = pow(length(p), power-1.0)*power*dr + 1.0;

    return vec3(
        p.x*p.x*p.x-3*p.x*(p.y*p.y+p.z*p.z),
        -p.y*p.y*p.y+3*p.y*p.x*p.x-p.y*p.z*p.z,
        p.z*p.z*p.z-3*p.z*p.x*p.x+p.z*p.y*p.y) + p0;
}

vec3 next_nine(vec3 p, vec3 p0, inout float dr) {
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

    float power = 9;
    dr = pow(length(p), power-1.0)*power*dr + 1.0;

    return vec3(xn, yn, zn) + p0;
}

vec3 next_quintic(vec3 p, vec3 p0, inout float dr) {
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

    float power = 5;
    dr = pow(length(p), power-1.0)*power*dr + 1.0;

    return vec3(xn, yn, zn) + p0;
}

vec3 next_point(vec3 p, vec3 p0, inout float dr) {
    if (NextType == 0) {
        return next_quadratic(p, p0, dr);
    } else if (NextType == 1) {
        return next_cubic(p, p0, dr);
    } else if (NextType == 2) {
        return next_nine(p, p0, dr);
    } else {
        return next_quintic(p, p0, dr);
    }
}

// trap: the minimum of |p|^2 along the orbit (orbit trap), used for coloring
float DE(vec3 p0, out float trap) {
    vec3 p = vec3(0,0,0);
    float dr = 1.0;
    trap = 1e10;
    for (int i = 1; i < 32; i++) {
        p = next_point(p, p0, dr);
        trap = min(trap, dot(p, p));
        if (dot(p, p) > 4) {
            break;
        }
    }
    if (dot(p, p) <= 4) {
        return 0.0; // the orbit did not escape: the point is inside the set
    }
    float r = length(p);
    return 0.5*log(r)*r/dr;
}

float DE(vec3 p0) {
    float trap;
    return DE(p0, trap);
}


// ao: 1 for a ray that hit at once, towards 0 for one that needed many steps (crevices)
bool check_point(vec2 p0, out vec3 n, out float ao, out float trap) {
    bool flag = false;
    float dist = 0;
    vec4 p1 = vec4(0,0,0,0);

    vec3 ray, p;
    if (Camera != 0) {
        // perspective camera at distance Dist from the center, p0 is the tangent of the view angle
        ray = (Rot*vec4(normalize(vec3(p0, -1)), 0)).xyz;
        p = (Rot*vec4(0, 0, Dist, 0)).xyz;
    } else {
        ray = (Rot*vec4(0,0,-1,0)).xyz;
        p = (Rot*vec4(p0.x,p0.y,2,0)).xyz;
    }
    float td = 0; int i; int max_iters = 200;
    for (i = 0; td < 10 && i < max_iters; i++) {
        dist = DE(p);
        // with the camera a pixel grows with the distance, so does the hit threshold
        float thr = Camera != 0 ? 5e-5*T.z*td : 0.0001;
        if (abs(dist) <= thr) {
            flag = true;
            break;
        }
        td += dist;
        p += ray * dist;
    }
    ao = 1.0 - float(i)/float(max_iters);
    trap = 0.0;
    p1 = vec4(p.x,p.y,p.z,0);

    dist = length(p1);

    n = vec3(Rot*vec4(0,0,1,0));

    if (flag) {
        float eps = 1e-2*T.z*(Camera != 0 ? td : 1.0);
        vec2 h = vec2(1,-1);
        //vec3 p = vec3(p0, z0);
        vec3 p = vec3(p1);
        DE(p, trap);
        n = normalize(
            h.xyy*DE(p + h.xyy*eps) +
            h.yyx*DE(p + h.yyx*eps) +
            h.yxy*DE(p + h.yxy*eps) +
            h.xxx*DE(p + h.xxx*eps));
        //n = vec3(inverse(Rot)*vec4(n,1));
        n = vec3(InvRot*vec4(n,0));
    }

    return flag;
}

// n is in camera space: the viewer looks along -z
vec3 shade(vec3 n, float ao, float trap) {
    //float colorRegulator = 20*l;
    //vec3 color = vec3(0.95 + .012*colorRegulator , 1.0, .2+.4*(1.0+sin(.3*colorRegulator)));
    //vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    //vec3 m = abs(fract(color.xxx + K.xyz) * 6.0 - K.www);

    //float r = 0.5*(z+1.0);
    //float b = 0.5*(z+1.0);
    //fragColor = 0.5*(z+1.0)*vec4(r, 1.0, b, 1.0);
    //return;
    vec3 albedo = 0.55 + 0.45*cos(6.2831853*(1.2*sqrt(trap) + vec3(0.0, 0.15, 0.3)));

    vec3 s = normalize(vec3(-0.4, 0.6, 0.7)); // key light: upper left, in front
    vec3 h = normalize(s + vec3(0.0, 0.0, 1.0));
    float diff = max(dot(n, s), 0.0);
    float spec = pow(max(dot(n, h), 0.0), 32.0);

    return (albedo*(0.3 + 0.9*diff) + 0.3*spec)*ao;

    //fragColor = 0.5*(z+1.0)*vec4(1.0, 1.0, 0, 1.0);
    //float light = 0.5*(z+1.0);
    //float light = l;
    //fragColor = light*vec4(color.z * mix(K.xxx, clamp(m - K.xxx, 0.0, 1.0), color.y), 1.0);
}

const int SS = 2; // SS x SS rays per pixel

void main() {
    float pix = fwidth(coord.x); // pixel size in world units; must stay outside branches
    vec4 color = vec4(0.0);
    for (int sx = 0; sx < SS; sx++) {
        for (int sy = 0; sy < SS; sy++) {
            vec2 off = (vec2(sx, sy) + 0.5)/float(SS) - 0.5;
            vec3 n; float ao, trap;
            if (check_point(coord + off*pix, n, ao, trap)) {
                color += vec4(shade(n, ao, trap), 1.0);
            }
        }
    }
    fragColor = color/float(SS*SS);
}
