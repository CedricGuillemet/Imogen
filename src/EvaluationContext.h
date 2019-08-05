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
#include "Evaluators.h"

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

struct EvaluationThumbnails
{
    struct Thumb
    {
        unsigned short mAtlasIndex = 0xFFFF;
        unsigned short mThumbIndex = 0xFFFF;
        bool Valid() const
        {
            return mAtlasIndex != 0xFFFF && mThumbIndex != 0xFFFF;
        }
    };

    void Clear();
    Thumb AddThumb();
    void DelThumb(const Thumb thumb);
    void GetThumb(const Thumb thumb, TextureHandle& textureId, ImRect& uvs) const;
    RenderTarget& GetThumbTarget(const Thumb thumb);
    void GetThumbCoordinates(const Thumb thumb, int* coordinates) const;
    std::vector<RenderTarget> GetAtlasTextures() const;
protected:

    const size_t AtlasSize = 4096;
    const size_t ThumbnailSize = 256;
    const size_t ThumbnailsPerAtlas = (AtlasSize / ThumbnailSize) * (AtlasSize / ThumbnailSize);

    ImRect ComputeUVFromIndexInAtlas(size_t thumbIndex) const;
    Thumb AddThumbInAtlas(size_t atlasIndex);

    struct ThumbAtlas
    {
        ThumbAtlas(size_t thumbnailsPerAtlas)
        {
            mbUsed.resize(thumbnailsPerAtlas, false);
        }
        RenderTarget mTarget;
        std::vector<bool> mbUsed;
        size_t mUsedCount = 0;
    };

    std::vector<ThumbAtlas> mAtlases;
    std::vector<Thumb> mThumbs;
};

struct EvaluationContext
{
    EvaluationContext(EvaluationStages& evaluation, bool synchronousEvaluation, int defaultWidth, int defaultHeight);
    ~EvaluationContext();

    // iterative editing
    void AddEvaluation(size_t nodeIndex);
    void DelEvaluation(size_t nodeIndex);

    void RunAll();
    // return true if any node is in processing state
    bool RunBackward(size_t nodeIndex);
    void RunSingle(size_t nodeIndex, EvaluationInfo& evaluationInfo);
    void RunDirty();


    void SetKeyboardMouse(size_t nodeIndex, const UIInput& input);
    int GetCurrentTime() const { return mCurrentTime; }
    void SetCurrentTime(int currentTime) { mCurrentTime = currentTime; }

    void GetThumb(size_t nodeIndex, TextureHandle& textureHandle, ImRect& uvs) const { mThumbnails.GetThumb(mEvaluations[nodeIndex].mThumb, textureHandle, uvs); }
	TextureHandle GetEvaluationTexture(size_t nodeIndex) const;

    std::shared_ptr<RenderTarget> GetRenderTarget(size_t nodeIndex) 
    { 
        assert(nodeIndex < mEvaluations.size());
        return mEvaluations[nodeIndex].mTarget; 
    }

    const std::shared_ptr<RenderTarget> GetRenderTarget(size_t nodeIndex) const
    {
        assert(nodeIndex < mEvaluations.size());
        return mEvaluations[nodeIndex].mTarget;
    }
#if USE_FFMPEG
    FFMPEGCodec::Encoder* GetEncoder(const std::string& filename, int width, int height);
#endif
    bool IsSynchronous() const
    {
        return mbSynchronousEvaluation;
    }
    void SetSynchronous(bool synchronous)
    {
        mbSynchronousEvaluation = synchronous;
    }
    void SetTargetDirty(size_t target, Dirty::Type dirtyflag, bool onlyChild = false);
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
    std::shared_ptr<RenderTarget> CreateRenderTarget(size_t nodeIndex)
    {
        assert(nodeIndex < mEvaluations.size());
        mEvaluations[nodeIndex].mTarget = std::make_shared<RenderTarget>();
        return mEvaluations[nodeIndex].mTarget;
    }
    const EvaluationThumbnails& GetThumbnails() const { return mThumbnails; }
    void AllocateComputeBuffer(int target, int elementCount, int elementSize);

    struct ComputeBuffer
    {
        unsigned int mBuffer{0};
        unsigned int mElementCount;
        unsigned int mElementSize;
    };

    void Clear();

    unsigned int GetMaterialUniqueId() const { return mRuntimeUniqueId; }
    void SetMaterialUniqueId(unsigned int uniqueId) { mRuntimeUniqueId = uniqueId; }

    EvaluationStages& mEvaluationStages;
    unsigned int mEvaluationStateGLSLBuffer;
    void DirtyAll();

    struct Evaluation
    {
        std::shared_ptr<RenderTarget> mTarget;
        
        ComputeBuffer mComputeBuffer;
        EvaluationThumbnails::Thumb mThumb;
        float mProgress             = 0.f;

        uint64_t mBlendingSrc        = BGFX_STATE_BLEND_ONE;
        uint64_t mBlendingDst        = BGFX_STATE_BLEND_ZERO;
		uint8_t mDirtyFlag = 0;
		uint8_t mProcessing = false;
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

    EvaluationThumbnails mThumbnails;

    void EvaluateGLSL(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo);
    void EvaluateC(const EvaluationStage& evaluationStage, size_t nodeIndex, EvaluationInfo& evaluationInfo);
#ifdef USE_PYTHON
    void EvaluatePython(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo);
#endif
    void EvaluateGLSLCompute(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo);
    // return true if any node is still in processing state
    bool RunNodeList(const std::vector<size_t>& nodesToEvaluate);
    void RunNode(size_t nodeIndex);
    void GenerateThumbnail(size_t nodeIndex);

    void RecurseBackward(size_t nodeIndex, std::vector<size_t>& usedNodes);

    void BindTextures(const EvaluationStage& evaluationStage,
                      size_t nodeIndex,
                      std::shared_ptr<RenderTarget> reusableTarget);
    void AllocRenderTargetsForBaking(const std::vector<size_t>& nodesToEvaluate);


    int GetBindedComputeBuffer(size_t nodeIndex) const;

    std::vector<ComputeBuffer> mComputeBuffers;
#if USE_FFMPEG    
    std::map<std::string, FFMPEGCodec::Encoder*> mWriteStreams;
#endif

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

    void Add(const char* graphName, const EvaluationStages& stages);
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