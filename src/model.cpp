#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include "model.h"

Model::Model(const char *filename) : verts_(), faces_() {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) iss >> v.raw[i];
            verts_.push_back(v);
        }
        else if(!line.compare(0, 2, "vt")){
            iss >> trash >> trash;
            Vec2f vt;
            for (int i =0;i<2;i++) iss >>vt.raw[i];
            verts_texture_.push_back(vt);
        } 
        else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            std::vector<int> ft;
            int itrash, v_idx, vt_idx;
            iss >> trash;
            while (iss >> v_idx >> trash >> vt_idx >> trash >> itrash) {
                v_idx--; // in wavefront obj all indices start at 1, not zero
                vt_idx--;
                f.push_back(v_idx);
                ft.push_back(vt_idx);
            }
            faces_.push_back(f);
            ft_.push_back(ft);

        }
    }
    std::cerr << "# v# " << verts_.size() << " f# "  << faces_.size() << std::endl;
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

Vec3f Model::vert(int i) {
    return verts_[i];
}

Vec2f Model::vt(int i) {
    return verts_texture_[i];
}

std::vector<int> Model::face_tex(int idx){
    return ft_[idx];
}

