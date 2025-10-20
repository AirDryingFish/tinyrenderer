#include <fstream>
#include <sstream>
#include "model.h"

Model::Model(const std::string filename) {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        std::string tag;
        iss >> tag;
        if (tag == "v"){
            vec3 v;
            for (int i : {0, 1, 2}){
                iss >> v[i];
            }
            verts.push_back(v);
        }
        if (tag == "vn"){
            vec3 vn;
            for (int i : {0, 1, 2}){
                iss >> vn[i];
            }
            normals.push_back(vn);
        }
        if (tag == "vt"){
            vec3 vt;
            for (int i : {0, 1, 2}){
                iss >> vt[i];
            }
            uvs.push_back(vec3{vt.x, 1 - vt.y, vt.z});
        }

        else if (tag == "f")
        {
            int f,t,n, cnt = 0;
            char trash;
            while (iss >> f >> trash >> t >> trash >> n){
                facet_vrt.push_back(--f);
                face_normal.push_back(--n);
                face_uv.push_back(--t);
                cnt++;
            }
            // std::cout << f << " " << t << " " << n << std::endl;
            if (3!=cnt) {
                std::cerr << "Error: the obj file is supposed to be triangulated" << std::endl;
                return;
            }
        }
        
    }

    tangents.resize(verts.size(), vec3{0, 0, 0});
    compute_tangent();
    std::cerr << "# v# " << nverts() << " f# "  << nfaces() << std::endl;
}

void Model::load_nm_tangent_tex(const std::string nm_tex_filename) {
    nm_tangent_tex.read_tga_file(nm_tex_filename);
}

void Model::load_diffuse_tex(const std::string diffuse_tex_filename) {
    diffuse_tex.read_tga_file(diffuse_tex_filename);
}


int Model::nverts() const { return verts.size(); }
int Model::nfaces() const { return facet_vrt.size()/3; }

vec3 Model::vert(const int i) const {
    return verts[i];
}

vec3 Model::vert(const int iface, const int nthvert) const {
    return verts[facet_vrt[iface*3+nthvert]];
}

vec3 Model::normal(const int iface, const int nthvert) const {
    return normals[face_normal[iface*3+nthvert]];
}

vec3 Model::uv(const int iface, const int nthvert) const {
    return uvs[face_uv[iface*3+nthvert]];
}

vec3 Model::tangent(const int iface, const int nthvert) const {
    return tangents[facet_vrt[iface*3+nthvert]];
}

const TGAImage& Model::get_nm_tangent_tex() const{
    return nm_tangent_tex;
}

vec4 Model::normal(const vec2 &uv) const{
    TGAColor c = get_nm_tangent_tex().get(uv.x * get_nm_tangent_tex().width(), uv.y * get_nm_tangent_tex().height());
    return vec4{(double)c[2],(double)c[1],(double)c[0],0} * 2. / 255. - vec4{1, 1, 1, 0};
}

TGAColor Model::diffuse_color(const vec2 &uv) const{
    return diffuse_tex.get(uv.x * diffuse_tex.width(), uv.y * diffuse_tex.height());
}

void Model::compute_tangent(){
    for (int i = 0; i < facet_vrt.size()/3; i++){
        vec3 p1 = verts[facet_vrt[i*3+0]];
        vec2 uv1 = uvs[face_uv[i*3+0]].xy();
        vec3 p2 = verts[facet_vrt[i*3+1]];
        vec2 uv2 = uvs[face_uv[i*3+1]].xy();
        vec3 p3 = verts[facet_vrt[i*3+2]];
        vec2 uv3 = uvs[face_uv[i*3+2]].xy();
        
        vec3 edge1 = p2 - p1;
        vec3 edge2 = p3 - p1;

        vec2 duv1 = uv2 - uv1;
        vec2 duv2 = uv3 - uv1;

        float f = 1.0 / (duv1.x * duv2.y - duv2.x * duv1.y);

        vec3 tangent = f * (edge1 * duv2.y - edge2 * duv1.y);
        vec3 bitangent = f * (edge2 * duv1.x - edge1 * duv2.x);
        for (int idx = 0; idx < 3; ++idx){
            // accumulate tangent for each vertex of the face
            tangents[facet_vrt[i*3 + idx]] = tangents[facet_vrt[i*3 + idx]] + tangent;
        }

    }
    // normalize accumulated tangents per-vertex to get a proper direction
    for (size_t vi = 0; vi < tangents.size(); ++vi) {
        tangents[vi] = normalized(tangents[vi]);
    }
}
