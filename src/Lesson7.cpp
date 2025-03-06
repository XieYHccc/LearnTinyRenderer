#include <vector>
#include <iostream>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

Model* model = NULL;

const int width = 800;
const int height = 800;

Vec3f light_position = Vec3f(1, 1, 1);
Vec3f light_dir = Vec3f(1, 1, 1).normalize();
Vec3f       eye(0.f, 0.f, 3.f);
Vec3f    center(0, 0, 0);
Vec3f        up(0, 1, 0);

mat4 Viewport;
mat4 Projection;

float fragDepth2LinearDepth(float frag_z, float zNear, float zFar) {
    float n = zNear;
    float f = zFar;
    float z_ndc = 2 * (frag_z - 0.5f); // convert to [-1, 1] from [0, 1]
    float linear_depth = -2 * n * f / (z_ndc * (f - n) - f - n);
    float normalized_depth = (linear_depth - n) / (f - n);

    return normalized_depth;
}

Vec3f reflect(Vec3f normal, Vec3f outward_light)
{
    normal.normalize();
    outward_light.normalize();
    return (normal * 2 * (normal * outward_light) - outward_light).normalize();
}

struct Shader : public IShader {
    mat<2, 3, float> varying_vUv;              // same as above
    mat<3, 3, float> varying_vPos;
    mat<3, 3, float> varying_vNorm;

    mat<4, 4, float> uniform_lightMatrix;   // only a ortho projection matrix here
    mat<4, 4, float> uniform_mvp;             // Projection*ModelView
    mat<4, 4, float> uniform_model;
    mat<4, 4, float> uniform_model_invtran;     // (Projection*ModelView).invert_transpose()

    TGAImage* inputAttachment_shadowMap;

    float shadowCalculation(Vec4f fragPosLightSpace, Vec3f normal, Vec3f lightDir)
    {
        normal.normalize();
        lightDir.normalize();
        Vec3f projCoords = proj<3>(fragPosLightSpace / fragPosLightSpace[3]);
        projCoords = projCoords * 0.5f + Vec3f(0.5f, 0.5f, 0.5f);
        float current_depth = projCoords.z;
        int x = projCoords.x * inputAttachment_shadowMap->get_width();
        int y = projCoords.y * inputAttachment_shadowMap->get_height();
        TGAColor sampled_depth = inputAttachment_shadowMap->get(x, y);

        float bias = std::max(0.1f * (1.0 - (normal * lightDir)), 0.005);
        float shadow = current_depth - 0.05 > sampled_depth.val ? 0.8f : 0.0;

        return shadow;
    }

    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f in_vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        Vec4f gl_Vertex = Viewport * uniform_mvp * in_vertex;     // transform it to screen coordinates
        varying_vUv.set_col(nthvert, model->uv(iface, nthvert));
        varying_vPos.set_col(nthvert, proj<3>(uniform_model * embed<4>(in_vertex, 1.f)));
        varying_vNorm.set_col(nthvert, proj<3>(uniform_model_invtran * embed<4>(model->normal(iface, nthvert), 0.f)));
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor& out_color) {
        // interpolation
        Vec2f uv = varying_vUv * bar;                 // interpolate uv for the current pixel
        Vec3f fragPos = varying_vPos * bar;       // interpolate world space position for the current pixel
        Vec3f fragNorm = (varying_vNorm * bar).normalize(); // interpolate normal for the current pixel

        // calculate tangent
        mat<3, 3, float> A;
        A[0] = varying_vPos.col(1) - varying_vPos.col(0);
        A[1] = varying_vPos.col(2) - varying_vPos.col(0);
        A[2] = fragNorm;
        mat<3, 3, float> AI = A.invert();
        Vec3f i = AI * Vec3f(varying_vUv[0][1] - varying_vUv[0][0], varying_vUv[0][2] - varying_vUv[0][0], 0);
        Vec3f j = AI * Vec3f(varying_vUv[1][1] - varying_vUv[1][0], varying_vUv[1][2] - varying_vUv[1][0], 0);
        mat<3, 3, float> B;
        B.set_col(0, i.normalize());
        B.set_col(1, j.normalize());
        B.set_col(2, fragNorm);

        // light calculation     !!caution!! vector can't be translated, set w to 0.
        Vec3f n = (B * model->normal_tangent(uv)).normalize();
        Vec3f l = light_dir;
        Vec3f r = reflect(n, l).normalize();
        Vec3f viewPos = eye;
        Vec3f viewDir = (viewPos - fragPos).normalize();
        TGAColor color = model->diffuse(uv);
        // ambient
        TGAColor ambient = color * 0.15;
        // diffuse
        float diff = std::max(0.f, n * l);
        TGAColor diffuse = color * diff;
        // specular !! not sure if the specular map is correct
        float shininess = std::max(4.f, model->specular(uv) / 255.0f * 64.0f);
        float spec = std::pow(std::max(0.f, viewDir * r), shininess);
        TGAColor specular = TGAColor(255, 255, 255) * spec * 0.3f;

        // shadow calculation
        Vec4f fragPosLight = uniform_lightMatrix * embed<4>(fragPos, 1.0f);
        float shadow = shadowCalculation(fragPosLight, n, l);

        for (int i = 0; i < 3; i++) out_color[i] = std::min<int>(ambient[i] + ((diffuse[i] + specular[i]) * (1 - shadow)), 255);
        return false;                              // no, we do not discard this pixel
    }
};

struct DepthShader : public IShader {
    mat<3, 3, float> varying_vPos;
    mat<4, 4, float> uniform_lightSpaceMatrix; // mvp
    mat<4, 4, float> uniform_model; // mvp

    DepthShader() : varying_vPos() {}

    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        gl_Vertex = Viewport * uniform_lightSpaceMatrix * uniform_model * gl_Vertex;          // transform it to screen coordinates
        varying_vPos.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor& color) {
        Vec3f p = varying_vPos * bar;
        float frag_depth = std::max(0.f, std::min(1.f, (0.5f * p.z + .5f)));
        color = TGAColor(255, 255, 255) * frag_depth;
        return false;
    }
};

int main(int argc, char** argv) {
    if (2 == argc) {
        model = new Model(argv[1]);
    }
    else {
        model = new Model("../../obj/diablo3_pose/diablo3_pose.obj");
    }

    light_dir.normalize();
    
    // lighting pass framebuffer
    TGAImage frame(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::RGBA);

    // depth pass framebuffer
    TGAImage shadow_buffer(width, height, TGAImage::RGB);
    TGAImage shadow_zbuffer(width, height, TGAImage::RGBA); // TODO: remove this redundent buffer
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            zbuffer.set(i, j, TGAColor(1.f));
            shadow_zbuffer.set(i, j, TGAColor(1.f));
            shadow_buffer.set(i, j, TGAColor(255, 255, 255));
        }
    }

    mat4 lightSpaceMatrix = ortho(-2, 2, -2, 2, 0.1, 5.f) * lookat(light_position, center, up);
    {   // rendering the shadow buffer
        Viewport = viewport(0, 0, width, height);

        DepthShader depthshader;
        depthshader.uniform_lightSpaceMatrix = lightSpaceMatrix;
        depthshader.uniform_model = mat4::identity();
        Vec4f screen_coords[3];
        for (int i = 0; i < model->nfaces(); i++) {
            for (int j = 0; j < 3; j++) {
                screen_coords[j] = depthshader.vertex(i, j);
            }
            triangle(screen_coords, depthshader, shadow_buffer, shadow_zbuffer);
        }
        shadow_buffer.flip_vertically(); // to place the origin in the bottom left corner of the image
        shadow_buffer.write_tga_file("depth.tga");
        //shadow_zbuffer.flip_vertically(); !!!!!! this is wrong, zbuffer should not be flipped
    }

    {
        // lighting pass
        mat4 ModelView = lookat(eye, center, up);
        mat4 Projection = perspective(45, width / height, 0.1f, 100.f);
        Viewport = viewport(0, 0, width, height);

        Shader shader;
        shader.uniform_mvp = Projection * ModelView;
        shader.uniform_model = mat4::identity();
        shader.uniform_model_invtran = shader.uniform_model.invert_transpose();
        shader.uniform_lightMatrix = lightSpaceMatrix;
        shader.inputAttachment_shadowMap = &shadow_zbuffer;

        for (int i = 0; i < model->nfaces(); i++) {
            Vec4f screen_coords[3];
            for (int j = 0; j < 3; j++) {
                screen_coords[j] = shader.vertex(i, j);
            }
            triangle(screen_coords, shader, frame, zbuffer);
        }
        frame.flip_vertically(); // to place the origin in the bottom left corner of the image
        frame.write_tga_file("framebuffer.tga");
    }

    { // dump z-buffer (debugging purposes only)
        TGAImage zbimage(width, height, TGAImage::GRAYSCALE);
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                float depth = fragDepth2LinearDepth(zbuffer.get(i, j).val, 0.1f, 100.f);
                zbimage.set(i, j, TGAColor(unsigned char(255 - depth * 255)));
            }
        }
        // zbimage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        zbimage.write_tga_file("zbuffer.tga");
    }

    delete model;
    return 0;
}