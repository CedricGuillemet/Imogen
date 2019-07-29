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

#ifdef GL_CLAMP_TO_BORDER
static const unsigned int wrap[] = {GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, GL_MIRRORED_REPEAT};
#else
// todogl
//static const unsigned int wrap[] = {GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT};
#endif
// todogl
//static const unsigned int filter[] = {GL_LINEAR, GL_NEAREST};
static const char* sampler2DName[] = {
    "Sampler0", "Sampler1", "Sampler2", "Sampler3", "Sampler4", "Sampler5", "Sampler6", "Sampler7"};
static const char* samplerCubeName[] = {"CubeSampler0",
                                        "CubeSampler1",
                                        "CubeSampler2",
                                        "CubeSampler3",
                                        "CubeSampler4",
                                        "CubeSampler5",
                                        "CubeSampler6",
                                        "CubeSampler7"};
/*
todogl

static const unsigned int GLBlends[] = {GL_ZERO,
                                        GL_ONE,
                                        GL_SRC_COLOR,
                                        GL_ONE_MINUS_SRC_COLOR,
                                        GL_DST_COLOR,
                                        GL_ONE_MINUS_DST_COLOR,
                                        GL_SRC_ALPHA,
                                        GL_ONE_MINUS_SRC_ALPHA,
                                        GL_DST_ALPHA,
                                        GL_ONE_MINUS_DST_ALPHA,
                                        GL_CONSTANT_COLOR,
                                        GL_ONE_MINUS_CONSTANT_COLOR,
                                        GL_CONSTANT_ALPHA,
                                        GL_ONE_MINUS_CONSTANT_ALPHA,
                                        GL_SRC_ALPHA_SATURATE};
*/
static const float rotMatrices[6][16] = {
    // toward +x
    {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},

    // -x
    {0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1},

    //+y
    {1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1},

    // -y
    {1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1},

    // +z
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},

    //-z
    {-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1}};



void EvaluationThumbnails::Clear()
{
    for (auto& atlas : mAtlases)
    {
        atlas.mTarget.Destroy();
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
    return AddThumbInAtlas(mAtlases.size() - 1);
}

void EvaluationThumbnails::DelThumb(const Thumb thumb)
{
    auto& atlas = mAtlases[thumb.mAtlasIndex];
    assert(atlas.mbUsed[thumb.mThumbIndex]);
    atlas.mbUsed[thumb.mThumbIndex] = false;
    atlas.mUsedCount --;
}

void EvaluationThumbnails::GetThumb(const Thumb thumb, unsigned int& textureId, ImRect& uvs) const
{
    textureId = mAtlases[thumb.mAtlasIndex].mTarget.mGLTexID;
    uvs = ComputeUVFromIndexInAtlas(thumb.mThumbIndex);
}

RenderTarget& EvaluationThumbnails::GetThumbTarget(const Thumb thumb)
{
    return mAtlases[thumb.mAtlasIndex].mTarget;
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

EvaluationContext::EvaluationContext(EvaluationStages& evaluation,
                                     bool synchronousEvaluation,
                                     int defaultWidth,
                                     int defaultHeight)
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
{
    mFSQuad.Init();

    // evaluation statedes
    /* todogl
	glGenBuffers(1, &mEvaluationStateGLSLBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, mEvaluationStateGLSLBuffer);

    glBufferData(GL_UNIFORM_BUFFER, sizeof(EvaluationInfo), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, mEvaluationStateGLSLBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // parameters
    glGenBuffers(1, &mParametersGLSLBuffer);
	*/
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
    mFSQuad.Finish();


	/* todogl
    glDeleteBuffers(1, &mEvaluationStateGLSLBuffer);
    glDeleteBuffers(1, &mParametersGLSLBuffer);
	*/
    Clear();
}

void EvaluationContext::AddEvaluation(size_t nodeIndex)
{
    mEvaluations.insert(mEvaluations.begin() + nodeIndex, Evaluation());
    mEvaluations[nodeIndex].mThumb = mThumbnails.AddThumb();
}

void EvaluationContext::DelEvaluation(size_t nodeIndex)
{
    mThumbnails.DelThumb(mEvaluations[nodeIndex].mThumb);
    mEvaluations.erase(mEvaluations.begin() + nodeIndex);
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

        evaluationInfo.keyModifier[0] = mUIInputs.mbCtrl ? 1 : 0;
        evaluationInfo.keyModifier[1] = mUIInputs.mbAlt ? 1 : 0;
        evaluationInfo.keyModifier[2] = mUIInputs.mbShift ? 1 : 0;
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
        if (eval.mComputeBuffer.mBuffer)
        {
			// todogl
            //glDeleteBuffers(1, &eval.mComputeBuffer.mBuffer);
        }
    }
    mThumbnails.Clear();
    mInputNodeIndex = -1;
}

unsigned int EvaluationContext::GetEvaluationTexture(size_t nodeIndex) const
{
    assert (nodeIndex < mEvaluations.size());
    const auto& tgt = mEvaluations[nodeIndex].mTarget;
    if (!tgt)
        return 0;
    return tgt->mGLTexID;
}

unsigned int bladesVertexArray;
unsigned int bladesVertexSize = 2 * sizeof(float);

unsigned int UploadIndices(const unsigned short* indices, unsigned int indexCount)
{
    unsigned int indexArray;
    /* todogl 
	glGenBuffers(1, &indexArray);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexArray);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned short), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	*/
    return indexArray;
}

void UploadVertices(const void* vertices, unsigned int vertexArraySize)
{
    /* todogl
	glGenBuffers(1, &bladesVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, bladesVertexArray);
    glBufferData(GL_ARRAY_BUFFER, vertexArraySize, vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
	*/
}

static const int tess = 10;
static unsigned int bladeIA = -1;
void drawBlades(int indexCount, int instanceCount, int elementCount)
{
    // instances
    /* todogl
	for (int i = 0; i < elementCount; i++)
        glVertexAttribDivisor(1 + i, 1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bladeIA);

    glDrawElementsInstanced(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_SHORT, (void*)0, instanceCount);

    for (int i = 0; i < elementCount; i++)
        glVertexAttribDivisor(1 + i, 0);


    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
	*/
}

void EvaluationContext::BindTextures(const EvaluationStage& evaluationStage,
                                     unsigned int program,
                                     size_t nodeIndex,
                                     std::shared_ptr<RenderTarget> reusableTarget)
{
    const Input& input = mEvaluationStages.mInputs[nodeIndex];
    for (int inputIndex = 0; inputIndex < 8; inputIndex++)
    {
		// todogl
        // glActiveTexture(GL_TEXTURE0 + inputIndex);
        int targetIndex = input.mOverrideInputs[inputIndex];
        if (targetIndex < 0)
        {
            targetIndex = input.mInputs[inputIndex];
        }
        if (targetIndex < 0)
        {
			// todogl
            // glBindTexture(GL_TEXTURE_2D, 0);
        }
        else
        {
            unsigned int parameter = -1;/* todogl glGetUniformLocation(program, sampler2DName[inputIndex]);*/
            if (parameter == 0xFFFFFFFF)
                parameter = -1; /* todogl glGetUniformLocation(program, samplerCubeName[inputIndex]);*/
            if (parameter == 0xFFFFFFFF)
            {
				// todogl
                //glBindTexture(GL_TEXTURE_2D, 0);
                continue;
            }
			// todogl
            // glUniform1i(parameter, inputIndex);

            std::shared_ptr<RenderTarget> tgt;
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
                if (tgt->mImage->mNumFaces == 1)
                {
                    /* todogl
					glBindTexture(GL_TEXTURE_2D, tgt->mGLTexID);
                    TexParam(filter[inputSampler.mFilterMin],
                             filter[inputSampler.mFilterMag],
                             wrap[inputSampler.mWrapU],
                             wrap[inputSampler.mWrapV],
                             GL_TEXTURE_2D);
							 */
                }
                else
                {
                    /* todogl
					glBindTexture(GL_TEXTURE_CUBE_MAP, tgt->mGLTexID);
                    TexParam(filter[inputSampler.mFilterMin],
                             filter[inputSampler.mFilterMag],
                             wrap[inputSampler.mWrapU],
                             wrap[inputSampler.mWrapV],
                             GL_TEXTURE_CUBE_MAP);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
                    // glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                    if (tgt->mImage->mNumMips > 1)
                    {
                        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, tgt->mImage->mNumMips - 1);
                        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    }
					*/
                }
            }
        }
    }
}

int EvaluationContext::GetBindedComputeBuffer(size_t nodeIndex) const
{
    const Input& input = mEvaluationStages.mInputs[nodeIndex];
    for (int inputIndex = 0; inputIndex < 8; inputIndex++)
    {
        const int targetIndex = input.mInputs[inputIndex];
        if (targetIndex == -1)
        {
            continue;
        }
        const auto& evaluation = mEvaluations[targetIndex];
        if (evaluation.mComputeBuffer.mBuffer && evaluation.mbActive)
        {
            return targetIndex;
        }
    }
    return -1;
}

void EvaluationContext::EvaluateGLSLCompute(const EvaluationStage& evaluationStage,
                                            size_t nodeIndex,
                                            EvaluationInfo& evaluationInfo)
{
    if (bladeIA == -1)
    {
        float bladeVertices[4 * tess];
        unsigned short bladeIndices[2 * tess];

        for (int i = 0; i < tess; i++)
        {
            bladeVertices[i * 4] = -1.f;
            bladeVertices[i * 4 + 1] = bladeVertices[i * 4 + 3] = float(i) / float(tess - 1);
            bladeVertices[i * 4 + 2] = 1.f;
        }
        for (int i = 0; i < tess * 2; i++)
        {
            bladeIndices[i] = i;
        }

        bladeIA = UploadIndices(bladeIndices, sizeof(bladeIndices) / sizeof(unsigned short));
        UploadVertices(bladeVertices, sizeof(bladeVertices));
    }

    const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mType);
    const unsigned int program = evaluator.mGLSLProgram;
    const auto& evaluation = mEvaluations[nodeIndex];

    // allocate buffer
    unsigned int feedbackVertexArray = 0;
    ComputeBuffer* destinationBuffer = NULL;
    ComputeBuffer* sourceBuffer = NULL;
    ComputeBuffer tempBuffer;
    int computeBufferIndex = GetBindedComputeBuffer(nodeIndex);
    if (computeBufferIndex != -1)
    {
        if (!mEvaluations[nodeIndex].mComputeBuffer.mBuffer)
        {
            // only allocate if needed
            AllocateComputeBuffer(int(nodeIndex),
                mEvaluations[computeBufferIndex].mComputeBuffer.mElementCount,
                mEvaluations[computeBufferIndex].mComputeBuffer.mElementSize);
        }
        sourceBuffer = &mEvaluations[computeBufferIndex].mComputeBuffer;
    }
    else
    {
        //
        Swap(mEvaluations[nodeIndex].mComputeBuffer, tempBuffer);

        AllocateComputeBuffer(int(nodeIndex), tempBuffer.mElementCount, tempBuffer.mElementSize);
        sourceBuffer = &tempBuffer;
    }

    //if (mComputeBuffers.size() <= nodeIndex)
    //    return; // no compute buffer destination, no source either -> non connected node -> early exit

    /// build source VAO
    /* todogl
	glGenVertexArrays(1, &feedbackVertexArray);
    glBindVertexArray(feedbackVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, sourceBuffer->mBuffer);
    const int transformElementCount = sourceBuffer->mElementSize / (4 * sizeof(float));
    for (int i = 0; i < transformElementCount; i++)
    {
        glVertexAttribPointer(
            i, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4 * transformElementCount, (void*)(4 * sizeof(float) * i));
        glEnableVertexAttribArray(i);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
	*/

    destinationBuffer = &mEvaluations[nodeIndex].mComputeBuffer;

    // compute buffer
    if (destinationBuffer->mElementCount)
    {
        const Parameters& parameters = mEvaluationStages.mParameters[nodeIndex];
        /* todogl
		glUseProgram(program);

        glBindBuffer(GL_UNIFORM_BUFFER, mEvaluationStateGLSLBuffer);
        evaluationInfo.vertexSpace = evaluation.mVertexSpace;
        glBufferData(GL_UNIFORM_BUFFER, sizeof(EvaluationInfo), &evaluationInfo, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, mParametersGLSLBuffer);
        glBufferData(
            GL_UNIFORM_BUFFER, parameters.size(), parameters.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);


        glBindBufferBase(GL_UNIFORM_BUFFER, 1, mParametersGLSLBuffer);
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, mEvaluationStateGLSLBuffer);


        BindTextures(evaluationStage, program, nodeIndex, std::shared_ptr<RenderTarget>());
        glEnable(GL_RASTERIZER_DISCARD);
        glBindVertexArray(feedbackVertexArray);
        glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,
                          0,
                          destinationBuffer->mBuffer,
                          0,
                          destinationBuffer->mElementCount * destinationBuffer->mElementSize);

        glBeginTransformFeedback(GL_POINTS);
        glDrawArrays(GL_POINTS, 0, destinationBuffer->mElementCount);
        glEndTransformFeedback();

        glDisable(GL_RASTERIZER_DISCARD);
        glBindVertexArray(0);
        glUseProgram(0);
		*/
    }
    if (feedbackVertexArray)
	{
		// todogl
		//glDeleteVertexArrays(1, &feedbackVertexArray);
	}

    if (tempBuffer.mBuffer)
    {
		// todogl
        //glDeleteBuffers(1, &tempBuffer.mBuffer);
    }
}

void EvaluationContext::EvaluateGLSL(const EvaluationStage& evaluationStage,
                                     size_t nodeIndex,
                                     EvaluationInfo& evaluationInfo)
{
    const Input& input = mEvaluationStages.mInputs[nodeIndex];
    const auto& evaluation = mEvaluations[nodeIndex];
    const auto tgt = evaluation.mTarget;
    const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mType);
    const unsigned int program = evaluator.mGLSLProgram;
    const int blendOps[] = {evaluation.mBlendingSrc, evaluation.mBlendingDst};
    const auto& parameters = mEvaluationStages.GetParameters(nodeIndex);
    const auto nodeType = mEvaluationStages.GetNodeType(nodeIndex);

    // todogl
	//unsigned int blend[] = {GL_ONE, GL_ZERO};

    if (!program)
    {
		// todogl
        //glUseProgram(gDefaultShader.mNodeErrorShader);
        // mFSQuad.Render();
        evaluationStage.mGScene->Draw(this, evaluationInfo);
        return;
    }
    for (int i = 0; i < 2; i++)
    {
        if (blendOps[i] < BLEND_LAST)
        {
            //blend[i] = GLBlends[blendOps[i]];
        }
    }

    // parameters
    /* todogl
	glBindBuffer(GL_UNIFORM_BUFFER, mParametersGLSLBuffer);
    glBufferData(GL_UNIFORM_BUFFER, parameters.size(), parameters.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, mParametersGLSLBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(blend[0], blend[1]);

    glUseProgram(program);
	*/
    const Camera* camera = GetCameraParameter(nodeType, parameters);
    if (camera)
    {
        camera->ComputeViewProjectionMatrix(evaluationInfo.viewProjection, evaluationInfo.viewInverse);
    }

    
    int passCount = GetIntParameter(evaluationStage.mType, parameters, "passCount", 1);
    auto transientTarget = std::make_shared<RenderTarget>(RenderTarget());
    if (passCount > 1)
    {
        // new transient target
        transientTarget->Clone(*tgt);
    }

    uint8_t mipmapCount = tgt->mImage->mNumMips;
    for (int passNumber = 0; passNumber < passCount; passNumber++)
    {
        for (int mip = 0; mip < mipmapCount; mip++)
        {
            if (!evaluationInfo.uiPass)
            {
                if (tgt->mImage->mNumFaces == 6)
                {
                    tgt->BindAsCubeTarget();
                }
                else
                {
                    tgt->BindAsTarget();
                }
            }

            size_t faceCount = evaluationInfo.uiPass ? 1 : tgt->mImage->mNumFaces;
            for (size_t face = 0; face < faceCount; face++)
            {
                if (tgt->mImage->mNumFaces == 6)
                {
                    tgt->BindCubeFace(face, mip, tgt->mImage->mWidth);
                }
                memcpy(evaluationInfo.viewRot, rotMatrices[face], sizeof(float) * 16);
                memcpy(evaluationInfo.inputIndices, input.mInputs, sizeof(input.mInputs));
                float sizeDiv = float(mip + 1);
                evaluationInfo.viewport[0] = float(tgt->mImage->mWidth) / sizeDiv;
                evaluationInfo.viewport[1] = float(tgt->mImage->mHeight) / sizeDiv;
                evaluationInfo.passNumber = passNumber;
                evaluationInfo.mipmapNumber = mip;
                evaluationInfo.mipmapCount = mipmapCount;

                /* todogl
				glBindBuffer(GL_UNIFORM_BUFFER, mEvaluationStateGLSLBuffer);
                evaluationInfo.vertexSpace = evaluation.mVertexSpace;
                glBufferData(GL_UNIFORM_BUFFER, sizeof(EvaluationInfo), &evaluationInfo, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_UNIFORM_BUFFER, 0);


                glBindBufferBase(GL_UNIFORM_BUFFER, 1, mParametersGLSLBuffer);
                glBindBufferBase(GL_UNIFORM_BUFFER, 2, mEvaluationStateGLSLBuffer);

                BindTextures(evaluationStage, program, nodeIndex, passNumber ? transientTarget : std::shared_ptr<RenderTarget>());

                glDisable(GL_CULL_FACE);
                // glCullFace(GL_BACK);
#ifdef __EMSCRIPTEN__
                glClearDepthf(1.f);
#else
                glClearDepth(1.f);
#endif
                if (evaluation.mbClearBuffer)
                {
                    glClear(GL_COLOR_BUFFER_BIT | (evaluation.mbDepthBuffer ? GL_DEPTH_BUFFER_BIT : 0));
                }
                if (evaluation.mbDepthBuffer)
                {
                    glDepthFunc(GL_LEQUAL);
                    glEnable(GL_DEPTH_TEST);
                }
				*/
                //

                if (evaluationStage.mTypename == "FurDisplay")
                {
                    /*const ComputeBuffer* buffer*/ int sourceBuffer = GetBindedComputeBuffer(nodeIndex);
                    if (sourceBuffer != -1)
                    {
                        const ComputeBuffer* buffer = &mEvaluations[sourceBuffer].mComputeBuffer;
                        unsigned int vao;
                        /* todogl
						glGenVertexArrays(1, &vao);
                        glBindVertexArray(vao);

                        // blade vertices
                        glBindBuffer(GL_ARRAY_BUFFER, bladesVertexArray);
                        glVertexAttribPointer(0 , 2, GL_FLOAT, GL_FALSE, bladesVertexSize, 0);
                        glEnableVertexAttribArray(0);
                        glBindBuffer(GL_ARRAY_BUFFER, 0);

                        // blade instances
                        const size_t transformElementCount = buffer->mElementSize / (sizeof(float) * 4);
                        glBindBuffer(GL_ARRAY_BUFFER, buffer->mBuffer);
                        for (unsigned int vp = 0; vp < transformElementCount; vp++)
                        {
                            glVertexAttribPointer(1 + vp,
                                                  4,
                                                  GL_FLOAT,
                                                  GL_FALSE,
                                                  GLsizei(sizeof(float) * 4 * transformElementCount),
                                                  (void*)(4 * sizeof(float) * vp));
                            glEnableVertexAttribArray(1 + vp);
                        }

                        glBindBuffer(GL_ARRAY_BUFFER, 0);

                        glBindVertexArray(vao);
                        drawBlades(tess * 2, buffer->mElementCount, int(transformElementCount));
                        glBindVertexArray(0);
                        glDeleteVertexArrays(1, &vao);
						*/
                    }
                }
                else
                {
                    evaluationStage.mGScene->Draw(this, evaluationInfo);
                }
            } // face
        }     // mip
        // swap target for multipass
        // set previous target as source
        if (passCount > 1 && passNumber != (passCount - 1))
        {
            transientTarget->Swap(*tgt);
        }
    } // passNumber
    if (transientTarget)
    {
        transientTarget->Destroy();
    }
	// todogl
    // glDisable(GL_BLEND);
}

void EvaluationContext::GenerateThumbnail(size_t nodeIndex)
{
    const auto& evaluation = mEvaluations[nodeIndex];
    const auto thumb = evaluation.mThumb;
    if (!thumb.Valid())
    {
        return;
    }
    const auto tgt = evaluation.mTarget;

    const int width = tgt->mImage->mWidth;
    const int height = tgt->mImage->mHeight;

    if (!width || !height)
    {
        return;
    }

    // create thumbnail
	/*
	todogl
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    
    auto thumbTarget = mThumbnails.GetThumbTarget(thumb);

    int sourceCoords[4];
    mThumbnails.GetThumbCoordinates(thumb, sourceCoords);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, tgt->mFbo);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, thumbTarget.mFbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    glBlitFramebuffer(0, 0, width, height, sourceCoords[0], sourceCoords[1], sourceCoords[2], sourceCoords[3], GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	*/
}

void EvaluationContext::EvaluateJS(const EvaluationStage& evaluationStage, size_t nodeIndex, EvaluationInfo& evaluationInfo)
{
    try
    {
        const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mType);
        int res = evaluator.RunJS((unsigned char*)mEvaluationStages.mParameters[nodeIndex].data(), &evaluationInfo, this);
        if (res == EVAL_DIRTY)
        {
            mStillDirty.push_back(uint32_t(nodeIndex));
        }
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
        const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mType);
        evaluator.RunPython();
    }
    catch (...)
    {
    }
}
#endif

void EvaluationContext::AllocRenderTargetsForEditingPreview()
{
    // alloc targets
    for (size_t i = 0; i < mEvaluationStages.GetStagesCount(); i++)
    {
        if (!mEvaluations[i].mTarget)
        {
            mEvaluations[i].mTarget = std::make_shared<RenderTarget>();
        }
    }
}

void EvaluationContext::AllocRenderTargetsForBaking(const std::vector<size_t>& nodesToEvaluate)
{
    std::vector<std::shared_ptr<RenderTarget>> freeRenderTargets;
    std::vector<uint8_t> useCount(nodesToEvaluate.size(), 0);
    // compute use count
    for (auto j = 0; j < nodesToEvaluate.size(); j++)
    {
        for (auto i = 0; i < 8; i++)
        {
            const auto input = mEvaluationStages.mInputs[j].mInputs[i];
            if (input != -1)
            {
                useCount[input] ++;
            }
        }
    }

    for (auto index : nodesToEvaluate)
    {
        if (!useCount[index])
            continue;

        if (freeRenderTargets.empty())
        {
            mEvaluations[index].mTarget = std::make_shared<RenderTarget>();
        }
        else
        {
            mEvaluations[index].mTarget = freeRenderTargets.back();
            freeRenderTargets.pop_back();
        }

        const Input& input = mEvaluationStages.mInputs[index];
        for (auto targetIndex : input.mInputs)
        {
            if (targetIndex == -1)
                continue;

            useCount[targetIndex]--;
            if (!useCount[targetIndex])
            {
                freeRenderTargets.push_back(mEvaluations[targetIndex].mTarget);
            }
        }
    }
}

void EvaluationContext::RunNode(size_t nodeIndex)
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

    mEvaluationInfo.targetIndex = int(nodeIndex);
    mEvaluationInfo.frame = mCurrentTime;
    mEvaluationInfo.dirtyFlag = evaluation.mDirtyFlag;
    memcpy(mEvaluationInfo.inputIndices, input.mInputs, sizeof(mEvaluationInfo.inputIndices));
    SetKeyboardMouseInfos(mEvaluationInfo);
    int evaluationMask = gEvaluators.GetMask(currentStage.mType);

    if (evaluationMask & EvaluationJS)
    {
        EvaluateJS(currentStage, nodeIndex, mEvaluationInfo);
    }
#ifdef USE_PYTHON
    if (evaluationMask & EvaluationPython)
    {
        EvaluatePython(currentStage, nodeIndex, mEvaluationInfo);
    }
#endif
    if (evaluationMask & EvaluationGLSLCompute)
    {
        EvaluateGLSLCompute(currentStage, nodeIndex, mEvaluationInfo);
    }

    if (evaluationMask & EvaluationGLSL)
    {
        if (!evaluation.mTarget)
        {
            evaluation.mTarget = CreateRenderTarget(nodeIndex);
        }
        if (!evaluation.mTarget->mGLTexID)
        {
            evaluation.mTarget->InitBuffer(mDefaultWidth, mDefaultHeight, evaluation.mbDepthBuffer);
        }

        EvaluateGLSL(currentStage, nodeIndex, mEvaluationInfo);
    }

    GenerateThumbnail(nodeIndex);
    evaluation.mDirtyFlag = 0;
}

bool EvaluationContext::RunNodeList(const std::vector<size_t>& nodesToEvaluate)
{
    /* todogl
	GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);

    // run C nodes
    bool anyNodeIsProcessing = false;
    for (size_t nodeIndex : nodesToEvaluate)
    {
        auto& evaluation = mEvaluations[nodeIndex];
        evaluation.mbActive = mCurrentTime >= mEvaluationStages.mStages[nodeIndex].mStartFrame &&
                             mCurrentTime <= mEvaluationStages.mStages[nodeIndex].mEndFrame;
        if (!evaluation.mbActive)
            continue;
        RunNode(nodeIndex);
        anyNodeIsProcessing |= evaluation.mProcessing != 0;
    }
    // set dirty nodes that tell so
    for (auto index : mStillDirty)
        SetTargetDirty(index, Dirty::Input);
    mStillDirty.clear();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    return anyNodeIsProcessing;
	*/
	return false;
}

void EvaluationContext::RunSingle(size_t nodeIndex, EvaluationInfo& evaluationInfo)
{
    /* todogl
	GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);

    mEvaluationInfo = evaluationInfo;

    RunNode(nodeIndex);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
	*/
}

void EvaluationContext::RecurseBackward(size_t nodeIndex, std::vector<size_t>& usedNodes)
{
    const Input& input = mEvaluationStages.mInputs[nodeIndex];

    for (size_t inputIndex = 0; inputIndex < 8; inputIndex++)
    {
        int targetIndex = input.mInputs[inputIndex];
        if (targetIndex == -1)
            continue;
        RecurseBackward(targetIndex, usedNodes);
    }

    if (std::find(usedNodes.begin(), usedNodes.end(), nodeIndex) == usedNodes.end())
        usedNodes.push_back(nodeIndex);
}

void EvaluationContext::RunDirty()
{
    memset(&mEvaluationInfo, 0, sizeof(EvaluationInfo));
    auto evaluationOrderList = mEvaluationStages.GetForwardEvaluationOrder();
    std::vector<size_t> nodesToEvaluate;
    for (size_t index = 0; index < evaluationOrderList.size(); index++)
    {
        size_t currentNodeIndex = evaluationOrderList[index];
        if (mEvaluations[currentNodeIndex].mDirtyFlag)
            nodesToEvaluate.push_back(currentNodeIndex);
    }
    AllocRenderTargetsForEditingPreview();
    RunNodeList(nodesToEvaluate);
}

void EvaluationContext::DirtyAll()
{
    // tag all as dirty
    for (auto& evaluation : mEvaluations)
    {
        evaluation.mDirtyFlag = Dirty::Parameter;
    }
}

void EvaluationContext::RunAll()
{
    DirtyAll();
    // get list of nodes to run
    memset(&mEvaluationInfo, 0, sizeof(EvaluationInfo));
    auto evaluationOrderList = mEvaluationStages.GetForwardEvaluationOrder();
    AllocRenderTargetsForEditingPreview();
    RunNodeList(evaluationOrderList);
}

bool EvaluationContext::RunBackward(size_t nodeIndex)
{
    memset(&mEvaluationInfo, 0, sizeof(EvaluationInfo));
    mEvaluationInfo.forcedDirty = true;
    std::vector<size_t> nodesToEvaluate;
    RecurseBackward(nodeIndex, nodesToEvaluate);
    AllocRenderTargetsForBaking(nodesToEvaluate);
    return RunNodeList(nodesToEvaluate);
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

void EvaluationContext::SetTargetDirty(size_t target, Dirty::Type dirtyFlag, bool onlyChild)
{
    assert(dirtyFlag != Dirty::AddedNode);
    assert(dirtyFlag != Dirty::DeletedNode);

    auto evaluationOrderList = mEvaluationStages.GetForwardEvaluationOrder();
    mEvaluations[target].mDirtyFlag = dirtyFlag;
    for (size_t i = 0; i < evaluationOrderList.size(); i++)
    {
        size_t currentNodeIndex = evaluationOrderList[i];
        if (currentNodeIndex != target)
            continue;

        for (i++; i < evaluationOrderList.size(); i++)
        {
            currentNodeIndex = evaluationOrderList[i];
            if (mEvaluations[currentNodeIndex].mDirtyFlag)
                continue;

            for (auto inp : mEvaluationStages.mInputs[currentNodeIndex].mInputs)
            {
                if (inp >= 0 && mEvaluations[inp].mDirtyFlag)
                {
                    mEvaluations[currentNodeIndex].mDirtyFlag = Dirty::Input;
                    break;
                }
            }
        }
    }
    if (onlyChild)
    {
        mEvaluations[target].mDirtyFlag = 0;
    }
}

void EvaluationContext::AllocateComputeBuffer(int target, int elementCount, int elementSize)
{
    ComputeBuffer& buffer = mEvaluations[target].mComputeBuffer;
    buffer.mElementCount = elementCount;
    buffer.mElementSize = elementSize;
    /* todogl
	if (!buffer.mBuffer)
        glGenBuffers(1, &buffer.mBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, buffer.mBuffer);
    glBufferData(GL_ARRAY_BUFFER, elementSize * elementCount, NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
	*/
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
    mMutex.lock();
    mEntries.push_back({graphName, 0.f, stages});
    mMutex.unlock();
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
    auto& evaluationStages = entry.mEvaluationStages;
    size_t stageCount = evaluationStages.mStages.size();
    for (size_t i = 0; i < stageCount; i++)
    {
        const auto& node = evaluationStages.mStages[i];
        const MetaNode& currentMeta = gMetaNodes[node.mType];
        bool forceEval = false;
        for (auto& param : currentMeta.mParams)
        {
            if (!param.mName.c_str())
                break;
            if (param.mType == Con_ForceEvaluate)
            {
                forceEval = true;
                break;
            }
        }
        if (forceEval)
        {
            EvaluationContext writeContext(evaluationStages, true, 1024, 1024);
            for (int frame = node.mStartFrame; frame <= node.mEndFrame; frame++)
            {
                writeContext.SetCurrentTime(frame);
                evaluationStages.SetTime(&writeContext, frame, false);
                evaluationStages.ApplyAnimation(&writeContext, frame);
                EvaluationInfo evaluationInfo;
                evaluationInfo.forcedDirty = 1;
                evaluationInfo.uiPass = 0;
                writeContext.RunSingle(i, evaluationInfo);
            }
        }
        entry.mProgress = float(i + 1) / float(stageCount);
        if (!mbRunning)
            break;
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
        /*todogl
		glUseProgram(gDefaultShader.mProgressShader);
        glUniform1f(glGetUniformLocation(gDefaultShader.mProgressShader, "time"),
                    float(double(SDL_GetTicks()) / 1000.0));
        context->mFSQuad.Render();
		*/
    }

    void DrawUISingle(EvaluationContext* context, size_t nodeIndex)
    {
        EvaluationInfo evaluationInfo;
        evaluationInfo.forcedDirty = 1;
        evaluationInfo.uiPass = 1;
        context->RunSingle(nodeIndex, evaluationInfo);
    }

    void DrawUICubemap(EvaluationContext* context, size_t nodeIndex)
    {
        /*todogl
		glUseProgram(gDefaultShader.mDisplayCubemapShader);
        int tgt = glGetUniformLocation(gDefaultShader.mDisplayCubemapShader, "samplerCubemap");
        glUniform1i(tgt, 0);
        glActiveTexture(GL_TEXTURE0);
        TexParam(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_CUBE_MAP);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, context->GetEvaluationTexture(nodeIndex));
        context->mFSQuad.Render();
		*/
    }
} // namespace DrawUICallbacks
