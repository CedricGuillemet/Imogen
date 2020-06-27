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
#include <stdint.h>
#include <vector>
#include <assert.h>
#include "Utils.h"

struct AnimationBase
{
    AnimationBase()
    {
    }

    AnimationBase(const AnimationBase&& animation)
    {
        mFrames = animation.mFrames;
    }

    AnimationBase(const AnimationBase& animation)
    {
        mFrames = animation.mFrames;
    }

    virtual void Allocate(size_t elementCount)
    {
        assert(0);
    }

    virtual void* GetData()
    {
        assert(0);
        return nullptr;
    }

    virtual const void* GetDataConst() const
    {
        assert(0);
        return nullptr;
    }

    virtual size_t GetValuesByteLength() const
    {
        assert(0);
        return 0;
    }

    virtual void GetValue(uint32_t frame, void* destination)
    {
        assert(0);
    }

    virtual void SetValue(uint32_t frame, void* source)
    {
        assert(0);
    }

    virtual float GetFloatValue(uint32_t index, int componentIndex)
    {
        assert(0);
        return 0.f;
    }

    virtual void SetFloatValue(uint32_t index, int componentIndex, float value)
    {
        assert(0);
    }

    virtual void Copy(AnimationBase* source)
    {
        mFrames = source->mFrames;
    }

    struct AnimationPointer
    {
        int mPreviousIndex;
        int mPreviousFrame;
        int mNextIndex;
        int mNextFrame;
        float mRatio;
    };

    AnimationPointer GetPointer(int32_t frame, bool bSetting) const;

    bool operator!=(const AnimationBase& other) const
    {
        if (mFrames != other.mFrames)
            return true;

        size_t la = GetValuesByteLength();
        size_t lb = other.GetValuesByteLength();
        if (la != lb)
            return true;
        if (memcmp(this->GetDataConst(), other.GetDataConst(), la))
            return true;
        return false;
    }

    std::vector<int32_t> mFrames;
};

template<typename T>
struct Animation : public AnimationBase
{
    Animation()
    {
    }

    Animation(const Animation&& animation) : AnimationBase(animation)
    {
        mValues = animation.mValues;
    }

    Animation(const Animation& animation) : AnimationBase(animation)
    {
        mValues = animation.mValues;
    }

    virtual void Allocate(size_t elementCount)
    {
        mFrames.resize(elementCount);
        mValues.resize(elementCount);
    }

    virtual void* GetData()
    {
        return mValues.data();
    }

    virtual const void* GetDataConst() const
    {
        return mValues.data();
    }

    virtual size_t GetValuesByteLength() const
    {
        return mValues.size() * sizeof(T);
    }

    virtual float GetFloatValue(uint32_t index, int componentIndex)
    {
        unsigned char* ptr = (unsigned char*)GetData();
        T& v = ((T*)ptr)[index];
        return GetComponent(componentIndex, v);
    }

    virtual void SetFloatValue(uint32_t index, int componentIndex, float value)
    {
        unsigned char* ptr = (unsigned char*)GetData();
        T& v = ((T*)ptr)[index];
        SetComponent(componentIndex, v, value);
    }

    virtual void GetValue(uint32_t frame, void* destination)
    {
        if (mValues.empty())
            return;
        AnimationPointer pointer = GetPointer(frame, false);
        T* dest = (T*)destination;
        *dest = Lerp(mValues[pointer.mPreviousIndex], mValues[pointer.mNextIndex], pointer.mRatio);
    }

    virtual void SetValue(uint32_t frame, void* source)
    {
        auto pointer = GetPointer(frame, true);
        T value = *(T*)source;
        if (frame == pointer.mPreviousFrame && !mValues.empty())
        {
            mValues[pointer.mPreviousIndex] = value;
        }
        else
        {
            if (mFrames.empty() || pointer.mPreviousIndex >= mFrames.size())
            {
                mFrames.push_back(frame);
                mValues.push_back(value);
            }
            else
            {
                mFrames.insert(mFrames.begin() + pointer.mPreviousIndex + 1, frame);
                mValues.insert(mValues.begin() + pointer.mPreviousIndex + 1, value);
            }
        }
    }

    virtual void Copy(AnimationBase* source)
    {
        AnimationBase::Copy(source);
        Allocate(source->mFrames.size());
        size_t valuesSize = GetValuesByteLength();
        if (valuesSize)
            memcpy(GetData(), source->GetData(), valuesSize);
    }

    std::vector<T> mValues;

protected:
    template<typename U>
    float GetComponent(int componentIndex, U& v)
    {
        return float(v[componentIndex]);
    }

    template<typename U>
    void SetComponent(int componentIndex, U& v, float value)
    {
        v[componentIndex] = decltype(v[componentIndex])(value);
    }

    float GetComponent(int componentIndex, unsigned char& v)
    {
        return float(v);
    }

    float GetComponent(int componentIndex, int& v)
    {
        return float(v);
    }

    float GetComponent(int componentIndex, float& v)
    {
        return v;
    }

    void SetComponent(int componentIndex, unsigned char& v, float value)
    {
        v = (unsigned char)(value);
    }

    void SetComponent(int componentIndex, int& v, float value)
    {
        v = int(value);
    }

    void SetComponent(int componentIndex, float& v, float value)
    {
        v = value;
    }
};

struct AnimTrack
{
    AnimTrack()
    {
    }

    AnimTrack(const AnimTrack& other)
    {
        *this = other;
    }

    AnimTrack& operator=(const AnimTrack& other);

    bool operator!=(const AnimTrack& other) const
    {
        if (mNodeIndex != other.mNodeIndex)
            return true;
        if (mParamIndex != other.mParamIndex)
            return true;
        if (mValueType != other.mValueType)
            return true;
        if (mAnimation->operator!=(*other.mAnimation))
            return true;
        return false;
    }

    uint32_t mNodeIndex{0xFFFFFFFF};
    uint32_t mParamIndex{0xFFFFFFFF};
    uint32_t mValueType; // Con_
    AnimationBase* mAnimation{nullptr};
};

AnimationBase* AllocateAnimation(uint32_t valueType);

typedef std::vector<AnimTrack> AnimationTracks;
