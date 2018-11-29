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

inline int align(int value, int alignment)
{
	return (value + alignment - 1)&~(alignment - 1);
}


struct Vec4
{
public:
	Vec4(const Vec4& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
	Vec4() {}
	Vec4(float _x, float _y, float _z = 0.f, float _w = 0.f) : x(_x), y(_y), z(_z), w(_w)
	{
	}
	Vec4(int _x, int _y, int _z = 0, int _w = 0) : x((float)_x), y((float)_y), z((float)_z), w((float)_w)
	{
	}

	//Vec4(u32 col) { fromUInt32(col); }
	Vec4(float v) : x(v), y(v), z(v), w(v) {}

	float x, y, z, w;

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

	inline void set(float v) { x = y = z = w = v; }
	inline void set(float _x, float _y, float _z = 0.f, float _w = 0.f) { x = _x; y = _y; z = _z; w = _w; }

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
	inline float length() const { return sqrtf(x*x + y*y + z*z); };
	inline float lengthSq() const { return (x*x + y*y + z*z); };
	inline Vec4 normalize() { (*this) *= (1.f / length() + FLT_EPSILON); return (*this); }
	inline Vec4 normalize(const Vec4& v) { this->set(v.x, v.y, v.z, v.w); this->normalize(); return (*this); }
	inline int LongestAxis() const
	{
		int res = 0;
		res = (fabsf((*this)[1]) > fabsf((*this)[res])) ? 1 : res;
		res = (fabsf((*this)[2]) > fabsf((*this)[res])) ? 2 : res;
		return res;
	}
	inline void cross(const Vec4& v)
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
	inline void cross(const Vec4& v1, const Vec4& v2)
	{
		x = v1.y * v2.z - v1.z * v2.y;
		y = v1.z * v2.x - v1.x * v2.z;
		z = v1.x * v2.y - v1.y * v2.x;
		w = 0.f;
	}
	inline float dot(const Vec4 &v) const
	{
		return (x * v.x) + (y * v.y) + (z * v.z) + (w * v.w);
	}

	void isMaxOf(const Vec4& v)
	{
		x = (v.x>x) ? v.x : x;
		y = (v.y>y) ? v.y : y;
		z = (v.z>z) ? v.z : z;
		w = (v.w>w) ? v.z : w;
	}
	void isMinOf(const Vec4& v)
	{
		x = (v.x>x) ? x : v.x;
		y = (v.y>y) ? y : v.y;
		z = (v.z>z) ? z : v.z;
		w = (v.w>w) ? z : v.w;
	}

	bool isInside(const Vec4& min, const Vec4& max) const
	{
		if (min.x > x || max.x < x ||
			min.y > y || max.y < y ||
			min.z > z || max.z < z)
			return false;
		return true;
	}

	Vec4 symetrical(const Vec4& v) const
	{
		Vec4 res;
		float dist = signedDistanceTo(v);
		res = v;
		res -= (*this)*dist*2.f;

		return res;
	}
	/*void transform(const matrix_t& matrix);
	void transform(const Vec4 & s, const matrix_t& matrix);

	void TransformVector(const matrix_t& matrix);
	void TransformPoint(const matrix_t& matrix);
	void TransformVector(const Vec4& v, const matrix_t& matrix) { (*this) = v; this->TransformVector(matrix); }
	void TransformPoint(const Vec4& v, const matrix_t& matrix) { (*this) = v; this->TransformPoint(matrix); }
	*/
	// quaternion slerp
	//void slerp(const Vec4 &q1, const Vec4 &q2, float t );

	inline float signedDistanceTo(const Vec4& point) const;
	//Vec4 interpolateHermite(const Vec4 &nextKey, const Vec4 &nextKeyP1, const Vec4 &prevKey, float ratio) const;
	static float d(const Vec4& v1, const Vec4& v2) { return (v1 - v2).length(); }
	static float d2(const Vec4& v1, const Vec4& v2) { return (v1 - v2).lengthSq(); }

	//static Vec4 zero;
	/*
	uint16 toUInt5551() const { return (uint16_t)(((int)(w*1.f) << 15) + ((int)(z*31.f) << 10) + ((int)(y*31.f) << 5) + ((int)(x*31.f))); }
	void fromUInt5551(unsigned short v) {
		w = (float)((v & 0x8000) >> 15); z = (float)((v & 0x7C00) >> 10) * (1.f / 31.f);
		y = (float)((v & 0x3E0) >> 5) * (1.f / 31.f); x = (float)((v & 0x1F)) * (1.f / 31.f);
	}
	
	uint32_t toUInt32() const { return ((int)(w*255.f) << 24) + ((int)(z*255.f) << 16) + ((int)(y*255.f) << 8) + ((int)(x*255.f)); }
	void fromUInt32(uint32_t v) {
		w = (float)((v & 0xFF000000) >> 24) * (1.f / 255.f); z = (float)((v & 0xFF0000) >> 16) * (1.f / 255.f);
		y = (float)((v & 0xFF00) >> 8) * (1.f / 255.f); x = (float)((v & 0xFF)) * (1.f / 255.f);
	}
	*/
	Vec4 swapedRB() const;
	float& operator [] (size_t index) { return ((float*)&x)[index]; }
	const float& operator [] (size_t index) const { return ((float*)&x)[index]; }
};

inline Vec4 Vec4::operator * (float f) const { return Vec4(x * f, y * f, z * f, w *f); }
inline Vec4 Vec4::operator - () const { return Vec4(-x, -y, -z, -w); }
inline Vec4 Vec4::operator - (const Vec4& v) const { return Vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
inline Vec4 Vec4::operator + (const Vec4& v) const { return Vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
inline Vec4 Vec4::operator * (const Vec4& v) const { return Vec4(x * v.x, y * v.y, z * v.z, w * v.w); }
inline float Vec4::signedDistanceTo(const Vec4& point) const { return (point.dot(Vec4(x, y, z))) - w; }

inline Vec4 normalized(const Vec4& v) { Vec4 res; res = v; res.normalize(); return res; }
inline Vec4 cross(const Vec4& v1, const Vec4& v2)
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
