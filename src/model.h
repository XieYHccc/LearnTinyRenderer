#pragma once
#include <vector>
#include "vector.h"

class Model 
{
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nfaces();
	vec3 vert(int i);
	vec2 vt(int i );
	std::vector<int> face(int idx);
	std::vector<int> face_tex(int idx);

private:
	std::vector<vec3> verts_;
	std::vector<vec2> verts_texture_;
	std::vector<std::vector<int> > faces_;
	std::vector<std::vector<int> > ft_;
};

