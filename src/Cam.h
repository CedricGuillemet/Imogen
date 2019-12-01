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

#pragma once

#include "Types.h"

struct Camera
{
    Vec4 mPosition;
    Vec4 mDirection;
    Vec4 mUp;
    Vec4 mLens; // fov, distanceToTarget ....

    Camera Lerp(const Camera& target, float t);
    void LookAt(const Vec4& eye, const Vec4& target, const Vec4& up);
    float& operator[](int index);
    void SetViewMatrix(float* view);
    void ComputeViewProjectionMatrix(float* viewProj, float* viewInverse) const;
    void ComputeViewMatrix(float* view, float* viewInverse) const;
};

inline Camera Lerp(Camera a, Camera b, float t)
{
    return a.Lerp(b, t);
}