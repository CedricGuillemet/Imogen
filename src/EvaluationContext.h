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
	void GetThumb(const Thumb thumb, bgfx::TextureHandle& textureId, ImRect& uvs) const;
	bgfx::FrameBufferHandle& GetThumbFrameBuffer(const Thumb thumb);
	void GetThumbCoordinates(const Thumb thumb, int* coordinates) const;
	std::vector<bgfx::TextureHandle> GetAtlasTextures() const;
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
		bgfx::FrameBufferHandle mFrameBuffer;
		std::vector<bool> mbUsed;
		size_t mUsedCount = 0;
	};

	std::vector<ThumbAtlas> mAtlases;
	std::vector<Thumb> mThumbs;
};


struct EvaluationContext
{
    EvaluationContext(EvaluationStages& evaluation, bool building, bool synchronousEvaluation, int defaultWidth, int defaultHeight, bool useThumbnail);
    ~EvaluationContext();

    // iterative editing
    void AddEvaluation(size_t nodeIndex);
    void DelEvaluation(size_t nodeIndex);

    void Evaluate();

    void SetKeyboardMouse(size_t nodeIndex, const UIInput& input);
    int GetCurrentTime() const { return mCurrentTime; }
    void SetCurrentTime(int currentTime) { mCurrentTime = currentTime; }

    void GetThumb(size_t nodeIndex, bgfx::TextureHandle& textureHandle, ImRect& uvs) const
    { 
        assert(mUseThumbnail);
        mThumbnails.GetThumb(mEvaluations[nodeIndex].mThumb, textureHandle, uvs); 
    }

	bgfx::TextureHandle GetEvaluationTexture(size_t nodeIndex) const;

    ImageTexture* GetRenderTarget(size_t nodeIndex) 
    { 
        assert(nodeIndex < mEvaluations.size());
        return mEvaluations[nodeIndex].mTarget; 
    }
    
    const ImageTexture* GetRenderTarget(size_t nodeIndex) const
    {
        assert(nodeIndex < mEvaluations.size());
        return mEvaluations[nodeIndex].mTarget;
    }
    
	ImageTexture* CreateRenderTarget(size_t nodeIndex)
	{
		assert(nodeIndex < mEvaluations.size());
		assert(!mEvaluations[nodeIndex].mTarget);
		mEvaluations[nodeIndex].mTarget = new ImageTexture;
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
	bool IsBuilding() const { return mBuilding; }
    void SetTargetDirty(size_t target, uint32_t dirtyflag, bool onlyChild = false);
	void SetTargetPersistent(size_t nodeIndex, bool persistent);
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

	int GetStageIndexFromRuntimeId(unsigned int runtimeUniqueId) const;
	unsigned int GetStageRuntimeId(size_t stageIndex) const;

    const EvaluationThumbnails& GetThumbnails() const 
    { 
        assert(mUseThumbnail);
        return mThumbnails; 
    }

    void Clear();

    unsigned int GetMaterialUniqueId() const { return mRuntimeUniqueId; }
    void SetMaterialUniqueId(unsigned int uniqueId) { mRuntimeUniqueId = uniqueId; }

    EvaluationStages& mEvaluationStages;
    

    struct Evaluation
    {
		Evaluation()
		{
			mRuntimeUniqueId = GetRuntimeId();
		}

        ImageTexture* mTarget = nullptr;
        
        EvaluationThumbnails::Thumb mThumb;
        float mProgress             = 0.f;

        uint64_t mBlendingSrc        = BGFX_STATE_BLEND_ONE;
        uint64_t mBlendingDst        = BGFX_STATE_BLEND_ZERO;
        uint32_t mDirtyFlag = 0;
        uint8_t mProcessing = false;
        uint8_t mVertexSpace        = 0; // UV, worldspace
        int mUseCount;
		RuntimeId mRuntimeUniqueId = InvalidRuntimeId;
        union
        {
            uint8_t u = 0;
            struct
            {
                uint8_t mbDepthBuffer : 1;
                uint8_t mbClearBuffer : 1;
                uint8_t mbActive : 1;
				uint8_t mbPersistent : 1; // target isn't deleted when not used anymore by subsequent nodes
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
    // return true if any node is still in processing state
    //bool RunNodeList(const std::vector<size_t>& nodesToEvaluate);

    void RunNode(bgfx::ViewId& viewId, size_t nodeIndex);


    void GenerateThumbnail(bgfx::ViewId& viewId, size_t nodeIndex);

    //void RecurseBackward(size_t nodeIndex, std::vector<size_t>& usedNodes);

    void BindTextures(EvaluationInfo& evaluationInfo, 
					  const EvaluationStage& evaluationStage,
                      size_t nodeIndex,
                      ImageTexture* reusableTarget);
    //void AllocRenderTargetsForBaking(const std::vector<size_t>& nodesToEvaluate);

    void ComputeTargetUseCount();
    void ReleaseInputs(size_t nodeIndex);

    ImageTexture* AcquireRenderTarget(int width, int height, bool depthBuffer);
    ImageTexture* AcquireClone(ImageTexture* source);
    void ReleaseRenderTarget(ImageTexture* renderTarget);


    std::vector<ImageTexture*> mAvailableRenderTargets;
#if USE_FFMPEG    
    std::map<std::string, FFMPEGCodec::Encoder*> mWriteStreams;
#endif

    EvaluationInfo mEvaluationInfo;

    int mDefaultWidth;
    int mDefaultHeight;
    bool mbSynchronousEvaluation;
    bool mUseThumbnail;
	bool mBuilding;
    unsigned int mRuntimeUniqueId; // material unique Id for thumbnail update
    int mCurrentTime;

    std::vector<int> mRemaining;

	std::map<uint32_t, bgfx::FrameBufferHandle> mProxies;
	void GetRenderProxy(bgfx::FrameBufferHandle& currentFramebuffer, int16_t width, uint16_t height, bool depthBuffer);

    void SetKeyboardMouseInfos(EvaluationInfo& evaluationInfo) const;
    void SetUniforms(size_t nodeIndex);
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
    void DrawUISingle(EvaluationContext* context, size_t nodeIndex);
    void DrawUIProgress(EvaluationContext* context, size_t nodeIndex);
} // namespace DrawUICallbacks