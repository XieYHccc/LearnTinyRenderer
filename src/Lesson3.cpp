#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "geometry.h"
#include "model_lesson3.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);
Model* model = nullptr;

const int width = 800;
const int height = 800;
Vec3f light_dir(0, 0, -1);

void line(Vec2i p0, Vec2i p1, TGAImage& image, const TGAColor& color)
{
    bool steep = false;
    if (std::abs(p0.x - p1.x) < std::abs(p0.y - p1.y))
    {
        std::swap(p0.x, p0.y);
        std::swap(p1.x, p1.y);
        steep = true;
    }
    if (p0.x > p1.x)
        std::swap(p0, p1);

    int dx = p1.x - p0.x;
    int dy = p1.y - p0.y;
    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = p0.y;
    for (int x = p0.x; x <= p1.x; x++)
    {
        if (steep)
            image.set(y, x, color);
        else
            image.set(x, y, color);
        error2 += derror2;

        if (error2 > dx)
        {
            y += (p1.y > p0.y ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

Vec3f barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P) {
    Vec3f s[2];
    for (int i = 1; i >= 0; i--) {
        s[i][0] = C[i] - A[i];
        s[i][1] = B[i] - A[i];
        s[i][2] = A[i] - P[i];
    }
    Vec3f u = cross(s[0], s[1]);
    if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

TGAColor nearst_sample_texture(Vec2f uv, TGAImage& texture) {
    return texture.get(uv.x * texture.get_width(), uv.y * texture.get_height());
}

TGAColor linear_sample_texture(Vec2f uv, TGAImage& texture) {
    int width = texture.get_width();
    int height = texture.get_height();

    float x = uv.x * (width - 1);
    float y = uv.y * (height - 1);

    int x0 = (x - int(x) > 0.5) ? int(x) : int(x) - 1;
    int y0 = (y - int(y) > 0.5) ? int(y) : int(y) - 1;
    x0 = std::max(0, std::min(width - 1, x0));
    y0 = std::max(0, std::min(height - 1, y0));
    int x1 = std::min(width - 1, x0 + 1);
    int y1 = std::min(height - 1, y0 + 1);

    TGAColor P00 = texture.get(x0, y0);
    TGAColor P10 = texture.get(x1, y0);
    TGAColor P01 = texture.get(x0, y1);
    TGAColor P11 = texture.get(x1, y1);

    float tx = (x - int(x) > 0.5) ? x - int(x) - 0.5 : 0.5 + x - int(x);
    float ty = (y - int(y) > 0.5) ? y - int(y) - 0.5 : 0.5 + y - int(y);

    TGAColor A = P00 * (1 - tx) + P10 * tx;
    TGAColor B = P01 * (1 - tx) + P11 * tx;
    TGAColor finalColor = A * (1 - ty) + B * ty;
    return finalColor;
}

void triangle(Vec3f* pts, Vec2f* uvs, float* zbuffer, TGAImage& image, float intensity, TGAImage& diffuse) {
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }
    Vec3f P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(pts[0], pts[1], pts[2], P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;

            P.z = 0;
            Vec2f uv(0, 0);
            for (int i = 0; i < 3; i++)
            {
                P.z += pts[i][2] * bc_screen[i];
                uv = uv + uvs[i] * bc_screen[i];
            }
            TGAColor diff = linear_sample_texture(uv, diffuse);
            diff.a = 255;

            if (zbuffer[int(P.x + P.y * width)] < P.z) {
                zbuffer[int(P.x + P.y * width)] = P.z;
                image.set(P.x, P.y, diff * intensity);
            }
        }
    }
}

Vec3f world2screen(Vec3f v) {
    return Vec3f(int((v.x + 1.) * width / 2. ), int((v.y + 1.) * height / 2.), v.z);
}

int main(int argc, char** argv) {
    // load model and diffuse texture
    model = new Model("../../obj/african_head/african_head.obj");
    TGAImage head_diffuse;
    head_diffuse.read_tga_file("../../obj/african_head/african_head_diffuse.tga");

    // create and init zbuffer. current depth value haven't been transformed with perspective matrix.
    float* zbuffer = new float[width * height];
    for (int i = width * height - 1; i >= 0; --i)
        zbuffer[i] = -1;

    TGAImage image(width, height, TGAImage::RGB);
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Model::Face f = model->get_face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];
        Vec2f uvs[3];
        for (int i = 0; i < 3; i++)
        {
            screen_coords[i] = world2screen(model->vert(f.v[i]));
            world_coords[i] = model->vert(f.v[i]);
            uvs[i] = model->uv(f.vt[i]);
            uvs[i].y = 1 - uvs[i].y;
        }
        Vec3f n = cross((world_coords[2] - world_coords[0]), (world_coords[1] - world_coords[0]));
        n.normalize();
        float intensity = n * light_dir;
        if (intensity > 0)
            triangle(screen_coords, uvs, zbuffer, image, intensity, head_diffuse);
    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}