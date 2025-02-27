#include "geometry.h"

template <> template <> Vec3i::vec(const Vec3f& v) : x(int(v.x)), y(int(v.y)), z(int(v.z)) {}
template <> template <> Vec3f::vec(const Vec3i& v) : x(v.x), y(v.y), z(v.z) {}