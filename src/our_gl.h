#ifndef __OUR_GL_H__
#define __OUR_GL_H__

#include "tgaimage.h"
#include "geometry.h"

mat4 viewport(int x, int y, int w, int h);
mat4 perspective(float eye_fov, float aspect_ratio, float zNear, float zFar);
mat4 lookat(Vec3f eye, Vec3f center, Vec3f up);

struct IShader {
    virtual ~IShader() = default;
    virtual Vec4f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor& color) = 0;
};

void triangle(Vec4f* pts, IShader& shader, TGAImage& image, TGAImage& zbuffer);

#endif //__OUR_GL_H__