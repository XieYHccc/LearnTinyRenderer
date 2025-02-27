#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model_lesson3.h"

Model::Model(const char* filename) : verts_(), faces_() {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i = 0; i < 3; i++) iss >> v[i];
            verts_.push_back(v);
        }
        else if (!line.compare(0, 3, "vt "))
        {
            iss >> trash;
            iss >> trash;
            Vec2f uv;
            for (int i = 0; i < 2; i++) iss >> uv[i];
            uvs_.push_back(uv);
        }
        else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            Face face;
            int itrash, idx, uv_idx;
            iss >> trash;
            int i = 0;
            while (iss >> idx >> trash >> uv_idx >> trash >> itrash) {
                idx--; // in wavefront obj all indices start at 1, not zero
                uv_idx--;
                f.push_back(idx);
                face.v[i] = idx;
                face.vt[i] = uv_idx;
                i++;
            }
            faces_.push_back(f);
            m_faces.push_back(face);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << std::endl;
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
    return faces_[idx];
}

Model::Face Model::get_face(int idx)
{
    assert(idx < m_faces.size());
    return m_faces[idx];

}

Vec2f Model::uv(int i)
{
    return uvs_[i];
}


Vec3f Model::vert(int i) {
    return verts_[i];
}