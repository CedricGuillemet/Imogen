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

struct Dirty
{
    enum
    {
        Input = 1 << 0,
        Parameter = 1 << 1,
        Mouse = 1 << 2,
        Camera = 1 << 3,
        Time = 1 << 4,
        Sampler = 1 << 5,
        AddedNode = 1 << 6,
        DeletedNode = 1 << 7
    };
};
typedef unsigned char DirtyFlag;

struct DirtyList
{
    size_t mNodeIndex;
    DirtyFlag mFlags;
};

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

struct UIInput
{
    float mRx;
    float mRy;
    float mDx;
    float mDy;
    float mWheel;
    uint8_t mLButDown : 1;
    uint8_t mRButDown : 1;
    uint8_t mbCtrl : 1;
    uint8_t mbAlt : 1;
    uint8_t mbShift : 1;
};

struct EvaluationStage
{
    //#ifdef _DEBUG needed for fur rendering
    std::string mTypename;
    //#endif
    std::shared_ptr<FFMPEGCodec::Decoder> mDecoder;
    uint16_t mType;
    unsigned int mRuntimeUniqueId;

    int mStartFrame, mEndFrame;
    
    // Camera
    Mat4x4 mParameterViewMatrix = Mat4x4::GetIdentity();

    // scene render
    void* mScene; // for path tracer
    std::shared_ptr<Scene> mGScene;
    void* renderer;
    Image DecodeImage();

    // runtime -> context
   
    int mLocalTime;
};

// simple API
struct EvaluationStages
{
    EvaluationStages();

    void AddSingleEvaluation(size_t nodeType);
    void DelSingleEvaluation(size_t nodeIndex);
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


    void ClearInputs();
    void SetEvaluationInput(size_t target, int slot, int source);
    void SetEvaluationOrder(const std::vector<size_t>& nodeOrderList);
    void SetKeyboardMouse(size_t nodeIndex, const UIInput& input);
    void SetStageLocalTime(EvaluationContext* evaluationContext, size_t target, int localTime, bool updateDecoder);
    void Clear();

    const std::vector<size_t>& GetForwardEvaluationOrder() const
    {
        return mEvaluationOrderList;
    }
    void ComputeEvaluationOrder();

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

    const Parameters& GetParameters(size_t nodeIndex) const
    {
        return mParameters[nodeIndex];
    }
    void SetParameters(size_t nodeIndex, const Parameters& parameters)
    {
        mParameters[nodeIndex] = parameters;
    }

    const std::vector<MultiplexInput>& GetMultiplexInputs() const
    {
        return mMultiplexInputs;
    }
    void SetMultiplexInputs(const std::vector<MultiplexInput>& multiplexInputs)
    {
        mMultiplexInputs = multiplexInputs;
    }

    
    bool GetMultiplexedInputs(size_t nodeIndex, size_t slotIndex, std::vector<size_t>& list) const;

    // ffmpeg encoders
    FFMPEGCodec::Decoder* FindDecoder(const std::string& filename);

    // Data
    std::vector<AnimTrack> mAnimTrack;
    std::vector<EvaluationStage> mStages;
    std::vector<Parameters> mParameters;
    std::vector<Samplers> mInputSamplers;
    std::vector<MultiplexInput> mMultiplexInputs;
    int mFrameMin, mFrameMax;

    UIInput mUIInputs;
    size_t mInputNodeIndex;

    // runtime -> context?
    std::vector<size_t> mEvaluationOrderList;
    std::vector<Input> mInputs;
    std::vector<uint8_t> mUseCountByOthers;

protected:

    struct NodeOrder
    {
        size_t mNodeIndex;
        size_t mNodePriority;
        bool operator<(const NodeOrder& other) const
        {
            return other.mNodePriority < mNodePriority; // reverse order compared to priority value: lower last
        }
    };
    std::vector<NodeOrder> ComputeEvaluationOrders();
    void RecurseSetPriority(std::vector<NodeOrder>& orders, size_t currentIndex, size_t currentPriority, size_t& undeterminedNodeCount) const;
    size_t PickBestNode(const std::vector<EvaluationStages::NodeOrder>& orders) const;

    void InitDefaultParameters(const EvaluationStage& stage, Parameters& parameters);

    void GetMultiplexedInputs(size_t nodeIndex, std::vector<size_t>& list) const;
};
