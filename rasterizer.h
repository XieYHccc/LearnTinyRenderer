//
// Created by 解沂航 on 04/05/2023.
//
#ifndef LEARNTINYRENDERER_RASTERIZER_H
#define LEARNTINYRENDERER_RASTERIZER_H
#include "geometry.h"
#include "tgaimage.h"
#include <cmath>
#include <vector>
#include "geometry.h"

Vec3f barycentric(Vec2i* vertex, Vec2i P){
    // calculate cross product
    Vec3f uv1=Vec3f(vertex[1].x-vertex[0].x,vertex[2].x-vertex[0].x,vertex[0].x-P.x)^
            Vec3f(vertex[1].y-vertex[0].y,vertex[2].y-vertex[0].y,vertex[0].y-P.y);

    if(abs(uv1.z)<1) return Vec3f(-1,1,1);
    return Vec3f (1.f-(uv1.x+uv1.y)/uv1.z,uv1.x/uv1.z,uv1.y/uv1.z);
}

void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) {
    bool steep = false;
    if (std::abs(x0-x1)<std::abs(y0-y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0>x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    float dy = std::abs((y1-y0)/(float)(x1-x0));
    float error =0.0;
    int y=y0;
    for (int x=x0; x<=x1; x++) {
        if (steep) {
            image.set(y, x, color);
        }
        else {
            image.set(x, y, color);
        }
        error +=dy;
        if (error>0.5)
        {
            y+=(y1>y0?1:-1);
            error-=1.;
        }
    }
}
void triangle_line_sweeping(Vec2f p0, Vec2f p1, Vec2f p2, TGAImage &image, TGAColor color){
    if(p0.y>p1.y) std::swap(p0,p1);
    if(p0.y>p2.y) std::swap(p0,p1);
    if(p1.y>p2.y) std::swap(p1,p2);

    double dx_0to1 = (p1.x-p0.x)/(p1.y-p0.y),dx_0to2 = (p2.x-p0.x)/(p2.y-p0.y);
    double x_0to1=p0.x,x_0to2=p0.x;
    int x_left, x_right;
    for(int y=p0.y;y<p1.y;y++)
    {
        x_left = std::min(x_0to1,x_0to2);
        x_right = std::max(x_0to1,x_0to2);
        for(int x=x_left;x<=x_right+1;x++)
        {
            image.set(x,y,color);
        }
        x_0to1 +=dx_0to1;
        x_0to2 +=dx_0to2;
    }

    double x_1to2 = p1.x;
    double dx_1to2 = (p2.x-p1.x)/(p2.y-p1.y);
    for(int y =p1.y;y<p2.y+1;y++){
        x_left = std::min(x_0to2,x_1to2);
        x_right = std::max(x_0to2,x_1to2);
        for(int x=x_left;x<=x_right+1;x++)
        {
            image.set(x,y,color);
        }
        x_0to2 +=dx_0to2;
        x_1to2 +=dx_1to2;
    }
}
void triangle(Vec2i* vertex, TGAImage &image, TGAColor color)
{
    int width = image.get_width();
    int height = image.get_height();

    // find bounding box of the triangle
    int xmin,xmax,ymin,ymax;
    xmin = std::max(0,std::min(vertex[0].x,std::min(vertex[1].x,vertex[2].x)));
    xmax = std::min(width-1,std::max(vertex[0].x,std::max(vertex[1].x,vertex[2].x)));
    ymin = std::max(0,std::min(vertex[0].y,std::min(vertex[1].y,vertex[2].y)));
    ymax = std::min(height-1,std::max(vertex[0].y,std::max(vertex[1].y,vertex[2].y)));
    if(xmin>width-1||xmax<0||ymin>height-1||ymax<0)
        return;

    for(int x=xmin;x<=xmax;x++){
        for(int y =ymin;y<=ymax;y++){
            Vec3f bc_screen = barycentric(vertex,Vec2i(x,y));
            if (bc_screen.x<0||bc_screen.y<0||bc_screen.z<0)
                continue;
            image.set(x,y,color);
        }
    }

}
#endif //LEARNTINYRENDERER_RASTERIZER_H
