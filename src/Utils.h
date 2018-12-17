// https://github.com/CedricGuillemet/Imogen
//
// The MIT License(MIT)
// 
// Copyright(c) 2018 Cedric Guillemet
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#pragma once

#include <string>
#include <float.h>

typedef unsigned int TextureID;
static const int SemUV0 = 0;

class FullScreenTriangle
{
public:
	FullScreenTriangle() : mGLFullScreenVertexArrayName(-1)
	{
	}
	~FullScreenTriangle()
	{
	}
	void Init();
	void Render();
protected:
	TextureID mGLFullScreenVertexArrayName;
};


void TexParam(TextureID MinFilter, TextureID MagFilter, TextureID WrapS, TextureID WrapT, TextureID texMode);

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);

unsigned int LoadShader(const std::string &shaderString, const char *fileName);
int Log(const char *szFormat, ...);
template<typename T> T Lerp(T a, T b, float t) { return T(a + (b - a) * t); }

inline int align(int value, int alignment)
{
	return (value + alignment - 1)&~(alignment - 1);
}

struct Mat4x4;

struct iVec2
{
	int x, y;
	inline iVec2 operator * (float f) const { return { int(x*f), int(y*f) }; }
	inline iVec2 operator - (const iVec2& v) const { return { x - v.x, y - v.y }; }
	inline iVec2 operator + (const iVec2& v) const { return { x + v.x, y + v.y }; }
	int& operator [] (size_t index) { return ((int*)&x)[index]; }
	const int& operator [] (size_t index) const { return ((int*)&x)[index]; }
};

struct iVec3
{
	int x, y, z;
	inline iVec3 operator * (float f) const { return { int(x*f), int(y*f), int(z*f) }; }
	inline iVec3 operator - (const iVec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
	inline iVec3 operator + (const iVec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
	int& operator [] (size_t index) { return ((int*)&x)[index]; }
	const int& operator [] (size_t index) const { return ((int*)&x)[index]; }
};

struct iVec4
{
	int x, y, z, w;
	inline iVec4 operator * (float f) const { return { int(x*f), int(y*f), int(z*f), int(w*f) }; }
	inline iVec4 operator - (const iVec4& v) const { return { x - v.x, y - v.y, z - v.z, w - v.w }; }
	inline iVec4 operator + (const iVec4& v) const { return { x + v.x, y + v.y, z + v.z, w + v.w }; }
	int& operator [] (size_t index) { return ((int*)&x)[index]; }
	const int& operator [] (size_t index) const { return ((int*)&x)[index]; }
};

struct Vec2
{
	float x, y;
	inline Vec2 operator * (float f) const { return { x*f, y*f }; }
	inline Vec2 operator - (const Vec2& v) const { return { x - v.x, y - v.y }; }
	inline Vec2 operator + (const Vec2& v) const { return { x + v.x, y + v.y }; }
	float& operator [] (size_t index) { return ((float*)&x)[index]; }
	const float& operator [] (size_t index) const { return ((float*)&x)[index]; }
};

struct Vec3
{
	float x, y, z;
	inline Vec3 operator * (float f) const { return { x*f, y*f, z*f }; }
	inline Vec3 operator - (const Vec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
	inline Vec3 operator + (const Vec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
	float& operator [] (size_t index) { return ((float*)&x)[index]; }
	const float& operator [] (size_t index) const { return ((float*)&x)[index]; }
};

struct Vec4
{
public:
	Vec4(const Vec4& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
	Vec4() {}
	Vec4(float _x, float _y, float _z = 0.f, float _w = 0.f) : x(_x), y(_y), z(_z), w(_w) {	}
	Vec4(int _x, int _y, int _z = 0, int _w = 0) : x((float)_x), y((float)_y), z((float)_z), w((float)_w) {	}
	Vec4(float v) : x(v), y(v), z(v), w(v) { }

	void Lerp(const Vec4& v, float t)
	{
		x += (v.x - x) * t;
		y += (v.y - y) * t;
		z += (v.z - z) * t;
		w += (v.w - w) * t;
	}
	void LerpColor(const Vec4& v, float t)
	{
		for (int i = 0; i < 4; i++)
			(*this)[i] = sqrtf(((*this)[i] * (*this)[i]) * (1.f - t) + (v[i] * v[i]) * (t));
	}
	void Lerp(const Vec4& v, const Vec4& v2, float t)
	{
		*this = v;
		Lerp(v2, t);
	}

	inline void Set(float v) { x = y = z = w = v; }
	inline void Set(float _x, float _y, float _z = 0.f, float _w = 0.f) { x = _x; y = _y; z = _z; w = _w; }

	inline Vec4& operator -= (const Vec4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	inline Vec4& operator += (const Vec4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	inline Vec4& operator *= (const Vec4& v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
	inline Vec4& operator *= (float v) { x *= v;	y *= v;	z *= v;	w *= v;	return *this; }

	inline Vec4 operator * (float f) const;
	inline Vec4 operator - () const;
	inline Vec4 operator - (const Vec4& v) const;
	inline Vec4 operator + (const Vec4& v) const;
	inline Vec4 operator * (const Vec4& v) const;

	inline const Vec4& operator + () const { return (*this); }
	inline float Length() const { return sqrtf(x*x + y*y + z*z); };
	inline float LengthSq() const { return (x*x + y*y + z*z); };
	inline Vec4 Normalize() { (*this) *= (1.f / Length() + FLT_EPSILON); return (*this); }
	inline Vec4 Normalize(const Vec4& v) { this->Set(v.x, v.y, v.z, v.w); this->Normalize(); return (*this); }
	inline int LongestAxis() const
	{
		int res = 0;
		res = (fabsf((*this)[1]) > fabsf((*this)[res])) ? 1 : res;
		res = (fabsf((*this)[2]) > fabsf((*this)[res])) ? 2 : res;
		return res;
	}
	inline void Cross(const Vec4& v)
	{
		Vec4 res;
		res.x = y * v.z - z * v.y;
		res.y = z * v.x - x * v.z;
		res.z = x * v.y - y * v.x;

		x = res.x;
		y = res.y;
		z = res.z;
		w = 0.f;
	}
	inline void Cross(const Vec4& v1, const Vec4& v2)
	{
		x = v1.y * v2.z - v1.z * v2.y;
		y = v1.z * v2.x - v1.x * v2.z;
		z = v1.x * v2.y - v1.y * v2.x;
		w = 0.f;
	}
	inline float Dot(const Vec4 &v) const
	{
		return (x * v.x) + (y * v.y) + (z * v.z) + (w * v.w);
	}

	void IsMaxOf(const Vec4& v)
	{
		x = (v.x>x) ? v.x : x;
		y = (v.y>y) ? v.y : y;
		z = (v.z>z) ? v.z : z;
		w = (v.w>w) ? v.z : w;
	}
	void IsMinOf(const Vec4& v)
	{
		x = (v.x>x) ? x : v.x;
		y = (v.y>y) ? y : v.y;
		z = (v.z>z) ? z : v.z;
		w = (v.w>w) ? z : v.w;
	}

	bool IsInside(const Vec4& min, const Vec4& max) const
	{
		if (min.x > x || max.x < x ||
			min.y > y || max.y < y ||
			min.z > z || max.z < z)
			return false;
		return true;
	}

	Vec4 Symetrical(const Vec4& v) const
	{
		Vec4 res;
		float dist = SignedDistanceTo(v);
		res = v;
		res -= (*this)*dist*2.f;

		return res;
	}
	/*void transform(const Mat4x4& matrix);
	void transform(const Vec4 & s, const Mat4x4& matrix);
	*/
	void TransformVector(const Mat4x4& matrix);
	void TransformPoint(const Mat4x4& matrix);

	void TransformVector(const Vec4& v, const Mat4x4& matrix) { (*this) = v; this->TransformVector(matrix); }
	void TransformPoint(const Vec4& v, const Mat4x4& matrix) { (*this) = v; this->TransformPoint(matrix); }
	
	// quaternion slerp
	//void slerp(const Vec4 &q1, const Vec4 &q2, float t );

	inline float SignedDistanceTo(const Vec4& point) const;
	float& operator [] (size_t index) { return ((float*)&x)[index]; }
	const float& operator [] (size_t index) const { return ((float*)&x)[index]; }

	float x, y, z, w;
};

inline Vec4 Vec4::operator * (float f) const { return Vec4(x * f, y * f, z * f, w *f); }
inline Vec4 Vec4::operator - () const { return Vec4(-x, -y, -z, -w); }
inline Vec4 Vec4::operator - (const Vec4& v) const { return Vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
inline Vec4 Vec4::operator + (const Vec4& v) const { return Vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
inline Vec4 Vec4::operator * (const Vec4& v) const { return Vec4(x * v.x, y * v.y, z * v.z, w * v.w); }
inline float Vec4::SignedDistanceTo(const Vec4& point) const { return (point.Dot(Vec4(x, y, z))) - w; }

inline Vec4 Normalized(const Vec4& v) { Vec4 res; res = v; res.Normalize(); return res; }
inline Vec4 Cross(const Vec4& v1, const Vec4& v2)
{
	Vec4 res;
	res.x = v1.y * v2.z - v1.z * v2.y;
	res.y = v1.z * v2.x - v1.x * v2.z;
	res.z = v1.x * v2.y - v1.y * v2.x;
	res.w = 0.f;
	return res;
}

inline float Dot(const Vec4 &v1, const Vec4 &v2)
{
	return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
}


inline void FPU_MatrixF_x_MatrixF(const float *a, const float *b, float *r)
{
	r[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
	r[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
	r[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
	r[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

	r[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
	r[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
	r[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
	r[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

	r[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
	r[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
	r[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
	r[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

	r[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
	r[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
	r[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
	r[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
}


struct Mat4x4
{
public:
	union
	{
		float m[4][4];
		float m16[16];
		struct
		{
			Vec4 right, up, dir, position;
		};
	};

	Mat4x4(float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8, float v9, float v10, float v11, float v12, float v13, float v14, float v15, float v16)
	{
		m16[0] = v1;
		m16[1] = v2;
		m16[2] = v3;
		m16[3] = v4;
		m16[4] = v5;
		m16[5] = v6;
		m16[6] = v7;
		m16[7] = v8;
		m16[8] = v9;
		m16[9] = v10;
		m16[10] = v11;
		m16[11] = v12;
		m16[12] = v13;
		m16[13] = v14;
		m16[14] = v15;
		m16[15] = v16;
	}
	Mat4x4(const Mat4x4& other) { memcpy(&m16[0], &other.m16[0], sizeof(float) * 16); }
	Mat4x4(const Vec4 & r, const Vec4 &u, const Vec4& d, const Vec4& p) { set(r, u, d, p); }
	Mat4x4() {}
	void set(const Vec4 & r, const Vec4 &u, const Vec4& d, const Vec4& p) { right = r; up = u; dir = d; position = p; }
	void set(float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8, float v9, float v10, float v11, float v12, float v13, float v14, float v15, float v16)
	{
		m16[0] = v1;
		m16[1] = v2;
		m16[2] = v3;
		m16[3] = v4;
		m16[4] = v5;
		m16[5] = v6;
		m16[6] = v7;
		m16[7] = v8;
		m16[8] = v9;
		m16[9] = v10;
		m16[10] = v11;
		m16[11] = v12;
		m16[12] = v13;
		m16[13] = v14;
		m16[14] = v15;
		m16[15] = v16;
	}
	static Mat4x4 GetIdentity() {
		return Mat4x4(1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f);
	}
	operator float * () { return m16; }
	operator const float* () const { return m16; }
	void Translation(float _x, float _y, float _z) { this->Translation(Vec4(_x, _y, _z)); }

	void Translation(const Vec4& vt)
	{
		right.Set(1.f, 0.f, 0.f, 0.f);
		up.Set(0.f, 1.f, 0.f, 0.f);
		dir.Set(0.f, 0.f, 1.f, 0.f);
		position.Set(vt.x, vt.y, vt.z, 1.f);
	}
	void TranslationScale(const Vec4& vt, const Vec4& scale)
	{
		right.Set(scale.x, 0.f, 0.f, 0.f);
		up.Set(0.f, scale.y, 0.f, 0.f);
		dir.Set(0.f, 0.f, scale.z, 0.f);
		position.Set(vt.x, vt.y, vt.z, 1.f);
	}

	inline void RotationY(const float angle)
	{
		float c = cosf(angle);
		float s = sinf(angle);

		right.Set(c, 0.f, -s, 0.f);
		up.Set(0.f, 1.f, 0.f, 0.f);
		dir.Set(s, 0.f, c, 0.f);
		position.Set(0.f, 0.f, 0.f, 1.f);
	}

	inline void RotationX(const float angle)
	{
		float c = cosf(angle);
		float s = sinf(angle);

		right.Set(1.f, 0.f, 0.f, 0.f);
		up.Set(0.f, c, s, 0.f);
		dir.Set(0.f, -s, c, 0.f);
		position.Set(0.f, 0.f, 0.f, 1.f);
	}

	inline void RotationZ(const float angle)
	{
		float c = cosf(angle);
		float s = sinf(angle);

		right.Set(c, s, 0.f, 0.f);
		up.Set(-s, c, 0.f, 0.f);
		dir.Set(0.f, 0.f, 1.f, 0.f);
		position.Set(0.f, 0.f, 0, 1.f);
	}
	inline void Scale(float _s)
	{
		right.Set(_s, 0.f, 0.f, 0.f);
		up.Set(0.f, _s, 0.f, 0.f);
		dir.Set(0.f, 0.f, _s, 0.f);
		position.Set(0.f, 0.f, 0.f, 1.f);
	}
	inline void Scale(float _x, float _y, float _z)
	{
		right.Set(_x, 0.f, 0.f, 0.f);
		up.Set(0.f, _y, 0.f, 0.f);
		dir.Set(0.f, 0.f, _z, 0.f);
		position.Set(0.f, 0.f, 0.f, 1.f);
	}
	inline void Scale(const Vec4& s) { Scale(s.x, s.y, s.z); }

	inline Mat4x4& operator *= (const Mat4x4& mat)
	{
		Mat4x4 tmpMat;
		tmpMat = *this;
		tmpMat.Multiply(mat);
		*this = tmpMat;
		return *this;
	}
	inline Mat4x4 operator * (const Mat4x4& mat) const
	{
		Mat4x4 matT;
		matT.Multiply(*this, mat);
		return matT;
	}

	inline void Multiply(const Mat4x4 &matrix)
	{
		Mat4x4 tmp;
		tmp = *this;

		FPU_MatrixF_x_MatrixF((float*)&tmp, (float*)&matrix, (float*)this);
	}

	inline void Multiply(const Mat4x4 &m1, const Mat4x4 &m2)
	{
		FPU_MatrixF_x_MatrixF((float*)&m1, (float*)&m2, (float*)this);
	}

	void glhPerspectivef2(float fovyInDegrees, float aspectRatio, float znear, float zfar);
	void glhFrustumf2(float left, float right, float bottom, float top, float znear, float zfar);
	void PerspectiveFovLH2(const float fovy, const float aspect, const float zn, const float zf);
	void OrthoOffCenterLH(const float l, float r, float b, const float t, float zn, const float zf);
	void lookAtRH(const Vec4 &eye, const Vec4 &at, const Vec4 &up);
	void lookAtLH(const Vec4 &eye, const Vec4 &at, const Vec4 &up);
	void LookAt(const Vec4 &eye, const Vec4 &at, const Vec4 &up);
	void rotationQuaternion(const Vec4 &q);

	inline float GetDeterminant() const
	{
		return m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[0][2] * m[1][0] * m[2][1] -
			m[0][2] * m[1][1] * m[2][0] - m[0][1] * m[1][0] * m[2][2] - m[0][0] * m[1][2] * m[2][1];
	}

	float Inverse(const Mat4x4 &srcMatrix, bool affine = false);
	float Inverse(bool affine = false);
	void Identity() {
		right.Set(1.f, 0.f, 0.f, 0.f);
		up.Set(0.f, 1.f, 0.f, 0.f);
		dir.Set(0.f, 0.f, 1.f, 0.f);
		position.Set(0.f, 0.f, 0.f, 1.f);
	}
	inline void transpose()
	{
		Mat4x4 tmpm;
		for (int l = 0; l < 4; l++)
		{
			for (int c = 0; c < 4; c++)
			{
				tmpm.m[l][c] = m[c][l];
			}
		}
		(*this) = tmpm;
	}
	void RotationAxis(const Vec4 & axis, float angle);
	/*
	void Lerp(const Mat4x4& r, const Mat4x4& t, float s)
	{
		right = LERP(r.right, t.right, s);
		up = LERP(r.up, t.up, s);
		dir = LERP(r.dir, t.dir, s);
		position = LERP(r.position, t.position, s);
	}
	*/
	void RotationYawPitchRoll(const float yaw, const float pitch, const float roll);

	inline void OrthoNormalize()
	{
		right.Normalize();
		up.Normalize();
		dir.Normalize();
	}
};


inline void Vec4::TransformVector(const Mat4x4& matrix)
{
	Vec4 out;

	out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0];
	out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1];
	out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2];
	out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3];

	x = out.x;
	y = out.y;
	z = out.z;
	w = out.w;
}

inline void Vec4::TransformPoint(const Mat4x4& matrix)
{
	Vec4 out;

	out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0] + matrix.m[3][0];
	out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1] + matrix.m[3][1];
	out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2] + matrix.m[3][2];
	out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3] + matrix.m[3][3];

	x = out.x;
	y = out.y;
	z = out.z;
	w = out.w;
}


inline void Mat4x4::RotationAxis(const Vec4 & axis, float angle)
{
	float length2 = axis.LengthSq();
	if (length2 < FLT_EPSILON)
	{
		Identity();
		return;
	}

	Vec4 n = axis * (1.f / sqrtf(length2));
	float s = sinf(angle);
	float c = cosf(angle);
	float k = 1.f - c;

	float xx = n.x * n.x * k + c;
	float yy = n.y * n.y * k + c;
	float zz = n.z * n.z * k + c;
	float xy = n.x * n.y * k;
	float yz = n.y * n.z * k;
	float zx = n.z * n.x * k;
	float xs = n.x * s;
	float ys = n.y * s;
	float zs = n.z * s;

	m[0][0] = xx;
	m[0][1] = xy + zs;
	m[0][2] = zx - ys;
	m[0][3] = 0.f;
	m[1][0] = xy - zs;
	m[1][1] = yy;
	m[1][2] = yz + xs;
	m[1][3] = 0.f;
	m[2][0] = zx + ys;
	m[2][1] = yz - xs;
	m[2][2] = zz;
	m[2][3] = 0.f;
	m[3][0] = 0.f;
	m[3][1] = 0.f;
	m[3][2] = 0.f;
	m[3][3] = 1.f;
}
