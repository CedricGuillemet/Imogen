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

#include "MetaNodes.h"
#include "Animation.h"
#include "Types.h"
#include "Cam.h"

AnimationBase::AnimationPointer AnimationBase::GetPointer(int32_t frame, bool bSetting) const
{
    if (mFrames.empty())
        return { bSetting ? -1 : 0, 0, 0, 0, 0.f };
    if (frame <= mFrames[0])
        return { bSetting ? -1 : 0, 0, 0, 0, 0.f };
    if (frame > mFrames.back())
    {
        int32_t last = int32_t(mFrames.size() - (bSetting ? 0 : 1));
        return { last, mFrames.back(), last, mFrames.back(), 0.f };
    }
    for (int i = 0; i < int(mFrames.size()) - 1; i++)
    {
        if (mFrames[i] <= frame && mFrames[i + 1] >= frame)
        {
            float ratio = float(frame - mFrames[i]) / float(mFrames[i + 1] - mFrames[i]);
            return { i, mFrames[i], i + 1, mFrames[i + 1], ratio };
        }
    }
    assert(0);
    return { bSetting ? -1 : 0, 0, 0, 0, 0.f };
}

AnimTrack& AnimTrack::operator=(const AnimTrack& other)
{
    mNodeIndex = other.mNodeIndex;
    mParamIndex = other.mParamIndex;
    mValueType = other.mValueType;
    delete mAnimation;
    mAnimation = AllocateAnimation(mValueType);
    mAnimation->Copy(other.mAnimation);
    return *this;
}


AnimationBase* AllocateAnimation(uint32_t valueType)
{
    switch (valueType)
    {
    case Con_Float:
        return new Animation<float>;
    case Con_Float2:
        return new Animation<Vec2>;
    case Con_Float3:
        return new Animation<Vec3>;
    case Con_Float4:
        return new Animation<Vec4>;
    case Con_Color4:
        return new Animation<Vec4>;
    case Con_Int:
        return new Animation<int>;
    case Con_Int2:
        return new Animation<iVec2>;
    case Con_Ramp:
        return new Animation<Vec2>;
    case Con_Angle:
        return new Animation<float>;
    case Con_Angle2:
        return new Animation<Vec2>;
    case Con_Angle3:
        return new Animation<Vec3>;
    case Con_Angle4:
        return new Animation<Vec4>;
    case Con_Enum:
        return new Animation<int>;
    case Con_Structure:
    case Con_FilenameRead:
    case Con_FilenameWrite:
    case Con_ForceEvaluate:
        return NULL;
    case Con_Bool:
        return new Animation<unsigned char>;
    case Con_Ramp4:
        return new Animation<Vec4>;
    case Con_Camera:
        return new Animation<Camera>;
    case Con_Multiplexer:
        return new Animation<int>;
    }
    return NULL;
}