#include "our_gl.h"

const float PI = 3.14159265358979323846;

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

mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar)
{
    mat4 m = mat4::identity();
    m[0][0] = 2.f / (right - left);
    m[1][1] = 2.f / (top - bottom);
    m[2][2] = -2.f / (zFar - zNear);
    m[0][3] = -(right + left) / (right - left);
    m[1][3] = -(top + bottom) / (top - bottom);
    m[2][3] = -(zFar + zNear) / (zFar - zNear);
    return m;
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

void triangle(Vec4f* pts, IShader& shader, TGAImage& image, TGAImage& zbuffer)
{
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j] / pts[i][3]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j] / pts[i][3]));
        }
    }

    Vec2i P;
    TGAColor color;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f c = barycentric(proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3]), proj<2>(P));
            float z = pts[0][2] * c.x + pts[1][2] * c.y + pts[2][2] * c.z;
            float w = pts[0][3] * c.x + pts[1][3] * c.y + pts[2][3] * c.z;
            float frag_depth = std::max(0.f, std::min(1.f, (0.5f * z / w + .5f)));
            if (c.x < 0 || c.y < 0 || c.z < 0 || frag_depth > zbuffer.get(P.x, P.y).val) continue;
            bool discard = shader.fragment(c, color);
            if (!discard) {
                zbuffer.set(P.x, P.y, TGAColor(frag_depth));
                float a = zbuffer.get(P.x, P.y).val;
                image.set(P.x, P.y, color);
            }
        }
    }
}

void triangle(Vec4f* pts, IShader& shader, TGAImage& image, float* zbuffer)
{
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j] / pts[i][3]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j] / pts[i][3]));
        }
    }

    Vec2i P;
    TGAColor color;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f c = barycentric(proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3]), proj<2>(P));
            float z = pts[0][2] * c.x + pts[1][2] * c.y + pts[2][2] * c.z;
            float w = pts[0][3] * c.x + pts[1][3] * c.y + pts[2][3] * c.z;
            float frag_depth = std::max(0.f, std::min(1.f, (0.5f * z / w + .5f)));
            if (c.x < 0 || c.y < 0 || c.z < 0 || frag_depth > zbuffer[int(P.x +  P.y * image.get_width())]) continue;
            bool discard = shader.fragment(c, color);
            if (!discard) {
                zbuffer[P.x + P.y * image.get_width()] = frag_depth;
                float a = zbuffer[P.x + P.y * image.get_width()];
                image.set(P.x, P.y, color);
            }
        }
    }
}
