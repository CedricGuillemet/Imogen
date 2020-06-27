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

#include "Cam.h"
#include "Utils.h"

Camera Camera::Lerp(const Camera& target, float t)
{
    Camera ret;
    ret.mPosition = ::Lerp(mPosition, target.mPosition, t);
    ret.mDirection = ::Lerp(mDirection, target.mDirection, t);
    ret.mDirection.Normalize();
    ret.mUp = ::Lerp(mUp, target.mUp, t);
    ret.mUp.Normalize();
    ret.mLens = ::Lerp(mLens, target.mLens, t);
    return ret;
}

void Camera::LookAt(const Vec4& eye, const Vec4& target, const Vec4& up)
{
    mPosition = eye;
    mDirection = (target - eye).Normalize();
    mUp = up;
    mLens.y = (target - eye).Length();
}

float& Camera::operator[](int index)
{
    switch (index)
    {
    case 0:
    case 1:
    case 2:
        return mPosition[index];
    case 3:
    case 4:
    case 5:
        return mDirection[index - 3];
    case 6:
        return mDirection[index - 6];
    }
    return mPosition[0];
}

void Camera::ComputeViewProjectionMatrix(float* viewProj, float* viewInverse) const
{
    Mat4x4 view, proj;
    Mat4x4& vi = *(Mat4x4*)viewInverse;
    Mat4x4& vp = *(Mat4x4*)viewProj;
    ComputeViewMatrix(view.m16, vi.m16);

    proj.glhPerspectivef2(53.f, 1.f, 0.01f, 100.f);
    vp = view * proj;
}

void Camera::ComputeViewMatrix(float* view, float* viewInverse) const
{
    Mat4x4& v = *(Mat4x4*)view;
    v.lookAtRH(mPosition, mPosition + mDirection, Vec4(0.f, 1.f, 0.f, 0.f));
    Mat4x4& vi = *(Mat4x4*)viewInverse;
    vi.LookAt(mPosition, mPosition + mDirection, Vec4(0.f, 1.f, 0.f, 0.f));
}

void Camera::SetViewMatrix(float* view)
{
    Mat4x4 matrix = *(Mat4x4*)view;
    matrix.Inverse();
    mPosition.TransformPoint(Vec4(0.f, 0.f, 0.f), matrix);
    mDirection.TransformVector(Vec4(0.f, 0.f, -1.f), matrix);
    mUp.TransformVector(Vec4(0.f, 1.f, 0.f), matrix);
}