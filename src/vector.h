#pragma once	

#include <cmath>

template <typename Scalar, int N> 
class Vector 
{
public :
	using value_type = Scalar;

	//! returns the dimension of the vector (or size of the matrix, rows*cols)
    static constexpr int size() { return N; }

	//! default constructor
	Vector() = default;

	//! constructor for 2D vector
	Vector(Scalar x0, Scalar x1) { data_[0] = x0; data_[1] = x1;}

	//! constructor for 3D vector
	Vector(Scalar x0, Scalar x1, Scalar x2) { data_[0] = x0; data_[1] = x1; data_[2] = x2;}

	Scalar& operator[] (unsigned int i) {return data_[i];}

	Scalar operator[] (unsigned int i) const {return data_[i];}

	Vector<Scalar, N>& operator +=(const Vector<Scalar, N> &v)
	{ 
		for(int i =0 ; i < size(); ++i)
			data_[i] += v.data_[i];
		return *this;

	}

	Vector<Scalar, N>& operator -=(const Vector<Scalar, N> &v)
	{
		for(int i =0 ; i < size(); ++i)
			data_[i] -= v.data_[i];
		return *this;
	}

	Vector<Scalar, N>& operator *=(Scalar t)
	{ 
		for(int i =0 ; i < size(); ++i)
			data_[i] *= t;
		return *this;
	}

	void normalize()
    {
        Scalar n = norm(*this);
        n = (n > std::numeric_limits<Scalar>::min()) ? Scalar(1.0) / n
                                                     : Scalar(0.0);
        *this *= n;
    }
private:
	Scalar data_[N];
};

using vec2 = Vector<float, 2>;
using vec3 = Vector<float, 3>;
using ivec2 = Vector<int, 2>;
using ivec3 = Vector<int, 3>;
using dvec2 = Vector<double, 2>;
using dvec3 = Vector<double, 3>;

template<typename Scalar, int N>
inline Scalar sqrnorm(const Vector<Scalar, N>& v)
{
	Scalar s{0.0};
	for(int i = 0; i < v.size(); ++i)
		s += v[i] * v[i];
	return s;
}

template<typename Scalar, int N>
inline Scalar norm(const Vector<Scalar, N>& v)
{
	return sqrt(sqrnorm(v));
}

template<typename Scalar, int N>
inline Vector<Scalar, N> normalize(const Vector<Scalar, N>& v)
{
	Scalar n = norm(v);
    n = (n > std::numeric_limits<Scalar>::min()) ? Scalar(1.0) / n
                                                 : Scalar(0.0);
    return v * n;
}