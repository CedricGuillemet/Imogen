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

#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include "EvaluationStages.h"

struct EvaluationInfo
{
    float viewRot[16];
    float viewProjection[16];
    float viewInverse[16];
    float model[16];
    float modelViewProjection[16];
    float viewport[4];

    int targetIndex;
    int forcedDirty;
    int uiPass;
    int passNumber;
    float mouse[4];
    int keyModifier[4];
    int inputIndices[8];

    int mFrame;
    int mLocalFrame;
    int mVertexSpace;
    int mDirtyFlag;

    int mipmapNumber;
    int mipmapCount;
};

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


struct EvaluationContext
{
    EvaluationContext(EvaluationStages& evaluation, bool synchronousEvaluation, int defaultWidth, int defaultHeight);
    ~EvaluationContext();

    void RunAll();
    // return true if any node is in processing state
    bool RunBackward(size_t nodeIndex);
    void RunSingle(size_t nodeIndex, EvaluationInfo& evaluationInfo);
    void RunDirty();


    void SetKeyboardMouse(size_t nodeIndex, const UIInput& input);
    int GetCurrentTime() const
    {
        return mCurrentTime;
    }
    void SetCurrentTime(int currentTime)
    {
        mCurrentTime = currentTime;
    }

    unsigned int GetEvaluationTexture(size_t target);
    std::shared_ptr<RenderTarget> GetRenderTarget(size_t target)
    {
        assert(target < mEvaluations.size());
        return mEvaluations[target].mTarget;
    }

    const std::shared_ptr<RenderTarget> GetRenderTarget(size_t target) const
    {
        assert(target < mEvaluations.size());
        return mEvaluations[target].mTarget;
    }

    FFMPEGCodec::Encoder* GetEncoder(const std::string& filename, int width, int height);
    bool IsSynchronous() const
    {
        return mbSynchronousEvaluation;
    }
    void SetSynchronous(bool synchronous)
    {
        mbSynchronousEvaluation = synchronous;
    }
    void SetTargetDirty(size_t target, DirtyFlag dirtyflag, bool onlyChild = false);
    int StageIsProcessing(size_t target) const
    {
        assert (target < mEvaluations.size());
        return mEvaluations[target].mProcessing;
    }
    float StageGetProgress(size_t target) const
    {
        assert(target < mEvaluations.size());
        return mEvaluations[target].mProgress;
    }
    void StageSetProcessing(size_t target, int processing);
    void StageSetProgress(size_t target, float progress);

    void AllocRenderTargetsForEditingPreview();

    void AllocateComputeBuffer(int target, int elementCount, int elementSize);

    struct ComputeBuffer
    {
        unsigned int mBuffer{0};
        unsigned int mElementCount;
        unsigned int mElementSize;
    };

    void Clear();

    unsigned int GetMaterialUniqueId() const
    {
        return mRuntimeUniqueId;
    }
    void SetMaterialUniqueId(unsigned int uniqueId)
    {
        mRuntimeUniqueId = uniqueId;
    }

    EvaluationStages& mEvaluationStages;
    FullScreenTriangle mFSQuad;
    unsigned int mEvaluationStateGLSLBuffer;
    void DirtyAll();

    struct Evaluation
    {
        std::shared_ptr<RenderTarget> mTarget;
        
        ComputeBuffer mComputeBuffer;
        float mProgress             = 0.f;

        uint8_t mDirtyFlag          = 0;
        uint8_t mProcessing         = false;
        uint8_t mBlendingSrc        = ONE;
        uint8_t mBlendingDst        = ZERO;
        uint8_t mVertexSpace        = 0; // UV, worldspace
        union
        {
            uint8_t u = 0;
            struct
            {
                uint8_t mbDepthBuffer : 1;
                uint8_t mbClearBuffer : 1;
                uint8_t mbActive : 1;
            };
        };
    };
    std::vector<Evaluation> mEvaluations;

    UIInput mUIInputs;
    size_t mInputNodeIndex;

protected:
    void EvaluateGLSL(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo);
    void EvaluateC(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo);
    void EvaluatePython(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo);
    void EvaluateGLSLCompute(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo);
    // return true if any node is still in processing state
    bool RunNodeList(const std::vector<size_t>& nodesToEvaluate);
    void RunNode(size_t nodeIndex);

    void RecurseBackward(size_t nodeIndex, std::vector<size_t>& usedNodes);

    void BindTextures(const EvaluationStage& evaluationStage,
                      unsigned int program,
                      size_t nodeIndex,
                      std::shared_ptr<RenderTarget> reusableTarget);
    void AllocRenderTargetsForBaking(const std::vector<size_t>& nodesToEvaluate);


    int GetBindedComputeBuffer(size_t nodeIndex) const;

    std::map<std::string, FFMPEGCodec::Encoder*> mWriteStreams;

    EvaluationInfo mEvaluationInfo;

    std::vector<int> mStillDirty;
    int mDefaultWidth;
    int mDefaultHeight;
    bool mbSynchronousEvaluation;
    unsigned int mRuntimeUniqueId; // material unique Id for thumbnail update
    int mCurrentTime;

    unsigned int mParametersGLSLBuffer;

    void SetKeyboardMouseInfos(EvaluationInfo& evaluationInfo) const;
};

struct Builder
{
    Builder();
    ~Builder();

    void Add(const char* graphName, EvaluationStages& stages);
    void Add(Material* material);
    struct BuildInfo
    {
        std::string mName;
        float mProgress;
    };

    // return true if buildInfo has been updated
    bool UpdateBuildInfo(std::vector<BuildInfo>& buildInfo);

private:
    std::mutex mMutex;
    std::thread mThread;

    std::atomic_bool mbRunning;

    struct Entry
    {
        std::string mName;
        float mProgress;
        EvaluationStages mEvaluationStages;
    };
    std::vector<Entry> mEntries;
    void BuildEntries();
    void DoBuild(Entry& entry);
};

namespace DrawUICallbacks
{
    void DrawUICubemap(EvaluationContext* context, size_t nodeIndex);
    void DrawUISingle(EvaluationContext* context, size_t nodeIndex);
    void DrawUIProgress(EvaluationContext* context, size_t nodeIndex);
} // namespace DrawUICallbacks