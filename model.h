#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
    std::vector<vec3> verts = {};    // array of vertices
    std::vector<vec3> normals = {};
    std::vector<vec3> uvs = {};
    std::vector<vec3> tangents = {};
    std::vector<int> facet_vrt = {}; // per-triangle index in the above array
    std::vector<int> face_normal = {};
    std::vector<int> face_uv = {};
    
    TGAImage nm_tangent_tex;
    TGAImage diffuse_tex;
public:
    Model(const std::string filename);
    void load_nm_tangent_tex(const std::string nm_tex_filename);
    void load_diffuse_tex(const std::string diffuse_tex_filename);
    int nverts() const; // number of vertices
    int nfaces() const; // number of triangles
    vec3 vert(const int i) const;                          // 0 <= i < nverts()
    vec3 vert(const int iface, const int nthvert) const;   // 0 <= iface <= nfaces(), 0 <= nthvert < 3

    vec3 normal(const int iface, const int nthvert) const;

    vec4 normal(const vec2 &uv) const;

    TGAColor diffuse_color(const vec2 &uv) const;

    vec3 uv(const int iface, const int nthvert) const;
    vec3 tangent(const int iface, const int nthvert) const;

    void compute_tangent();
    const TGAImage& get_nm_tangent_tex() const;

};

