#include <vector>
#include <iostream>
#include <cmath>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const int width = 800;
const int height = 800;
const int depth = 255;
const float PI = 3.14159265358979323846;

Model* model = NULL;
float* zbuffer = NULL;
Vec3f light_dir = Vec3f(1, 1, 1).normalize();
Vec3f eye(0, 0, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

mat<4, 1, float> vec2mat(Vec3f v) {
    mat<4, 1, float> m;
	for (int i = 0; i < 3; i++) {
		m[i][0] = v[i];
	}
	m[3][0] = 1;
	return m;
}

Vec3f mat2vec(mat<4, 1, float> m) {
	Vec3f v;
	for (int i = 0; i < 3; i++) {
		v[i] = m[i][0] / m[3][0];
	}
	return v;
}

mat4 viewport(int x, int y, int w, int h) {
    mat4 m = mat4::identity();
    m[0][3] = x + w / 2.f;
    m[1][3] = y + h / 2.f;

    m[0][0] = w / 2.f;
    m[1][1] = h / 2.f;
    return m;
}

mat4 lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye - center).normalize();
    Vec3f x = cross(up, z).normalize();
    Vec3f y = cross(z, x).normalize();
    mat4 rotate = mat4::identity();
    mat4 translate = mat4::identity();
    for (int i = 0; i < 3; i++) {
        rotate[0][i] = x[i];
        rotate[1][i] = y[i];
        rotate[2][i] = z[i];
        translate[i][3] = -eye[i];
    }

    return rotate * translate;
}

mat4 my_perspective(float eye_fov, float aspect_ratio, float zNear, float zFar) {
    const auto eye_fov_rad = eye_fov / 180.0f * PI;
    const auto t = zNear * tan(eye_fov_rad / 2.0f); // top plane
    const auto r = t * aspect_ratio;    // right plane
    const auto l = -r;  // left plane
    const auto b = -t;  // bottom plane
    const auto n = -zNear; // near plane
    const auto f = -zFar;  // far plane

    mat4 proj = mat4::identity();
    mat4 ortho = mat4::identity();

    proj[0][0] = n;
    proj[1][1] = n;
    proj[2][2] = (n + f);
    proj[2][3] = -n * f;
    proj[3][2] = 1.f;
    proj[3][3] = 0.f;

    ortho[0][0] = 2.f / (r - l);
    ortho[1][1] = 2.f / (t - b);
    ortho[2][2] = 2.f / (n - f);
    ortho[2][3] = -(n + f) / (n - f);

    return ortho * proj;
}

mat4 perspective(float eye_fov, float aspect_ratio, float zNear, float zFar) {
    eye_fov = eye_fov * PI / 180;
    float fax = 1.0f / (float)tan(eye_fov * 0.5f);

    mat4 m = mat4::identity();
    m[0][0] = fax / aspect_ratio;
    m[1][1] = fax;
    m[2][2] = -(zFar + zNear) / (zFar - zNear);
    m[2][3] = -(2 * zFar * zNear) / (zFar - zNear);
    m[3][2] = -1;
    m[3][3] = 0;

    return m;
}

void triangle(Vec3f t0, Vec3f t1, Vec3f t2, float ity0, float ity1, float ity2, TGAImage& image, float* zbuffer) {
    if (t0.y == t1.y && t0.y == t2.y) return; // i dont care about degenerate triangles
    if (t0.y > t1.y) { std::swap(t0, t1); std::swap(ity0, ity1); }
    if (t0.y > t2.y) { std::swap(t0, t2); std::swap(ity0, ity2); }
    if (t1.y > t2.y) { std::swap(t1, t2); std::swap(ity1, ity2); }

    int total_height = t2.y - t0.y;
    for (int i = 0; i < total_height; i++) {
        bool second_half = i > t1.y - t0.y || t1.y == t0.y;
        int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
        float alpha = (float)i / total_height;
        float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height; // be careful: with above conditions no division by zero here
        Vec3f A = Vec3f(t0) + Vec3f(t2 - t0) * alpha;
        Vec3f B = second_half ? Vec3f(t1) + Vec3f(t2 - t1) * beta : Vec3f(t0) + Vec3f(t1 - t0) * beta;
        float ityA = ity0 + (ity2 - ity0) * alpha;
        float ityB = second_half ? ity1 + (ity2 - ity1) * beta : ity0 + (ity1 - ity0) * beta;
        if (A.x > B.x) { std::swap(A, B); std::swap(ityA, ityB); }
        for (int j = A.x; j <= B.x; j++) {
            float phi = B.x == A.x ? 1. : (float)(j - A.x) / (B.x - A.x);
            Vec3i    P = Vec3f(A) + Vec3f(B - A) * phi;
            float ityP = ityA + (ityB - ityA) * phi;
            int idx = P.x + P.y * width;
            if (P.x >= width || P.y >= height || P.x < 0 || P.y < 0) continue;
            if (zbuffer[idx] < P.z) {
                zbuffer[idx] = P.z;
                // std::cout << "P.z: " << P.z << std::endl;
                image.set(P.x, P.y, TGAColor(255, 255, 255, 255) * ityP);
            }
        }
    }
}

Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
    Vec3f s[2];
    for (int i = 2; i--; ) {
        s[i][0] = C[i] - A[i];
        s[i][1] = B[i] - A[i];
        s[i][2] = A[i] - P[i];
    }
    Vec3f u = cross(s[0], s[1]);
    if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

void triangle(Vec3f* pts, float ity0, float ity1, float ity2, TGAImage& image, float* zbuffer) {
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }
    Vec2i P;        
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(proj<2>(pts[0]), proj<2>(pts[1]), proj<2>(pts[2]), P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;

            float frag_depth = 0;
            float ityP = ity0 * bc_screen.x + ity1 * bc_screen.y + ity2 * bc_screen.z;
            for (int i = 0; i < 3; i++)
                frag_depth += pts[i][2] * bc_screen[i];

            if (zbuffer[P.x + P.y * width] > frag_depth) {   // !!! x and y of P must be integer to get the correct index of zbuffer
                zbuffer[P.x + P.y * width] = frag_depth;
                image.set(P.x, P.y, TGAColor(255, 255, 255, 255) * ityP);
            }
        }
    }
}

float my_NDCDepth2LinearDepth(float z_ndc, float zNear, float zFar) {
    float n = -zNear;
    float f = -zFar;
	float linear_depth = -2 * n * f / (z_ndc * (n - f) - f - n);
    float normalized_depth = (n - linear_depth) / (n - f);

    return normalized_depth;
}

float NDCDepth2LinearDepth(float z_ndc, float zNear, float zFar) {
    float n = zNear;
    float f = zFar;
    float linear_depth = -2 * n * f / (z_ndc * (f - n) - f - n);
    float normalized_depth = (linear_depth - n) / (f - n);

    return normalized_depth;
}

int main(int argc, char** argv) {
    if (2 == argc) {
        model = new Model(argv[1]);
    }
    else {
        model = new Model("../../obj/african_head/african_head.obj");
    }

    zbuffer = new float[width * height];
    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = 1.f;
    }

    { // draw the model
        mat4 ModelView = lookat(eye, center, up);
        mat4 proj = perspective(45, width / height, 0.1, 10);
        mat4 vp = viewport(0, 0, width, height);

        std::cerr << ModelView << std::endl;
        std::cerr << proj << std::endl;
        std::cerr << vp << std::endl;

        TGAImage image(width, height, TGAImage::RGB);
        for (int i = 0; i < model->nfaces(); i++) {
            std::vector<int> face = model->face(i);
            Vec3f screen_coords[3];
            Vec3f world_coords[3];
            float intensity[3];
            for (int j = 0; j < 3; j++) {
                Vec3f v = model->vert(face[j]);
                Vec4f gl_Pos = vp * proj * ModelView * embed<4>(v, 1.f);
                screen_coords[j] = Vec3f(gl_Pos[0] / gl_Pos[3], gl_Pos[1] / gl_Pos[3], gl_Pos[2] / gl_Pos[3]);
                world_coords[j] = v;
                intensity[j] = std::max(0.f, model->normal(i, j) * light_dir);
            }
            triangle(screen_coords, intensity[0], intensity[1], intensity[2], image, zbuffer);
            // triangle(screen_coords[0], screen_coords[1], screen_coords[2], intensity[0], intensity[1], intensity[2], image, zbuffer);
        }
        image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        image.write_tga_file("output.tga");
    }

    { // dump z-buffer (debugging purposes only)
        TGAImage zbimage(width, height, TGAImage::GRAYSCALE);
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                float depth = NDCDepth2LinearDepth(zbuffer[i + j * width], 0.1f, 10.f);
                // std::cout << zbuffer[i + j * width] << ", " << depth << std::endl;
                zbimage.set(i, j, TGAColor(unsigned char(depth * 255)));
            }
        }
        zbimage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
        zbimage.write_tga_file("zbuffer.tga");
    }
    delete model;
    delete[] zbuffer;
    return 0;
}