#include <vector>
#include <iostream>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

Model* model = NULL;
const int width = 800;
const int height = 800;

Vec3f light_dir = Vec3f(1, 1, 1).normalize();
Vec3f       eye(1, 1, 3);
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

struct GouraudShader : public IShader {
    mat<2, 3, float> varying_uv;              // same as above
    mat<3, 3, float> varying_vPos_viewspace;
    mat<3, 3, float> varying_vNorm_viewspace;

    mat<4, 4, float> uniform_mvp;             // Projection*ModelView
    mat<4, 4, float> uniform_view;
    mat<4, 4, float> uniform_modelview_invtran;     // (Projection*ModelView).invert_transpose()

    virtual Vec4f vertex(int iface, int nthvert) {
        Vec4f in_vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        Vec4f gl_Vertex = Viewport * uniform_mvp * in_vertex;     // transform it to screen coordinates
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        varying_vPos_viewspace.set_col(nthvert, proj<3>(uniform_view * in_vertex));
        varying_vNorm_viewspace.set_col(nthvert, proj<3>(uniform_modelview_invtran * embed<4>(model->normal(iface, nthvert), 0.f)));

        return gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor& out_color) {
        // light calculation in view space
        Vec2f uv = varying_uv * bar;                 // interpolate uv for the current pixel
        Vec3f fragPos = varying_vPos_viewspace * bar;       // interpolate world space position for the current pixel
        Vec3f geomrty_normal = (varying_vNorm_viewspace * bar).normalize(); // interpolate normal for the current pixel

        // calculate tangent
        mat<3, 3, float> A;
        A[0] = varying_vPos_viewspace.col(1) - varying_vPos_viewspace.col(0);
        A[1] = varying_vPos_viewspace.col(2) - varying_vPos_viewspace.col(0);
        A[2] = geomrty_normal;

        mat<3, 3, float> AI = A.invert();
        Vec3f i = AI * Vec3f(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
        Vec3f j = AI * Vec3f(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);

        mat<3, 3, float> B;
        B.set_col(0, i.normalize());
        B.set_col(1, j.normalize());
        B.set_col(2, geomrty_normal);
           
        // light calculation     !!caution!! vector can't be translated, set w to 0.
        Vec3f n = (B * model->normal_tangent(uv)).normalize();
        // Vec3f n = proj<3>(uniform_modelview_invtran * embed<4>(model->normal(uv), 0.f)).normalize();
        Vec3f l = proj<3>(uniform_view * embed<4>(light_dir, 0.f)).normalize(); 
        Vec3f r = reflect(n, l).normalize();
        Vec3f viewDir = (Vec3f(0.f, 0.f, 0.f) - fragPos).normalize();
        TGAColor color = model->diffuse(uv);
        // ambient
        TGAColor ambient = color * 0.1;
        // diffuse
        float diff = std::max(0.f, n * l );
        TGAColor diffuse = color * diff;
        // specular
        // float shininess = std::max(4.f, model->specular(uv) / 255.0f * 64.0f);
        float spec = std::pow(std::max(0.f, viewDir * r), model->specular(uv));
        TGAColor specular = TGAColor(255, 255, 255) * spec * 0.4f;

        for(int i = 0; i < 3; i++) out_color[i] = std::min<int>(ambient[i] + diffuse[i] + specular[i], 255);
        return false;                              // no, we do not discard this pixel
    }
};

int main(int argc, char** argv) {
    if (2 == argc) {
        model = new Model(argv[1]);
    }
    else {
        model = new Model("../../obj/african_head/african_head.obj");
    }

    light_dir.normalize();

    mat4 ModelView = lookat(eye, center, up);
    Viewport = viewport(0, 0, width, height);
    Projection = perspective(45, width / height, 0.1, 10);

    TGAImage image(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::RGBA);
    for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			zbuffer.set(i, j, TGAColor(1.f));
		}
	}

    GouraudShader shader;
    shader.uniform_mvp = Projection * ModelView;
    shader.uniform_modelview_invtran = ModelView.invert_transpose();
    shader.uniform_view = ModelView;
    for (int i = 0; i < model->nfaces(); i++) {
        Vec4f screen_coords[3];
        for (int j = 0; j < 3; j++) {
            screen_coords[j] = shader.vertex(i, j);
        }
        triangle(screen_coords, shader, image, zbuffer);
    }

    { // dump z-buffer (debugging purposes only)
        TGAImage zbimage(width, height, TGAImage::GRAYSCALE);
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                float depth = fragDepth2LinearDepth(zbuffer.get(i, j).val, 0.1f, 10.f);
                zbimage.set(i, j, TGAColor(unsigned char(255 - depth * 255)));
            }
        }
        zbimage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        zbimage.write_tga_file("zbuffer.tga");
    }

    image.flip_vertically(); // to place the origin in the bottom left corner of the image
    image.write_tga_file("output.tga");

    delete model;
    return 0;
}