#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "rasterizer.h"

Model *model = nullptr;
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0,   255, 0,   255);
const int width = 800;
const int height =800;


int main(int argc, char** argv){
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }
    TGAImage image(width, height, TGAImage::RGB);

    Vec3f light_dir(0,0,-1);
    for(int i=0;i<model->nfaces();i++)
    {
        Vec2i screen_coords[3];
        Vec3f world_coords[3];
        std::vector<int> face = model->face(i);
        for(int j =0;j<3;j++){
            world_coords[j] = model->vert(face[j]);
            screen_coords[j] = Vec2i((world_coords[j].x+1.)*width/2,(world_coords[j].y+1.)*height/2);
        }
        Vec3f normal = ((world_coords[2]-world_coords[0])^(world_coords[1]-world_coords[0])).normalize();
        float intensity = normal*light_dir;
        if (intensity>0)
            triangle(screen_coords,image,TGAColor(255*intensity, 255*intensity, 255*intensity, 255));
    }
    image.flip_vertically(); // I want to have the origin at the left bottom corner of the image
    image.write_tga_file("framebuffer.tga");
    delete model;
    return 0;
}

