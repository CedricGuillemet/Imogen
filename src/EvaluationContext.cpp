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

#include "Platform.h"
#include <memory>
#include "EvaluationContext.h"
#include "Evaluators.h"
#include "GraphControler.h"

static const float rotMatrices[6][16] = {
    // toward +x
    {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},

    // -x
    {0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1},

	// -y
{1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1},

    //+y
    {1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1},



    // +z
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},

    //-z
    {-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1}};


void EvaluationThumbnails::Clear()
{
    for (auto& atlas : mAtlases)
    {
        atlas.mTarget.Destroy();
		bgfx::destroy(atlas.mFrameBuffer);
    }
    mAtlases.clear();
}

EvaluationThumbnails::Thumb EvaluationThumbnails::AddThumbInAtlas(size_t atlasIndex)
{
    auto& atlas = mAtlases[atlasIndex];
    for (size_t thumbIndex = 0; thumbIndex < ThumbnailsPerAtlas; thumbIndex++)
    {
        if (!atlas.mbUsed[thumbIndex])
        {
            atlas.mbUsed[thumbIndex] = true;
            atlas.mUsedCount++;
            return { (unsigned short)atlasIndex, (unsigned short)thumbIndex };
        }
    }
    // mUsedCount not in sync with used map
    assert(0);
    return {};
}

EvaluationThumbnails::Thumb EvaluationThumbnails::AddThumb()
{
    for (size_t atlasIndex = 0; atlasIndex < mAtlases.size(); atlasIndex++)
    {
        const auto& atlas = mAtlases[atlasIndex];
        if (atlas.mUsedCount < ThumbnailsPerAtlas)
        {
            return AddThumbInAtlas(atlasIndex);
        }
    }
    // no atlas, create new one
    mAtlases.push_back(ThumbAtlas(ThumbnailsPerAtlas));
    mAtlases.back().mTarget.InitBuffer(int(AtlasSize), int(AtlasSize), false);

	bgfx::Attachment at;
	at.init(mAtlases.back().mTarget.mGLTexID, bgfx::Access::Write, 0);
	mAtlases.back().mFrameBuffer = bgfx::createFrameBuffer(1, &at);

    return AddThumbInAtlas(mAtlases.size() - 1);
}

void EvaluationThumbnails::DelThumb(const Thumb thumb)
{
    auto& atlas = mAtlases[thumb.mAtlasIndex];
    assert(atlas.mbUsed[thumb.mThumbIndex]);
    atlas.mbUsed[thumb.mThumbIndex] = false;
    atlas.mUsedCount --;
}

void EvaluationThumbnails::GetThumb(const Thumb thumb, TextureHandle& textureHandle, ImRect& uvs) const
{
    textureHandle = mAtlases[thumb.mAtlasIndex].mTarget.mGLTexID;
    uvs = ComputeUVFromIndexInAtlas(thumb.mThumbIndex);
}

RenderTarget& EvaluationThumbnails::GetThumbTarget(const Thumb thumb)
{
    return mAtlases[thumb.mAtlasIndex].mTarget;
}

bgfx::FrameBufferHandle& EvaluationThumbnails::GetThumbFrameBuffer(const Thumb thumb)
{
	return mAtlases[thumb.mAtlasIndex].mFrameBuffer;
	
}

ImRect EvaluationThumbnails::ComputeUVFromIndexInAtlas(size_t thumbIndex) const
{
    const size_t thumbnailsPerSide = AtlasSize / ThumbnailSize;
    const size_t indexY = thumbIndex / thumbnailsPerSide;
    const size_t indexX = thumbIndex % thumbnailsPerSide;

    const float u = float(indexX) / float(thumbnailsPerSide);
    const float v = float(indexY) / float(thumbnailsPerSide);
    const float suv = 1.f / float(thumbnailsPerSide);
    return ImRect(ImVec2(u, v + suv), ImVec2(u + suv, v));
}

void EvaluationThumbnails::GetThumbCoordinates(const Thumb thumb, int* coordinates) const
{
    const size_t index = thumb.mThumbIndex;
    const size_t thumbnailsPerSide = AtlasSize / ThumbnailSize;
    const size_t indexY = index / thumbnailsPerSide;
    const size_t indexX = index % thumbnailsPerSide;

    coordinates[0] = int(indexX * ThumbnailSize);
    coordinates[1] = int(indexY * ThumbnailSize);
    coordinates[2] = int(coordinates[0] + ThumbnailSize - 1);
    coordinates[3] = int(coordinates[1] + ThumbnailSize - 1); 
}

std::vector<RenderTarget> EvaluationThumbnails::GetAtlasTextures() const
{
    std::vector<RenderTarget> ret;
    for (auto& atlas : mAtlases)
    {
        ret.push_back(atlas.mTarget);
    }
    return ret;
}

EvaluationContext::EvaluationContext(EvaluationStages& evaluation
									 , bool building
                                     , bool synchronousEvaluation
                                     , int defaultWidth
                                     , int defaultHeight
                                     , bool useThumbnail)
    : mEvaluationStages(evaluation)
#ifdef __EMSCRIPTEN
    , mbSynchronousEvaluation(true)
#else
    , mbSynchronousEvaluation(synchronousEvaluation)
#endif
    , mDefaultWidth(defaultWidth)
    , mDefaultHeight(defaultHeight)
    , mRuntimeUniqueId(-1)
    , mInputNodeIndex(-1)
    , mUseThumbnail(useThumbnail)
	, mBuilding(building)
{

    mEvaluations.resize(evaluation.mStages.size());
}

EvaluationContext::~EvaluationContext()
{
#ifdef USE_FFMPEG
    for (auto& stream : mWriteStreams)
    {
        stream.second->Finish();
        delete stream.second;
    }
    
    mWriteStreams.clear();
#endif

    Clear();
}

void EvaluationContext::AddEvaluation(size_t nodeIndex)
{
    mEvaluations.insert(mEvaluations.begin() + nodeIndex, Evaluation());
    if (mUseThumbnail)
    {
        mEvaluations[nodeIndex].mThumb = mThumbnails.AddThumb();
    }
}

void EvaluationContext::DelEvaluation(size_t nodeIndex)
{
    if (mUseThumbnail)
    {
        mThumbnails.DelThumb(mEvaluations[nodeIndex].mThumb);
    }
    // set nodes using that node to be dirty
    SetTargetDirty(nodeIndex, Dirty::Input, false);
	if (mEvaluations[nodeIndex].mTarget)
	{
		delete mEvaluations[nodeIndex].mTarget;
	}
    mEvaluations.erase(mEvaluations.begin() + nodeIndex);
}

void EvaluationContext::Evaluate()
{
    //Has dirty? be smart and discard nodes from mRemaining when tagged as dirty
    bool hasDirty = false;
    for (auto& evaluation : mEvaluations)
    {
        if (evaluation.mDirtyFlag)
        {
            hasDirty = true;
            break;
        }
    }

    // clear todo list
    if (hasDirty)
    {
        mRemaining.clear();

        auto evaluationOrderList = mEvaluationStages.GetForwardEvaluationOrder();
        for (size_t index = 0; index < evaluationOrderList.size(); index++)
        {
            size_t currentNodeIndex = evaluationOrderList[index];
            if (mEvaluations[currentNodeIndex].mDirtyFlag)
            {
                mRemaining.push_back(int32_t(currentNodeIndex));
            }
        }
        ComputeTargetUseCount();
    }

    // do something from the list
    if (!mRemaining.empty())
    {
        bgfx::ViewId viewId = viewId_Evaluation;
        static const int nodeCountPerIteration = 100;
        for (int i = 0;i< nodeCountPerIteration;i++)
        {
            int nodeIndex = mRemaining.front();
            mRemaining.erase(mRemaining.begin()); //TODOEVA don't remove from remaining(or push it back after) when node needs more work to be done (raytracer)
            auto& evaluation = mEvaluations[nodeIndex];
            evaluation.mbActive = mCurrentTime >= mEvaluationStages.mStages[nodeIndex].mStartFrame &&
                mCurrentTime <= mEvaluationStages.mStages[nodeIndex].mEndFrame;
            /*if (!evaluation.mbActive) TODOEVA
            {
                continue;
            }*/
            
            RunNode(viewId, nodeIndex);

            if (mRemaining.empty())
            {
                break;
            }
        }
    }
}

void EvaluationContext::SetKeyboardMouse(size_t nodeIndex, const UIInput& input)
{
    mUIInputs = input;
    mInputNodeIndex = nodeIndex;
}

void EvaluationContext::SetKeyboardMouseInfos(EvaluationInfo& evaluationInfo) const
{
    if (mInputNodeIndex == evaluationInfo.targetIndex)
    {
        evaluationInfo.mouse[0] = mUIInputs.mRx;
        evaluationInfo.mouse[1] = 1.f - mUIInputs.mRy;
        evaluationInfo.mouse[2] = mUIInputs.mLButDown ? 1.f : 0.f;
        evaluationInfo.mouse[3] = mUIInputs.mRButDown ? 1.f : 0.f;

        evaluationInfo.keyModifier[0] = mUIInputs.mbCtrl ? 1.f : 0.f;
        evaluationInfo.keyModifier[1] = mUIInputs.mbAlt ? 1.f : 0.f;
        evaluationInfo.keyModifier[2] = mUIInputs.mbShift ? 1.f : 0.f;
        evaluationInfo.keyModifier[3] = 0;
    }
    else
    {
        evaluationInfo.mouse[0] = -99999.f;
        evaluationInfo.mouse[1] = -99999.f;
        evaluationInfo.mouse[2] = 0.f;
        evaluationInfo.mouse[3] = 0.f;

        evaluationInfo.keyModifier[0] = 0;
        evaluationInfo.keyModifier[1] = 0;
        evaluationInfo.keyModifier[2] = 0;
        evaluationInfo.keyModifier[3] = 0;
    }
}

void EvaluationContext::Clear()
{
    for (auto& eval : mEvaluations)
    {
        if (eval.mTarget)
        {
            eval.mTarget->Destroy();
        }
    }
    mThumbnails.Clear();
    mInputNodeIndex = -1;
}

TextureHandle EvaluationContext::GetEvaluationTexture(size_t nodeIndex) const
{
    assert (nodeIndex < mEvaluations.size());
    const auto& tgt = mEvaluations[nodeIndex].mTarget;
    if (!tgt)
	{
        return {bgfx::kInvalidHandle};
	}
    return tgt->mGLTexID;
}

void EvaluationContext::BindTextures(const EvaluationStage& evaluationStage,
                                     size_t nodeIndex,
                                     RenderTarget* reusableTarget)
{
    const Input& input = mEvaluationStages.mInputs[nodeIndex];
    for (int inputIndex = 0; inputIndex < 8; inputIndex++)
    {
        int targetIndex = input.mOverrideInputs[inputIndex];
        if (targetIndex < 0)
        {
            targetIndex = input.mInputs[inputIndex];
        }
        if (targetIndex < 0)
        {
        }
        else
        {
            RenderTarget* tgt;
            if (inputIndex == 0 && reusableTarget)
            {
                tgt = reusableTarget;
            }
            else
            {
                tgt = mEvaluations[targetIndex].mTarget;
            }

            if (tgt)
            {
                const InputSampler& inputSampler = mEvaluationStages.mInputSamplers[nodeIndex][inputIndex];
                if (!tgt->mImage.mIsCubemap)
                {

                    bgfx::setTexture(inputIndex, gEvaluators.mSamplers2D[inputIndex], tgt->mGLTexID, inputSampler.Value());
                }
                else
                {
                    bgfx::setTexture(inputIndex+8, gEvaluators.mSamplersCube[inputIndex], tgt->mGLTexID, inputSampler.Value());
                }
            }
        }
    }
}

void EvaluationContext::SetUniforms(size_t nodeIndex)
{
    float tempUniforms[8 * 4]; // max size for ramp
    const Parameters& parameters = mEvaluationStages.mParameters[nodeIndex];
    const auto& evaluationStage = mEvaluationStages.mStages[nodeIndex];
    auto nodeType = evaluationStage.mType;
    const auto& evaluator = gEvaluators.GetEvaluator(evaluationStage.mType);
    const unsigned char* ptr = parameters.data();
    int paramIndex = 0;
    for (const auto& uniform : evaluator.mUniformHandles)
    {
        const auto& parameter = gMetaNodes[nodeType].mParams[paramIndex];
        auto paramSize = GetParameterTypeSize(parameter.mType);
        int count = 1;
        bool *ptrb = (bool*)ptr;
        int* ptri = (int*)ptr;
        float *ptrf = (float*)ptr;
        enum SubType
        {
            ST_Float,
            ST_Bool,
            ST_Int
        };
        SubType st = ST_Float;
        switch (parameter.mType)
        {
        case Con_Float:
            break;
        case Con_Float2:
            break;
        case Con_Float3:
            break;
        case Con_Float4:
            break;
        case Con_Color4:
            break;
        case Con_Int:
            st = ST_Int;
            break;
        case Con_Int2:
            st = ST_Int;
            break;
        case Con_Ramp:
            count = 8;
            break;
        case Con_Angle:
            break;
        case Con_Angle2:
            break;
        case Con_Angle3:
            break;
        case Con_Angle4:
            break;
        case Con_Enum:
            st = ST_Int;
            break;
        case Con_Any:
        case Con_Multiplexer:
        case Con_Camera:
        case Con_Structure:
        case Con_FilenameRead:
        case Con_FilenameWrite:
        case Con_ForceEvaluate:
            count = 0;
            continue;
        case Con_Bool:
            st = ST_Bool;
            break;
        case Con_Ramp4:
            count = 8;
            break;
        }

        if (count)
        {
            switch(st)
            {
            case ST_Bool:
                for (int i = 0; i < paramSize / sizeof(int); i++)
                {
                    tempUniforms[i] = (float)((* ptrb++)?1.f:0.f);
                }
                break;
            case ST_Float:
                memcpy(tempUniforms, ptr, paramSize);
                break;
            case ST_Int:
                for (int i = 0; i < paramSize/sizeof(int); i++)
                {
                    tempUniforms[i] = (float)*ptri++;
                }
                break;
            }
            bgfx::setUniform(uniform, tempUniforms, count);
        }
        paramIndex++;
        ptr += paramSize;
    }
}


void EvaluationContext::EvaluateGLSL(const EvaluationStage& evaluationStage,
                                     bgfx::ViewId& viewId,
                                     size_t nodeIndex,
                                     EvaluationInfo& evaluationInfo)
{
    const Input& input = mEvaluationStages.mInputs[nodeIndex];
    const auto& evaluation = mEvaluations[nodeIndex];
    const auto tgt = evaluation.mTarget;
    const auto& evaluator = gEvaluators.GetEvaluator(evaluationStage.mType);
    const ProgramHandle program = evaluator.mProgram;
    const auto& parameters = mEvaluationStages.GetParameters(nodeIndex);
    const auto nodeType = mEvaluationStages.GetNodeType(nodeIndex);

    if (!program.idx)
    {
        return;
    }

    const Camera* camera = GetCameraParameter(nodeType, parameters);
    if (camera)
    {
        camera->ComputeViewProjectionMatrix(evaluationInfo.viewProjection, evaluationInfo.viewInverse);
    }

    
    int passCount = GetIntParameter(evaluationStage.mType, parameters, "passCount", 1);
    RenderTarget* transientTarget = nullptr;
    if (passCount > 1)
    {
        // new transient target
        transientTarget = AcquireClone(tgt);
    }

    auto w = tgt->mImage.mWidth;
    auto h = tgt->mImage.mHeight;
	FrameBufferHandle proxyFrameBuffer[16];
	for (int i = 0;i<sizeof(proxyFrameBuffer)/sizeof(FrameBufferHandle);i++)
	{
		proxyFrameBuffer[i] = {bgfx::kInvalidHandle};
	}
	size_t faceCount = evaluationInfo.uiPass ? 1 : (tgt->mImage.mIsCubemap ? 6 : 1);
	const uint8_t mipmapCount = tgt->mImage.GetMipmapCount();
	for (auto i = 0;i< mipmapCount;i++)
	{
		proxyFrameBuffer[i] = bgfx::createFrameBuffer(w>>i, h>>i, bgfx::TextureFormat::BGRA8);
	}
	

    
    for (int passNumber = 0; passNumber < passCount; passNumber++)
    {
        for (int mip = 0; mip < mipmapCount; mip++)
        {
			FrameBufferHandle& currentProxy = proxyFrameBuffer[mip];
			for (size_t face = 0; face < faceCount; face++)
			{
				const int viewportWidth = w >> mip;
				const int viewportHeight = h >> mip;
				bgfx::setViewName(viewId, gMetaNodes[evaluator.mType].mName.c_str());
				bgfx::setViewMode(viewId, bgfx::ViewMode::Sequential);
				bgfx::setViewFrameBuffer(viewId, currentProxy);
				bgfx::setViewRect(viewId, 0, 0, viewportWidth , viewportHeight);
				//bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xF03030ff+face*0x2000, 1.0f, 0);

                memcpy(evaluationInfo.viewRot, rotMatrices[face], sizeof(float) * 16);
				for (int i = 0; i < 8; i++)
				{
					evaluationInfo.inputIndices[i] = input.mInputs[i];
				}

                evaluationInfo.viewport[0] = float(viewportWidth);
                evaluationInfo.viewport[1] = float(viewportHeight);
                evaluationInfo.passNumber = float(passNumber);
                evaluationInfo.mipmapNumber = float(mip);
                evaluationInfo.mipmapCount = mipmapCount;

                BindTextures(evaluationStage, nodeIndex, passNumber ? transientTarget : nullptr);

                if (evaluation.mbClearBuffer)
                {
                    bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
                }

                SetUniforms(nodeIndex);
                
                {
                    uint64_t state = 0
                        | BGFX_STATE_WRITE_RGB
                        | BGFX_STATE_WRITE_A
                        | BGFX_STATE_BLEND_FUNC(evaluation.mBlendingSrc, evaluation.mBlendingDst)
                        | (evaluation.mbDepthBuffer ? BGFX_STATE_DEPTH_TEST_LEQUAL : BGFX_STATE_DEPTH_TEST_ALWAYS)
                        ;

                    bgfx::setState(state);
                    static const float uvt[4] = { 2.f, -2.f, -1.0f, 1.0f };
                    bgfx::setUniform(gEvaluators.u_uvTransform, uvt);
                    evaluationStage.mGScene->Draw(evaluationInfo, viewId, program);
                }

                // copy from proxy to destination
                viewId++;
				bgfx::touch(viewId);
				bgfx::setViewFrameBuffer(viewId, {0});
				bgfx::setViewRect(viewId, 0, 0, viewportWidth, viewportHeight);
				bgfx::blit(viewId++, tgt->mGLTexID, mip, 0, 0, face, bgfx::getTexture(currentProxy), 0, 0, 0, 0, viewportWidth, viewportHeight);
            } // face
        } // mip
        // swap target for multipass
        // set previous target as source
        if (passCount > 1 && passNumber != (passCount - 1))
        {
            transientTarget->Swap(*tgt);
        }
    } // passNumber
	for (auto i =0;i< mipmapCount;i++)
	{
		if (proxyFrameBuffer[i].idx != bgfx::kInvalidHandle)
		{
			bgfx::destroy(proxyFrameBuffer[i]);
		}
	}
    if (transientTarget)
    {
        transientTarget->Destroy();
    }
    viewId++;
}

void EvaluationContext::GenerateThumbnail(bgfx::ViewId& viewId, size_t nodeIndex)
{
    assert(mUseThumbnail);
    const auto& evaluation = mEvaluations[nodeIndex];
    const auto thumb = evaluation.mThumb;
    if (!thumb.Valid())
    {
        return;
    }
    const auto tgt = evaluation.mTarget;
	if (!tgt)
	{
		return; // can be null for file read/file write
	}
    const int width = tgt->mImage.mWidth;
    const int height = tgt->mImage.mHeight;

    if (!width || !height)
    {
        return;
    }
    // create thumbnail
	auto thumbFrameBuffer = mThumbnails.GetThumbFrameBuffer(thumb);

    auto def = Scene::BuildDefaultScene();
    int sourceCoords[4];
    mThumbnails.GetThumbCoordinates(thumb, sourceCoords);
    bgfx::setViewName(viewId, "Make Thumbnail");
    bgfx::setViewMode(viewId, bgfx::ViewMode::Sequential);
    bgfx::setViewFrameBuffer(viewId, thumbFrameBuffer);
    auto h = sourceCoords[3] - sourceCoords[1];
    if (bgfx::getRendererType() == bgfx::RendererType::OpenGL || bgfx::getRendererType() == bgfx::RendererType::OpenGLES)
    {
        bgfx::setViewRect(viewId, sourceCoords[0], mThumbnails.GetAtlasTextures()[0].mImage.mHeight - sourceCoords[1] - h, sourceCoords[2] - sourceCoords[0], h);
    }
    else
    {
        bgfx::setViewRect(viewId, sourceCoords[0], sourceCoords[1], sourceCoords[2] - sourceCoords[0], h);
    }
    bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x3030F0ff, 1.0f, 0);
    uint64_t state = 0
        | BGFX_STATE_WRITE_RGB
        | BGFX_STATE_WRITE_A
        | BGFX_STATE_DEPTH_TEST_ALWAYS
        ;

    bgfx::setState(state);
    bgfx::setTexture(0, gEvaluators.mSamplers2D[0], tgt->mGLTexID);

    static const float uvt[4] = { 2.f, -2.f, -1.0f, 1.0f };
    bgfx::setUniform(gEvaluators.u_uvTransform, uvt);
    
    EvaluationInfo evaluationInfo = {0};
    def->Draw(evaluationInfo, viewId, gEvaluators.mBlitProgram);

	
    viewId++;
}

void EvaluationContext::EvaluateC(const EvaluationStage& evaluationStage, size_t nodeIndex, EvaluationInfo& evaluationInfo)
{
    try
    {
        const auto& evaluator = gEvaluators.GetEvaluator(evaluationStage.mType);
        int res = evaluator.mCFunction((unsigned char*)mEvaluationStages.mParameters[nodeIndex].data(), &evaluationInfo, this);
    }
    catch (std::exception e)
    {
        printf("Duktape exception %s\n", e.what());
    }
}

#if USE_PYTHON
void EvaluationContext::EvaluatePython(const EvaluationStage& evaluationStage,
                                       size_t index,
                                       EvaluationInfo& evaluationInfo)
{
    try // todo: find a better solution than a try catch
    {
        const auto& evaluator = gEvaluators.GetEvaluator(evaluationStage.mType);
        evaluator.RunPython();
    }
    catch (...)
    {
    }
}
#endif

void EvaluationContext::ComputeTargetUseCount()
{
    for (auto& evaluation : mEvaluations)
    {
        evaluation.mUseCount = 0;
    }
    for (auto j = 0; j < mEvaluationStages.mStages.size(); j++)
    {
        for (auto i = 0; i < 8; i++)
        {
            const auto input = mEvaluationStages.mInputs[j].mInputs[i];
            if (input != -1)
            {
                mEvaluations[input].mUseCount ++;
            }
        }
    }
}

void EvaluationContext::ReleaseInputs(size_t nodeIndex)
{
    // is this node used anytime soon?
    if (!mEvaluations[nodeIndex].mUseCount && mEvaluations[nodeIndex].mTarget && !mEvaluations[nodeIndex].mbPersistent)
    {
        ReleaseRenderTarget(mEvaluations[nodeIndex].mTarget);
        mEvaluations[nodeIndex].mTarget = nullptr;
    }
    // decrement use of inputs
    for (auto i = 0; i < 8; i++)
    {
        const auto input = mEvaluationStages.mInputs[nodeIndex].mInputs[i];
        if (input != -1)
        {
            // use count must be positive
            assert(mEvaluations[input].mUseCount);

            mEvaluations[input].mUseCount--;
            if (mEvaluations[input].mUseCount <= 0 && mEvaluations[input].mTarget && !mEvaluations[input].mbPersistent)
            {
                ReleaseRenderTarget(mEvaluations[input].mTarget);
                mEvaluations[input].mTarget = nullptr;
            }
        }
    }
}

void EvaluationContext::RunNode(bgfx::ViewId& viewId, size_t nodeIndex)
{
    auto& currentStage = mEvaluationStages.mStages[nodeIndex];
    const Input& input = mEvaluationStages.mInputs[nodeIndex];
    auto& evaluation = mEvaluations[nodeIndex];

    // check processing
    for (auto& inp : input.mInputs)
    {
        if (inp < 0)
            continue;
        if (mEvaluations[inp].mProcessing)
        {
            evaluation.mProcessing = 1;
            return;
        }
    }

    evaluation.mProcessing = 0;
    memset(&mEvaluationInfo, 0, sizeof(EvaluationInfo));
    mEvaluationInfo.targetIndex = float(nodeIndex);
    mEvaluationInfo.frame = float(mCurrentTime);
    mEvaluationInfo.dirtyFlag = evaluation.mDirtyFlag;
    //memcpy(mEvaluationInfo.inputIndices, input.mInputs, sizeof(mEvaluationInfo.inputIndices));
	for (int i = 0; i < 8; i++)
	{
		mEvaluationInfo.inputIndices[i] = input.mInputs[i];
	}
    SetKeyboardMouseInfos(mEvaluationInfo);
    int evaluationMask = gEvaluators.GetMask(currentStage.mType);

    if (evaluationMask & EvaluationC)
    {
        EvaluateC(currentStage, nodeIndex, mEvaluationInfo);
    }
#ifdef USE_PYTHON
    if (evaluationMask & EvaluationPython)
    {
        EvaluatePython(currentStage, nodeIndex, mEvaluationInfo);
    }
#endif

    if (evaluationMask & EvaluationGLSL)
    {
        // target might be allocated by any node evaluator before
        if (!evaluation.mTarget)
        {
            evaluation.mTarget = AcquireRenderTarget(mDefaultWidth, mDefaultHeight, evaluation.mbDepthBuffer);
        }
        EvaluateGLSL(currentStage, viewId, nodeIndex, mEvaluationInfo);
    }

    if (mUseThumbnail)
    {
        GenerateThumbnail(viewId, nodeIndex);
    }

    ReleaseInputs(nodeIndex);
    evaluation.mDirtyFlag = 0;
}

#if USE_FFMPEG
FFMPEGCodec::Encoder* EvaluationContext::GetEncoder(const std::string& filename, int width, int height)
{
    FFMPEGCodec::Encoder* encoder;
    auto iter = mWriteStreams.find(filename);
    if (iter != mWriteStreams.end())
    {
        encoder = iter->second;
    }
    else
    {
        encoder = new FFMPEGCodec::Encoder;
        mWriteStreams[filename] = encoder;
        encoder->Init(filename, align(width, 4), align(height, 4), 25, 400000);
    }
    return encoder;
}
#endif

void EvaluationContext::SetTargetDirty(size_t target, uint32_t dirtyFlag, bool onlyChild)
{
    assert(dirtyFlag != Dirty::AddedNode);
    assert(dirtyFlag != Dirty::DeletedNode);

    auto evaluationOrderList = mEvaluationStages.GetForwardEvaluationOrder();
	uint32_t dirtyFlagSource = mEvaluations[target].mDirtyFlag;
    mEvaluations[target].mDirtyFlag |= dirtyFlag;
    for (size_t i = 0; i < evaluationOrderList.size(); i++)
    {
        size_t currentNodeIndex = evaluationOrderList[i];
        if (currentNodeIndex != target)
		{
            continue;
		}
        for (i++; i < evaluationOrderList.size(); i++)
        {
            currentNodeIndex = evaluationOrderList[i];
            if (mEvaluations[currentNodeIndex].mDirtyFlag)
			{
                continue;
			}
            for (auto inp : mEvaluationStages.mInputs[currentNodeIndex].mInputs)
            {
                if (inp >= 0 && mEvaluations[inp].mDirtyFlag)
                {
                    mEvaluations[currentNodeIndex].mDirtyFlag |= Dirty::Input;
                    break;
                }
            }
        }
    }
    if (onlyChild)
    {
        mEvaluations[target].mDirtyFlag = dirtyFlagSource;
    }
}

void EvaluationContext::SetTargetPersistent(size_t nodeIndex, bool persistent)
{
	mEvaluations[nodeIndex].mbPersistent = persistent;
}

void EvaluationContext::StageSetProcessing(size_t target, int processing)
{
    if (mEvaluations[target].mProcessing != processing)
    {
        mEvaluations[target].mProgress = 0.f;
    }
    mEvaluations[target].mProcessing = processing;
}

void EvaluationContext::StageSetProgress(size_t target, float progress)
{
    mEvaluations[target].mProgress = progress;
}


RenderTarget* EvaluationContext::AcquireRenderTarget(int width, int height, bool depthBuffer)
{
    for (size_t i = 0; i < mAvailableRenderTargets.size(); i++)
    {
        RenderTarget* rt = mAvailableRenderTargets[i];
        if (rt->mImage.mWidth == width && rt->mImage.mHeight == height && (rt->mGLTexDepth.idx != bgfx::kInvalidHandle) == depthBuffer)
        {
            mAvailableRenderTargets.erase(mAvailableRenderTargets.begin() + i);
            return rt;
        }
    }
    RenderTarget* rt = new RenderTarget;
    rt->InitBuffer(width, height, depthBuffer);
    return rt;
}

RenderTarget* EvaluationContext::AcquireClone(RenderTarget* source)
{
    return AcquireRenderTarget(source->mImage.mWidth, source->mImage.mHeight, source->mGLTexDepth.idx != bgfx::kInvalidHandle);
}

void EvaluationContext::ReleaseRenderTarget(RenderTarget* renderTarget)
{
    assert(renderTarget);
    mAvailableRenderTargets.push_back(renderTarget);
}


std::vector<RenderTarget*> mAvailableRenderTargets;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Builder::Builder() : mbRunning(true)
{
#ifndef __EMSCRIPTEN__
    mThread = std::thread([&]() { BuildEntries(); });
#endif    
}

Builder::~Builder()
{
    mbRunning = false;
    mThread.join();
}

void Builder::Add(const char* graphName, const EvaluationStages& stages)
{
    /* aync
    mMutex.lock();
    mEntries.push_back({graphName, 0.f, stages});
    mMutex.unlock();
    */
    // sync 
    Builder::Entry entry{ graphName, 0.f, stages };
    DoBuild(entry);
}

void Builder::Add(Material* material)
{
    try
    {
        EvaluationStages stages;
        stages.BuildEvaluationFromMaterial(*material);
        Add(material->mName.c_str(), stages);
    }
    catch (std::exception e)
    {
        Log("Exception : %s\n", e.what());
    }
}

bool Builder::UpdateBuildInfo(std::vector<BuildInfo>& buildInfo)
{
    if (mMutex.try_lock())
    {
        buildInfo.clear();
        for (auto& entry : mEntries)
        {
            buildInfo.push_back({entry.mName, entry.mProgress});
        }
        mMutex.unlock();
        return true;
    }
    return false;
}

void Builder::DoBuild(Entry& entry)
{
	auto & evaluationStages = entry.mEvaluationStages;
	int startFrame(INT_MAX), endFrame(INT_MIN);
	for (auto& evaluation : evaluationStages.mStages)
	{
		startFrame = std::min(startFrame, evaluation.mStartFrame);
		endFrame = std::max(endFrame, evaluation.mEndFrame);
	}
    EvaluationContext writeContext(evaluationStages, true, true, 4096, 4096, false);

	// dirty all or parameters might not be taken into consideration
	for (auto& evaluation : writeContext.mEvaluations)
	{
		evaluation.mDirtyFlag = -1;
	}
    for (int frame = startFrame; frame < endFrame; frame++)
    {
        writeContext.SetCurrentTime(frame);
        evaluationStages.SetTime(&writeContext, frame, false);
        evaluationStages.ApplyAnimation(&writeContext, frame);
        writeContext.Evaluate();
    }
}

void MakeThreadContext();
void Builder::BuildEntries()
{
    MakeThreadContext();

    while (mbRunning)
    {
        if (mMutex.try_lock())
        {
            if (!mEntries.empty())
            {
                auto& entry = *mEntries.begin();
                entry.mProgress = 0.01f;
                DoBuild(entry);
                entry.mProgress = 1.f;
                if (entry.mProgress >= 1.f)
                {
                    mEntries.erase(mEntries.begin());
                }
            }
            mMutex.unlock();
        }
#ifdef Sleep
        Sleep(100);
#endif
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace DrawUICallbacks
{
    void DrawUIProgress(EvaluationContext* context, size_t nodeIndex)
    {
        auto def = Scene::BuildDefaultScene();

        uint64_t state = 0
            | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_DEPTH_TEST_ALWAYS
            ;

        bgfx::setState(state);
        float uniform[] = { float(double(SDL_GetTicks()) / 1000.0), 0.f, 0.f, 0.f};
        bgfx::setUniform(gEvaluators.u_time, uniform);

        EvaluationInfo evaluationInfo;
        def->Draw(evaluationInfo, viewId_ImGui, gEvaluators.mProgressProgram);
    }

    void DrawUISingle(EvaluationContext* context, size_t nodeIndex)
    {
        EvaluationInfo evaluationInfo;
        //evaluationInfo.forcedDirty = 1;
        evaluationInfo.uiPass = 1;
        //context->RunSingle(nodeIndex, viewId_ImGui, evaluationInfo); TODOEVA
    }

    void DrawUICubemap(EvaluationContext* context, size_t nodeIndex)
    {
        auto def = Scene::BuildDefaultScene();

        uint64_t state = 0
            | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_DEPTH_TEST_ALWAYS
            ;

        bgfx::setState(state);
        bgfx::setTexture(0, gEvaluators.mSamplersCube[0], context->GetEvaluationTexture(nodeIndex));
        EvaluationInfo evaluationInfo;
        def->Draw(evaluationInfo, viewId_ImGui, gEvaluators.mDisplayCubemapProgram);
    }
} // namespace DrawUICallbacks
