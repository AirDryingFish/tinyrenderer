#include <algorithm>
#include "GL.h"

mat<4, 4> ModelView, ViewPort, Perspective, WorldToLight;
std::vector<double> zbuffer;
std::vector<double> shadow_map;

mat<4, 4> lookat(const vec3 eye, const vec3 center, const vec3 up){
    vec3 n = normalized(eye - center);      // camera direction (backward)
    vec3 l = normalized(cross(up, n));      // right
    vec3 m = normalized(cross(n, l));       // up

    mat<4,4> R = {{
        {l.x, l.y, l.z, 0},
        {m.x, m.y, m.z, 0},
        {n.x, n.y, n.z, 0},
        {0,   0,   0,   1}
    }};

    mat<4,4> T = {{
        {1,0,0,-eye.x},
        {0,1,0,-eye.y},
        {0,0,1,-eye.z},
        {0,0,0,1}
    }};

    ModelView = R * T;
    return ModelView;
}

void init_perspective(const double fov, const double aspect, const double zNear, const double zFar){
    Perspective = mat<4,4>{{
        {1 / (aspect * tan(fov * 0.5)), 0, 0, 0},
        {0, 1 / tan(fov * 0.5), 0, 0},
        {0, 0, -(zFar + zNear) / (zFar - zNear), -2 * zFar * zNear / (zFar - zNear)},
        {0, 0, -1, 0}
    }};
}

void init_viewport(const double x, const double y, const double w, const double h){
    // screen: ([x, x + w], [y, y + h])
    ViewPort = mat<4,4>{{
        {w/2.0, 0, 0, x + w/2.0},
        {0, h/2.0, 0, y + h/2.0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    }};
}

void init_worldToLight(const vec3 lightPos, const vec3 up){
    vec3 n = lightPos;
    vec3 l = normalized(cross(up, n));
    vec3 m = normalized(cross(n, l));

    mat<4,4> R = {{
        {l.x, l.y, l.z, 0},
        {m.x, m.y, m.z, 0},
        {n.x, n.y, n.z, 0},
        {0,   0,   0,   1}
    }};

    mat<4,4> T = {{
        {1,0,0,-lightPos.x},
        {0,1,0,-lightPos.y},
        {0,0,1,-lightPos.z},
        {0,0,0,1}
    }};

    WorldToLight = R * T;
}
// void init_ortho(const double zNear, const double zFar){

// }

void init_zbuffer(const int width, const int height){
    zbuffer = std::vector(width * height, std::numeric_limits<double>::max());
}

void init_shadowmap(const int width, const int height){
    shadow_map = std::vector(width * height, std::numeric_limits<double>::max());
}

vec3 reflect(const vec3 l, const vec3 n){
    // standard reflection formula: r = l - 2*(l·n)*n
    return l - 2.0 * (l * n) * n;
}

void rasterize(const Triangle& clip, const IShader& shader, TGAImage &framebuffer, std::vector<double>& inzbuffer){
    

    vec4 ndc[3] = {clip[0] / clip[0].w, clip[1] / clip[1].w, clip[2] / clip[2].w};
    vec2 screen[3] = {(ViewPort * ndc[0]).xy(), (ViewPort * ndc[1]).xy(), (ViewPort * ndc[2]).xy()};
    mat<3, 3> ABC = {{
        {screen[0].x, screen[0].y, 1},
        {screen[1].x, screen[1].y, 1},
        {screen[2].x, screen[2].y, 1}
    }};
    if (ABC.det() < 1){
        return;
    }
    auto [xbbmin, xbbmax] = std::minmax({screen[0].x, screen[1].x, screen[2].x});
    auto [ybbmin, ybbmax] = std::minmax({screen[0].y, screen[1].y, screen[2].y});
    for (int x = std::max<int>(xbbmin, 0); x <= std::min<int>(xbbmax, framebuffer.width() - 1); x++){
        for (int y = std::max<int>(ybbmin, 0); y <= std::min<int>(ybbmax, framebuffer.height() - 1); y++){
            vec3 barycentric = ABC.invert_transpose() * vec3{static_cast<double>(x), static_cast<double>(y), 1};
            if (barycentric.x < 0 || barycentric.y < 0 || barycentric.z < 0){
                continue;
            }


            double z = barycentric * vec3{ndc[0].z, ndc[1].z, ndc[2].z};

            vec3 corrected_weights;
            float w0 = clip[0].w, w1 = clip[1].w, w2 = clip[2].w;
            corrected_weights.x = barycentric.x / w0;
            corrected_weights.y = barycentric.y / w1;
            corrected_weights.z = barycentric.z / w2;
            corrected_weights = corrected_weights / (corrected_weights.x + corrected_weights.y + corrected_weights.z);

            if (z >= inzbuffer[x + y * framebuffer.width()]){
                continue;
            }

            auto [discard, color] = shader.fragment(corrected_weights);

            if (discard){
                continue;
            }

            inzbuffer[x + y * framebuffer.width()] = z;
            // std::cout << z << std::endl;
            framebuffer.set(x, y, color);
        }
    }
}

bool IShader::isInShadow(const vec4& worldPos, int width, int height) const{
    vec4 lightClipPos = Perspective * WorldToLight * worldPos;
    lightClipPos = lightClipPos / lightClipPos.w;
    double cur_depth = lightClipPos.z;

    vec2 lightScreen =  (ViewPort * lightClipPos).xy();
    int lightx = (int)lightScreen.x;
    int lighty = (int)lightScreen.y;
    if (lightx > width){
        lightx = width;
    }
    if (lightx < 0) {
        lightx = 0;
    }
    if (lighty > height){
        lighty = height;
    }
    if (lighty < 0){
        lighty = 0;
    }
    double shadowmap_depth = shadow_map[lightx + lighty * width];

    bool isinshadow = cur_depth > shadowmap_depth + 0.001;
    return isinshadow;
}