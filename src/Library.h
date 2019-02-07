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

#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <string>
#include <map>
#include <memory>
#include "Utils.h"
#include <assert.h>

struct Camera
{
    Vec4 mPosition;
    Vec4 mDirection;
    Vec4 mUp;
    Vec4 mLens; // fov,....

    Camera Lerp(const Camera& target, float t)
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
    float& operator[] (int index)
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

    void ComputeViewProjectionMatrix(float *viewProj, float *viewInverse);
};
inline Camera Lerp(Camera a, Camera b, float t) { return a.Lerp(b, t); }

// used to retrieve structure in library. left is index. right is uniqueId
// if item at index doesn't correspond to uniqueid, then a search is done
// based on the unique id
typedef std::pair<size_t, unsigned int> ASyncId;
template<typename T> T* GetByAsyncId(ASyncId id, std::vector<T>& items)
{
    if (items.size() > id.first && items[id.first].mRuntimeUniqueId == id.second)
    {
        return &items[id.first];
    }
    else
    {
        // find identifier
        for (auto& item : items)
        {
            if (item.mRuntimeUniqueId == id.second)
            {
                return &item;
            }
        }
    }
    return NULL;
}

struct InputSampler
{
    InputSampler() : mWrapU(0), mWrapV(0), mFilterMin(0), mFilterMag(0) 
    {
    }
    uint32_t mWrapU;
    uint32_t mWrapV;
    uint32_t mFilterMin;
    uint32_t mFilterMag;
    
    bool operator != (const InputSampler& other) const
    {
        return (mWrapU != other.mWrapU || mWrapV != other.mWrapV || mFilterMin != other.mFilterMin || mFilterMag != other.mFilterMag);
    }
    bool operator == (const InputSampler& other) const
    {
        return (mWrapU == other.mWrapU && mWrapV == other.mWrapV && mFilterMin == other.mFilterMin && mFilterMag == other.mFilterMag);
    }
};

struct MaterialNode
{
    uint32_t mType;
    std::string mTypeName;
    int32_t mPosX;
    int32_t mPosY;
    std::vector<InputSampler> mInputSamplers;
    std::vector<uint8_t> mParameters;
    std::vector<uint8_t> mImage;

    uint32_t mFrameStart;
    uint32_t mFrameEnd;

    // runtime
    unsigned int mRuntimeUniqueId;
};

struct MaterialNodeRug
{
    int32_t mPosX;
    int32_t mPosY;
    int32_t mSizeX;
    int32_t mSizeY;
    uint32_t mColor;
    std::string mComment;
};

struct MaterialConnection
{
    uint32_t mInputNode;
    uint32_t mOutputNode;
    uint8_t mInputSlot;
    uint8_t mOutputSlot;
};

struct AnimationBase 
{
    AnimationBase() {}
    AnimationBase(const AnimationBase&& animation)
    {
        mFrames = animation.mFrames;
    }
    AnimationBase(const AnimationBase& animation)
    {
        mFrames = animation.mFrames;
    }
    std::vector<int32_t> mFrames;

    virtual void Allocate(size_t elementCount) { assert(0); }
    virtual void* GetData() { assert(0); return nullptr; }
    virtual const void* GetDataConst() const { assert(0); return nullptr; }
    virtual size_t GetValuesByteLength() const { assert(0); return 0; }
    virtual void GetValue(uint32_t frame, void *destination) { assert(0); }
    virtual void SetValue(uint32_t frame, void *source) { assert(0); }
    virtual float GetFloatValue(uint32_t index, int componentIndex) { assert(0); return 0.f; }
    virtual void SetFloatValue(uint32_t index, int componentIndex, float value) { assert(0); }
    virtual void Copy(AnimationBase *source)
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
    bool operator != (const AnimationBase& other) const
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
};

template<typename T> struct Animation : public AnimationBase
{
    Animation() {}
    Animation(const Animation&& animation) : AnimationBase(animation)
    {
        mValues = animation.mValues;
    }
    Animation(const Animation& animation) : AnimationBase(animation)
    {
        mValues = animation.mValues;
    }
    std::vector<T> mValues;

    virtual void Allocate(size_t elementCount) 
    { 
        mFrames.resize(elementCount); 
        mValues.resize(elementCount);
    }
    virtual void* GetData() { return mValues.data(); }
    virtual const void* GetDataConst() const { return mValues.data(); }

    virtual size_t GetValuesByteLength() const { return mValues.size() * sizeof(T); }
    virtual float GetFloatValue(uint32_t index, int componentIndex)
    {
        unsigned char*ptr = (unsigned char*)GetData();
        T& v = ((T*)ptr)[index];
        return GetComponent(componentIndex, v);
    }

    virtual void SetFloatValue(uint32_t index, int componentIndex, float value)
    {
        unsigned char*ptr = (unsigned char*)GetData();
        T& v = ((T*)ptr)[index];
        SetComponent(componentIndex, v, value);
    }

    virtual void GetValue(uint32_t frame, void *destination) 
    {
        if (mValues.empty())
            return;
        AnimationPointer pointer = GetPointer(frame, false);
        T *dest = (T*)destination;
        *dest = Lerp(mValues[pointer.mPreviousIndex], mValues[pointer.mNextIndex], pointer.mRatio);
    }

    virtual void SetValue(uint32_t frame, void *source) 
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

    virtual void Copy(AnimationBase *source)
    {
        AnimationBase::Copy(source);
        Allocate(source->mFrames.size());
        size_t valuesSize = GetValuesByteLength();
        if (valuesSize)
            memcpy(GetData(), source->GetData(), valuesSize);
    }

protected:
    template<typename T> float GetComponent(int componentIndex, T& v)
    {
        return float(v[componentIndex]);
    }
    template<typename T> void SetComponent(int componentIndex, T& v, float value)
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
        v = unsigned char(value);
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
    AnimTrack() {}
    AnimTrack(const AnimTrack& other)
    {
        *this = other;
    }
    uint32_t mNodeIndex;
    uint32_t mParamIndex;
    uint32_t mValueType; // Con_
    AnimationBase* mAnimation = nullptr;
    AnimTrack& operator = (const AnimTrack& other);
    bool operator != (const AnimTrack& other) const
    {
        if (mNodeIndex != other.mNodeIndex)
            return true;
        if (mParamIndex != other.mParamIndex)
            return true;
        if (mValueType != other.mValueType)
            return true;
        if (mAnimation->operator != (*other.mAnimation))
            return true;
        return false;
    }
};

struct Material
{
    std::string mName;
    std::string mComment;
    std::vector<MaterialNode> mMaterialNodes;
    std::vector<MaterialNodeRug> mMaterialRugs;
    std::vector<MaterialConnection> mMaterialConnections;
    std::vector<uint8_t> mThumbnail;

    std::vector<AnimTrack> mAnimTrack;

    int mFrameMin, mFrameMax;

    std::vector<uint32_t> mPinnedParameters;
    MaterialNode* Get(ASyncId id) { return GetByAsyncId(id, mMaterialNodes); }

    //run time
    unsigned int mThumbnailTextureId;
    unsigned int mRuntimeUniqueId;
};

struct Library
{
    std::vector<Material> mMaterials;
    Material* Get(ASyncId id) { return GetByAsyncId(id, mMaterials); }
    Material* GetByName(const char* materialName)
    {
        for (auto &material : mMaterials)
        {
            if (material.mName == materialName)
            {
                return &material;
            }
        }
        return nullptr;
    }
};

void LoadLib(Library *library, const char *szFilename);
void SaveLib(Library *library, const char *szFilename);

enum ConTypes
{
    Con_Float,
    Con_Float2,
    Con_Float3,
    Con_Float4,
    Con_Color4,
    Con_Int,
    Con_Int2,
    Con_Ramp,
    Con_Angle,
    Con_Angle2,
    Con_Angle3,
    Con_Angle4,
    Con_Enum,
    Con_Structure,
    Con_FilenameRead,
    Con_FilenameWrite,
    Con_ForceEvaluate,
    Con_Bool,
    Con_Ramp4,
    Con_Camera,
    Con_Any,
};

enum CurveType
{
    CurveNone,
    CurveDiscrete,
    CurveLinear,
    CurveSmooth,
    CurveBezier,
};

size_t GetParameterTypeSize(ConTypes paramType);
size_t GetParameterOffset(uint32_t type, uint32_t parameterIndex);
size_t GetCurveCountPerParameterType(uint32_t paramType);
const char* GetCurveParameterSuffix(uint32_t paramType, int suffixIndex);
uint32_t GetCurveParameterColor(uint32_t paramType, int suffixIndex);
AnimationBase *AllocateAnimation(uint32_t valueType);
CurveType GetCurveTypeForParameterType(ConTypes paramType);
void DecodeThumbnailAsync(Material * material);
struct MetaCon
{
    std::string mName;
    int mType;
    bool operator == (const MetaCon& other) const
    {
        if (mName != other.mName)
            return false;
        if (mType != other.mType)
            return false;
        return true;
    }
};

struct MetaParameter
{
    std::string mName;
    ConTypes mType;
    float mRangeMinX, mRangeMaxX;
    float mRangeMinY, mRangeMaxY;
    bool mbRelative;
    bool mbQuadSelect;
    std::string mEnumList;

    bool operator == (const MetaParameter& other) const
    {
        if (mName != other.mName)
            return false;
        if (mType != other.mType)
            return false;
        if (mRangeMaxX != other.mRangeMaxX)
            return false;
        if (mRangeMinX != other.mRangeMinX)
            return false;
        if (mRangeMaxY != other.mRangeMaxY)
            return false;
        if (mRangeMinY != other.mRangeMinY)
            return false;
        if (mbRelative != other.mbRelative)
            return false;
        if (mbQuadSelect != other.mbQuadSelect)
            return false;
        if (mEnumList != other.mEnumList)
            return false;
        return true;
    }
};

struct MetaNode
{
    std::string mName;
    uint32_t mHeaderColor;
    int mCategory;
    std::vector<MetaCon> mInputs;
    std::vector<MetaCon> mOutputs;
    std::vector<MetaParameter> mParams;

    bool mbHasUI;
    bool mbSaveTexture;

    bool operator == (const MetaNode& other) const
    {
        if (mName != other.mName)
            return false;
        if (mCategory != other.mCategory)
            return false;
        if (mHeaderColor != other.mHeaderColor)
            return false;
        if (mInputs != other.mInputs)
            return false;
        if (mOutputs != other.mOutputs)
            return false;
        if (mParams != other.mParams)
            return false;
        if (mbHasUI != other.mbHasUI)
            return false;
        if (mbSaveTexture != other.mbSaveTexture)
            return false;
        return true;
    }
};

extern std::vector<MetaNode> gMetaNodes;
size_t GetMetaNodeIndex(const std::string& metaNodeName);
void LoadMetaNodes();

std::vector<MetaNode> ReadMetaNodes(const char *filename);
unsigned int GetRuntimeId();
extern Library library;

