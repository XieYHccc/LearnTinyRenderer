#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model {
public:
	struct Face {
		int v[3];
		int vt[3];
		int vn[3];
	};
	Model(const char* filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int i);
	std::vector<int> face(int idx);
	Face get_face(int idx);
	Vec2f uv(int i);
private:
	std::vector<Vec3f> verts_;
	std::vector<Vec2f> uvs_;
	std::vector<std::vector<int>> faces_;
	std::vector<Face> m_faces;
};

#endif //__MODEL_H__

