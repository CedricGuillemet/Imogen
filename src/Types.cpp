// https://github.com/CedricGuillemet/Imogen
//
// The MIT License(MIT)
//
// Copyright(c) 2019 Cedric Guillemet
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

#include <math.h>
#include "Types.h"
#include "bgfx/defines.h"
#include "Utils.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// matrix will receive the calculated perspective matrix.
// You would have to upload to your shader
// or use glLoadMatrixf if you aren't using shaders.
void Mat4x4::glhPerspectivef2(float fovyInDegrees, float aspectRatio, float znear, float zfar)
{
    float ymax, xmax;
    ymax = znear * tanf(fovyInDegrees * 3.14159f / 360.0f);
    xmax = ymax * aspectRatio;
    glhFrustumf2(-xmax, xmax, -ymax, ymax, znear, zfar);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Mat4x4::glhFrustumf2(float left, float right, float bottom, float top, float znear, float zfar)
{
    float temp, temp2, temp3, temp4;
    temp = 2.0f * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;
    m16[0] = temp / temp2;
    m16[1] = 0.0;
    m16[2] = 0.0;
    m16[3] = 0.0;
    m16[4] = 0.0;
    m16[5] = temp / temp3;
    m16[6] = 0.0;
    m16[7] = 0.0;
    m16[8] = (right + left) / temp2;
    m16[9] = (top + bottom) / temp3;
    m16[10] = (-zfar - znear) / temp4;
    m16[11] = -1.0f;
    m16[12] = 0.0;
    m16[13] = 0.0;
    m16[14] = (-temp * zfar) / temp4;
    m16[15] = 0.0;
}

void Mat4x4::lookAtRH(const Vec4& eye, const Vec4& at, const Vec4& up)
{
    Vec4 X, Y, Z, tmp;

    Z.Normalize(eye - at);
    Y.Normalize(up);

    tmp.Cross(Y, Z);
    X.Normalize(tmp);

    tmp.Cross(Z, X);
    Y.Normalize(tmp);

    m[0][0] = X.x;
    m[0][1] = Y.x;
    m[0][2] = Z.x;
    m[0][3] = 0.0f;

    m[1][0] = X.y;
    m[1][1] = Y.y;
    m[1][2] = Z.y;
    m[1][3] = 0.0f;

    m[2][0] = X.z;
    m[2][1] = Y.z;
    m[2][2] = Z.z;
    m[2][3] = 0.0f;

    m[3][0] = -X.Dot(eye);
    m[3][1] = -Y.Dot(eye);
    m[3][2] = -Z.Dot(eye);
    m[3][3] = 1.0f;
}


void Mat4x4::lookAtLH(const Vec4& eye, const Vec4& at, const Vec4& up)
{
    Vec4 X, Y, Z, tmp;

    Z.Normalize(at - eye);
    Y.Normalize(up);

    tmp.Cross(Y, Z);
    X.Normalize(tmp);

    tmp.Cross(Z, X);
    Y.Normalize(tmp);

    m[0][0] = X.x;
    m[0][1] = Y.x;
    m[0][2] = Z.x;
    m[0][3] = 0.0f;

    m[1][0] = X.y;
    m[1][1] = Y.y;
    m[1][2] = Z.y;
    m[1][3] = 0.0f;

    m[2][0] = X.z;
    m[2][1] = Y.z;
    m[2][2] = Z.z;
    m[2][3] = 0.0f;

    m[3][0] = -X.Dot(eye);
    m[3][1] = -Y.Dot(eye);
    m[3][2] = -Z.Dot(eye);
    m[3][3] = 1.0f;
}

void Mat4x4::LookAt(const Vec4& eye, const Vec4& at, const Vec4& up)
{
    Vec4 X, Y, Z, tmp;

    Z.Normalize(at - eye);
    Y.Normalize(up);

    tmp.Cross(Y, Z);
    X.Normalize(tmp);

    tmp.Cross(Z, X);
    Y.Normalize(tmp);

    m[0][0] = X.x;
    m[0][1] = X.y;
    m[0][2] = X.z;
    m[0][3] = 0.0f;

    m[1][0] = Y.x;
    m[1][1] = Y.y;
    m[1][2] = Y.z;
    m[1][3] = 0.0f;

    m[2][0] = Z.x;
    m[2][1] = Z.y;
    m[2][2] = Z.z;
    m[2][3] = 0.0f;

    m[3][0] = eye.x;
    m[3][1] = eye.y;
    m[3][2] = eye.z;
    m[3][3] = 1.0f;
}

void Mat4x4::PerspectiveFovLH2(const float fovy, const float aspect, const float zn, const float zf)
{
    /*
        xScale     0          0               0
    0        yScale       0               0
    0          0       zf/(zf-zn)         1
    0          0       -zn*zf/(zf-zn)     0
    where:
    */
    /*
    +    pout->m[0][0] =3D 1.0f / (aspect * tan(fovy/2.0f));
    +    pout->m[1][1] =3D 1.0f / tan(fovy/2.0f);
    +    pout->m[2][2] =3D zf / (zf - zn);
    +    pout->m[2][3] =3D 1.0f;
    +    pout->m[3][2] =3D (zf * zn) / (zn - zf);
    +    pout->m[3][3] =3D 0.0f;



    float yscale = cosf(fovy*0.5f);

    float xscale = yscale / aspect;

    */
    m[0][0] = 1.0f / (aspect * tanf(fovy * 0.5f));
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[0][3] = 0.0f;

    m[1][0] = 0.0f;
    m[1][1] = 1.0f / tanf(fovy * 0.5f);
    m[1][2] = 0.0f;
    m[1][3] = 0.0f;

    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = zf / (zf - zn);
    m[2][3] = 1.0f;

    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = (zf * zn) / (zn - zf);
    m[3][3] = 0.0f;
}

void Mat4x4::OrthoOffCenterLH(const float l, float r, float b, const float t, float zn, const float zf)
{
    m[0][0] = 2 / (r - l);
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[0][3] = 0.0f;

    m[1][0] = 0.0f;
    m[1][1] = 2 / (t - b);
    m[1][2] = 0.0f;
    m[1][3] = 0.0f;

    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = 1.0f / (zf - zn);
    m[2][3] = 0.0f;

    m[3][0] = (l + r) / (l - r);
    m[3][1] = (t + b) / (b - t);
    m[3][2] = zn / (zn - zf);
    m[3][3] = 1.0f;
}

float Mat4x4::Inverse(const Mat4x4& srcMatrix, bool affine)
{
    *this = srcMatrix;
    return Inverse(affine);
}

float Mat4x4::Inverse(bool affine)
{
    float det = 0;

    if (affine)
    {
        det = GetDeterminant();
        float s = 1 / det;
        float v00 = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * s;
        float v01 = (m[2][1] * m[0][2] - m[2][2] * m[0][1]) * s;
        float v02 = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * s;
        float v10 = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * s;
        float v11 = (m[2][2] * m[0][0] - m[2][0] * m[0][2]) * s;
        float v12 = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * s;
        float v20 = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * s;
        float v21 = (m[2][0] * m[0][1] - m[2][1] * m[0][0]) * s;
        float v22 = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * s;
        float v30 = -(v00 * m[3][0] + v10 * m[3][1] + v20 * m[3][2]);
        float v31 = -(v01 * m[3][0] + v11 * m[3][1] + v21 * m[3][2]);
        float v32 = -(v02 * m[3][0] + v12 * m[3][1] + v22 * m[3][2]);
        m[0][0] = v00;
        m[0][1] = v01;
        m[0][2] = v02;
        m[1][0] = v10;
        m[1][1] = v11;
        m[1][2] = v12;
        m[2][0] = v20;
        m[2][1] = v21;
        m[2][2] = v22;
        m[3][0] = v30;
        m[3][1] = v31;
        m[3][2] = v32;
    }
    else
    {
        // transpose matrix
        float src[16];
        for (int i = 0; i < 4; ++i)
        {
            src[i] = m16[i * 4];
            src[i + 4] = m16[i * 4 + 1];
            src[i + 8] = m16[i * 4 + 2];
            src[i + 12] = m16[i * 4 + 3];
        }

        // calculate pairs for first 8 elements (cofactors)
        float tmp[12]; // temp array for pairs
        tmp[0] = src[10] * src[15];
        tmp[1] = src[11] * src[14];
        tmp[2] = src[9] * src[15];
        tmp[3] = src[11] * src[13];
        tmp[4] = src[9] * src[14];
        tmp[5] = src[10] * src[13];
        tmp[6] = src[8] * src[15];
        tmp[7] = src[11] * src[12];
        tmp[8] = src[8] * src[14];
        tmp[9] = src[10] * src[12];
        tmp[10] = src[8] * src[13];
        tmp[11] = src[9] * src[12];

        // calculate first 8 elements (cofactors)
        m16[0] = (tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7]) - (tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7]);
        m16[1] = (tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7]) - (tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7]);
        m16[2] = (tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7]) - (tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7]);
        m16[3] = (tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6]) - (tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6]);
        m16[4] = (tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3]) - (tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3]);
        m16[5] = (tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3]) - (tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3]);
        m16[6] = (tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3]) - (tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3]);
        m16[7] = (tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2]) - (tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2]);

        // calculate pairs for second 8 elements (cofactors)
        tmp[0] = src[2] * src[7];
        tmp[1] = src[3] * src[6];
        tmp[2] = src[1] * src[7];
        tmp[3] = src[3] * src[5];
        tmp[4] = src[1] * src[6];
        tmp[5] = src[2] * src[5];
        tmp[6] = src[0] * src[7];
        tmp[7] = src[3] * src[4];
        tmp[8] = src[0] * src[6];
        tmp[9] = src[2] * src[4];
        tmp[10] = src[0] * src[5];
        tmp[11] = src[1] * src[4];

        // calculate second 8 elements (cofactors)
        m16[8] = (tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15]) - (tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15]);
        m16[9] = (tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15]) - (tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15]);
        m16[10] = (tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15]) - (tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15]);
        m16[11] = (tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14]) - (tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14]);
        m16[12] = (tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9]) - (tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10]);
        m16[13] = (tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10]) - (tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8]);
        m16[14] = (tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8]) - (tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9]);
        m16[15] = (tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9]) - (tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8]);

        // calculate determinant
        det = src[0] * m16[0] + src[1] * m16[1] + src[2] * m16[2] + src[3] * m16[3];

        // calculate matrix inverse
        float invdet = 1 / det;
        for (int j = 0; j < 16; ++j)
        {
            m16[j] *= invdet;
        }
    }

    return det;

}

uint32_t InputSampler::Value() const
{
    static const uint32_t wrapu[] = { 0, BGFX_SAMPLER_U_CLAMP, BGFX_SAMPLER_U_BORDER, 0 };
    static const uint32_t wrapv[] = { 0, BGFX_SAMPLER_V_CLAMP, BGFX_SAMPLER_V_BORDER, 0 };
    static const uint32_t filterMin[] = { 0, BGFX_SAMPLER_MIN_POINT };
    static const uint32_t filterMag[] = { 0, BGFX_SAMPLER_MIN_POINT };
    return wrapu[mWrapU] + wrapv[mWrapV] + filterMin[mFilterMin] + filterMag[mFilterMag];
}

void Bounds::AddPoint(const Vec3 pt)
{
    mMin.x = Min(mMin.x, pt.x);
    mMin.y = Min(mMin.y, pt.y);
    mMin.z = Min(mMin.z, pt.z);
    mMax.x = Max(mMax.x, pt.x);
    mMax.y = Max(mMax.y, pt.y);
    mMax.z = Max(mMax.z, pt.z);
}

void Bounds::AddBounds(const Bounds bounds, const Mat4x4& matrix)
{
    for(int i = 0; i < 8; i++)
    {
        Vec4 p((i & 1) ? bounds.mMax.x : bounds.mMin.x,
            (i & 2) ? bounds.mMax.y : bounds.mMin.y,
            (i & 4) ? bounds.mMax.z : bounds.mMin.z);
        p.TransformPoint(matrix);
        AddPoint({p.x, p.y, p.z});
    }
}

Vec4 Bounds::Center() const
{
    return Vec4(mMin.x + mMax.x, mMin.y + mMax.y, mMin.z + mMax.z) * 0.5f;
}