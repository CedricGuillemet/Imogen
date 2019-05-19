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
#include <string>
#include <vector>
#include <map>
#include "Library.h"
#include "libtcc/libtcc.h"
#include "Imogen.h"
#include <string.h>
#include <stdio.h>
#include "ffmpegCodec.h"
#include <memory>
#include "Utils.h"
#include "Bitmap.h"

struct ImDrawList;
struct ImDrawCmd;
struct EvaluationContext;
struct EvaluationInfo;

enum BlendOp
{
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    CONSTANT_COLOR,
    ONE_MINUS_CONSTANT_COLOR,
    CONSTANT_ALPHA,
    ONE_MINUS_CONSTANT_ALPHA,
    SRC_ALPHA_SATURATE,
    BLEND_LAST
};


struct Input
{
    Input()
    {
        memset(mInputs, -1, sizeof(int) * 8);
        memset(mOverrideInputs, -1, sizeof(int) * 8);
    }
    int mInputs[8];
    int mOverrideInputs[8];
};


struct Scene
{
    Scene()
    {
    }
    virtual ~Scene();
    struct Mesh
    {
        struct Format
        {
            enum
            {
                POS = 1 << 0,
                NORM = 1 << 1,
                COL = 1 << 2,
                UV = 1 << 3,
            };
        };
        struct Buffer
        {
            unsigned int id;
            unsigned int format;
            unsigned int stride;
            unsigned int count;
        };
        struct IndexBuffer
        {
            unsigned int id;
            unsigned int stride;
            unsigned int count;
        };
        struct Primitive
        {
            std::vector<Buffer> mBuffers;
            IndexBuffer mIndexBuffer = {0, 0, 0};
            void AddBuffer(const void* data, unsigned int format, unsigned int stride, unsigned int count);
            void AddIndexBuffer(const void* data, unsigned int stride, unsigned int count);
            void Draw() const;
        };
        std::vector<Primitive> mPrimitives;
        void Draw() const;
    };
    std::vector<Mesh> mMeshes;
    std::vector<Mat4x4> mWorldTransforms;
    std::vector<int> mMeshIndex;
    std::string mName;
    void Draw(EvaluationContext* context, EvaluationInfo& evaluationInfo) const;
};

typedef std::vector<unsigned char> Parameters;
typedef std::vector<InputSampler> Samplers;

struct EvaluationStage
{
    //#ifdef _DEBUG needed for fur rendering
    std::string mTypename;
    //#endif
    std::shared_ptr<FFMPEGCodec::Decoder> mDecoder;
    size_t mType;
    unsigned int mRuntimeUniqueId;
    Input mInput;
    
    int gEvaluationMask; // see EvaluationMask
    int mUseCountByOthers;
    int mBlendingSrc;
    int mBlendingDst;
    int mLocalTime;
    int mStartFrame, mEndFrame;
    int mVertexSpace; // UV, worldspace
    bool mbDepthBuffer;
    bool mbClearBuffer;
    // Camera
    Mat4x4 mParameterViewMatrix = Mat4x4::GetIdentity();
    // mouse
    float mRx;
    float mRy;
    uint8_t mLButDown : 1;
    uint8_t mRButDown : 1;
    uint8_t mbCtrl : 1;
    uint8_t mbAlt : 1;
    uint8_t mbShift : 1;

    // scene render
    void* mScene; // for path tracer
    std::shared_ptr<Scene> mGScene;
    void* renderer;
    Image DecodeImage();
#if 0
    bool operator!=(const EvaluationStage& other) const
    {
        if (mType != other.mType)
            return true;
        if (mParameters != other.mParameters)
            return true;
        if (mRuntimeUniqueId != other.mRuntimeUniqueId)
            return true;
        if (mStartFrame != other.mStartFrame)
            return true;
        if (mEndFrame != other.mEndFrame)
            return true;
        if (mInputSamplers.size() != other.mInputSamplers.size())
            return true;
        /*
        for (size_t i = 0; i < mInputSamplers.size(); i++)
        {
            if (mInputSamplers[i] != other.mInputSamplers[i])
                return true;
        }
        */
        if (mParameterViewMatrix != other.mParameterViewMatrix)
            return true;
        return false;
    }
#endif
};

// simple API
struct EvaluationStages
{
    EvaluationStages();

    void AddSingleEvaluation(size_t nodeType);

    size_t GetStagesCount() const
    {
        return mStages.size();
    }
    size_t GetStageType(size_t target) const
    {
        return mStages[target].mType;
    }
    size_t GetEvaluationImageDuration(size_t target);

    void SetEvaluationParameters(size_t target, const Parameters& parameters);
    void SetSamplers(size_t nodeIndex, const std::vector<InputSampler>& inputSamplers);
    void AddEvaluationInput(size_t target, int slot, int source);
    void DelEvaluationInput(size_t target, int slot);
    void SetEvaluationOrder(const std::vector<size_t>& nodeOrderList);
    void SetKeyboardMouse(
        int target, float rx, float ry, bool lButDown, bool rButDown, bool bCtrl, bool bAlt, bool bShift);
    void SetStageLocalTime(EvaluationContext* evaluationContext, size_t target, int localTime, bool updateDecoder);
    void Clear();

    const std::vector<size_t>& GetForwardEvaluationOrder() const
    {
        return mEvaluationOrderList;
    }

    const EvaluationStage& GetEvaluationStage(size_t index) const
    {
        return mStages[index];
    }

    Camera* GetCameraParameter(size_t index);
    int GetIntParameter(size_t index, const char* parameterName, int defaultValue);
    Mat4x4* GetParameterViewMatrix(size_t index)
    {
        if (index >= mStages.size())
            return NULL;
        return &mStages[index].mParameterViewMatrix;
    }
    float GetParameterComponentValue(size_t index, int parameterIndex, int componentIndex);

    // animation
    const std::vector<AnimTrack>& GetAnimTrack() const
    {
        return mAnimTrack;
    }
    void ApplyAnimationForNode(EvaluationContext* context, size_t nodeIndex, int frame);
    void ApplyAnimation(EvaluationContext* context, int frame);
    void RemoveAnimation(size_t nodeIndex);
    void SetAnimTrack(const std::vector<AnimTrack>& animTrack);
    void SetTime(EvaluationContext* evaluationContext, int time, bool updateDecoder);

    // pins
    bool IsIOPinned(size_t nodeIndex, size_t io, bool forOutput) const;
    void SetIOPin(size_t nodeIndex, size_t io, bool forOutput, bool pinned);
    bool IsParameterPinned(size_t nodeIndex, size_t parameterIndex) const;
    void SetParameterPin(size_t nodeIndex, size_t parameterIndex, bool pinned);
    void SetParameterPins(const std::vector<uint32_t>& pins)
    {
        mPinnedParameters = pins;
    }
    void SetIOPins(const std::vector<uint32_t>& pins)
    {
        mPinnedIO = pins;
    }
    const std::vector<uint32_t>& GetParameterPins() const
    {
        return mPinnedParameters;
    }
    const std::vector<uint32_t>& GetIOPins() const
    {
        return mPinnedIO;
    }
    const Parameters& GetParameters(size_t nodeIndex) const
    {
        return mParameters[nodeIndex];
    }
    void SetParameters(size_t nodeIndex, const Parameters& parameters)
    {
        mParameters[nodeIndex] = parameters;
    }

    // ffmpeg encoders
    FFMPEGCodec::Decoder* FindDecoder(const std::string& filename);

    // Data
    std::vector<AnimTrack> mAnimTrack;
    std::vector<EvaluationStage> mStages;
    std::vector<size_t> mEvaluationOrderList;
    std::vector<uint32_t> mPinnedParameters; // 32bits -> 32parameters
    std::vector<uint32_t> mPinnedIO;         // 24bits input, 8 bits output
    std::vector<Parameters> mParameters;
    std::vector<Samplers> mInputSamplers;
    int mFrameMin, mFrameMax;

protected:


    void StageIsAdded(int index);
    void StageIsDeleted(int index);
    void InitDefaultParameters(const EvaluationStage& stage, Parameters& parameters);
    void RemovePins(size_t nodeIndex);
};
