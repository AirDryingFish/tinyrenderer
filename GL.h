#include "tgaimage.h"
#include "geometry.h"

mat<4, 4> lookat(const vec3 eye, const vec3 center, const vec3 up);
void init_perspective(const double fov, const double aspect, const double zNear, const double zFar);
void init_viewport(const double x, const double y, const double w, const double h);
void init_zbuffer(const int width, const int height);
void init_shadowmap(const int width, const int height);
vec3 reflect(const vec3 l, const vec3 n);

struct IShader{
    virtual std::pair<bool, TGAColor> fragment(const vec3 bc) const = 0;
    virtual bool isInShadow(const vec4& worldPos, int width, int height) const;
};
typedef vec4 Triangle[3];
void rasterize(const Triangle& clip, const IShader& shader, TGAImage& framebuffer, std::vector<double>& inzbuffer);
void init_worldToLight(const vec3 lightPos, const vec3 up);

struct v2f{
    vec3 bc;
    vec3 normal;
};