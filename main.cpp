#include <cmath>
#include <algorithm>
#include <tuple>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"
#include "GL.h"


constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 0,  0, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

constexpr int width  = 1024;
constexpr int height = 1024;

extern mat<4, 4> ModelView, Perspective, WorldToLight;
extern std::vector<double> zbuffer, shadow_map;

constexpr TGAColor environment_light_color = {  0,   0, 255, 255};
constexpr float environment_light_strength = 0.;
constexpr vec3 eye{-1, 0, 3};

constexpr TGAColor light_color = white;
constexpr float light_strength = 0.5;

constexpr TGAColor diffuse_color = {  127,   127, 127, 255};
constexpr vec3 light_start_point = {1, 1, 1};

struct ShadowShader: IShader{
    const Model& model;
    ShadowShader(const Model& m):model(m){

    }

    mutable TGAColor color = {};
    virtual vec4 vertex(const int faceid, const int vertid) {
        vec3 v = model.vert(faceid, vertid);
        vec3 view_pos = (WorldToLight * vec4{v.x, v.y, v.z, 1}).xyz();
        vec4 clip_pos = Perspective * vec4{view_pos.x, view_pos.y, view_pos.z, 1};
        return clip_pos;
    }

    virtual std::pair<bool, TGAColor> fragment(const vec3 bc) const {
        return {false, white};
    }
};

struct RandomShader: IShader
{
    const Model& model;
    RandomShader(const Model& m):model(m){

    }
    vec3 tri[3];
    mutable TGAColor color = {};
    vec3 view[3];
    vec3 lightDir;
    vec3 worldSpacePos[3];
    vec3 viewSpaceNormals[3];
    vec3 viewSpaceTangents[3];
    vec3 viewSpaceBitangents[3];
    vec2 uv[3];

    virtual vec4 vertex(const int faceid, const int vertid){
        vec3 v = model.vert(faceid, vertid);
        worldSpacePos[vertid] = v;

        vec3 view_pos = (ModelView * vec4{v.x, v.y, v.z, 1}).xyz();
        vec4 clip_pos = Perspective * vec4{view_pos.x, view_pos.y, view_pos.z, 1};

        vec3 normal = model.normal(faceid, vertid);
        normal = normalized(normal);
        viewSpaceNormals[vertid] = (ModelView.invert_transpose() * vec4{normal.x, normal.y, normal.z, 0}).xyz();

        vec3 now_uv = model.uv(faceid, vertid);
        uv[vertid] = now_uv.xy();

        vec3 tangent = model.tangent(faceid, vertid);
        tangent = normalized(tangent);
        viewSpaceTangents[vertid] = (ModelView * vec4{tangent.x, tangent.y, tangent.z, 0}).xyz();

        vec3 bitangent = normalized(cross(normal, tangent));
        viewSpaceBitangents[vertid] = (ModelView * vec4{bitangent.x, bitangent.y, bitangent.z, 0}).xyz();

        view[vertid] = vec3{0, 0, 0} - view_pos; 

        lightDir = normalized((ModelView * vec4{light_start_point.x, light_start_point.y, light_start_point.z, 0}).xyz());
        
        return clip_pos;
    }


    virtual std::pair<bool, TGAColor> fragment(const vec3 bc) const {

        double viewx = bc * vec3{view[0].x, view[1].x, view[2].x};
        double viewy = bc * vec3{view[0].y, view[1].y, view[2].y};
        double viewz = bc * vec3{view[0].z, view[1].z, view[2].z};

        double worldPosx = bc * vec3{worldSpacePos[0].x, worldSpacePos[1].x, worldSpacePos[2].x};
        double worldPosy = bc * vec3{worldSpacePos[0].y, worldSpacePos[1].y, worldSpacePos[2].y};
        double worldPosz = bc * vec3{worldSpacePos[0].z, worldSpacePos[1].z, worldSpacePos[2].z};

        vec2 uv_frag = bc.x * uv[0] + bc.y * uv[1] + bc.z * uv[2];
        vec4 tangentSpaceNormal = model.normal(uv_frag);
        
        // Get interpolated view-space vectors
        vec3 normal = normalized(viewSpaceNormals[0] * bc[0] + viewSpaceNormals[1] * bc[1] + viewSpaceNormals[2] * bc[2]);
        vec3 tangent = normalized(viewSpaceTangents[0] * bc[0] + viewSpaceTangents[1] * bc[1] + viewSpaceTangents[2] * bc[2]);
        
        // Build TBN matrix - ensure orthonormal basis
        normal = normalized(normal);
        tangent = normalized(tangent);
        vec3 bitangent = normalized(cross(normal, tangent));
        mat<3,3> TBN = {{  // Column-major: [tangent bitangent normal]
            {tangent.x, bitangent.x, normal.x},
            {tangent.y, bitangent.y, normal.y},
            {tangent.z, bitangent.z, normal.z}
        }};
        
        // Transform sampled normal from tangent to view space
        vec3 viewSpaceNormal = normalized(TBN * tangentSpaceNormal.xyz());

        // vec3 reflectDir = reflect(vec3{0, 0, 0} - lightDir, viewSpaceNormal);
        vec3 view = normalized(vec3{viewx, viewy, viewz});
        vec4 worldPos = vec4{worldPosx, worldPosy, worldPosz, 1};

        vec3 halfVec = normalized(view + lightDir);
        bool isinshadow = isInShadow(worldPos, width, height);

        color = environment_light_color * environment_light_strength;


        TGAColor specular_color = light_color * light_strength * std::pow(std::max<double>(0, normalized(viewSpaceNormal) * halfVec), 20); //blinn-phong
        
        if (!isinshadow){
            color = color + diffuse_color * std::max<double>(0, viewSpaceNormal * lightDir);
            color = color + specular_color;
        }
        color = color + model.diffuse_color(uv_frag);
        return {false, color};
    }
};


int main(int argc, char** argv) {
    if (argc < 2){
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }
    
    
    constexpr vec3 center{0, 0, 0};
    constexpr vec3 up{0, 1, 0};
    constexpr float fov = M_PI/4.0;
    constexpr float aspect = (float)width / (float)height;
    constexpr float zNear = 0.1;
    constexpr float zFar = 10.0;

    

    TGAImage framezbuffer(width, height, TGAImage::RGB, {177, 195, 209, 255});
    TGAImage framebuffer(width, height, TGAImage::RGB, {177, 195, 209, 255});

    init_zbuffer( width, height );
    init_viewport(width/16, height/16, width*7/8, height*7/8);
    lookat(eye, center, up);
    init_perspective(fov, aspect, zNear, zFar);


    init_worldToLight(light_start_point, up);
    // lookat(light_start_point, center, up);
    init_shadowmap( width, height );

    for (int i_model = 1; i_model < argc; i_model++){
        Model model(argv[i_model]);
        std::string model_name = argv[i_model];
        int idx = model_name.rfind(".");
        std::string nm_tanget_file_name = model_name.substr(0, idx) + "_nm_tangent.tga";
        std::string diffuse_tex_file_name = model_name.substr(0, idx) + "_diffuse.tga";
        model.load_nm_tangent_tex(nm_tanget_file_name);
        model.load_diffuse_tex(diffuse_tex_file_name);
        ShadowShader shader(model);
        for (int i = 0; i < model.nfaces(); i++){
            Triangle clip_verts = {
                shader.vertex(i, 0),
                shader.vertex(i, 1),
                shader.vertex(i, 2),
            };
            rasterize(clip_verts, shader, framezbuffer, shadow_map);
        }
    }
    framezbuffer.write_tga_file("framezbuffer.tga");
    
    for (int i_model = 1; i_model < argc; i_model++){
        Model model(argv[i_model]);
        std::string model_name = argv[i_model];
        int idx = model_name.rfind(".");
        std::string nm_tanget_file_name = model_name.substr(0, idx) + "_nm_tangent.tga";
        std::string diffuse_tex_file_name = model_name.substr(0, idx) + "_diffuse.tga";
        model.load_nm_tangent_tex(nm_tanget_file_name);
        model.load_diffuse_tex(diffuse_tex_file_name);
        RandomShader shader(model);
        for (int i = 0; i < model.nfaces(); i++){
            Triangle clip_verts = {
                shader.vertex(i, 0),
                shader.vertex(i, 1),
                shader.vertex(i, 2),
            };
            rasterize(clip_verts, shader, framebuffer, zbuffer);
        }
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}

