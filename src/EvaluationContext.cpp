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

#include <SDL.h>
#include <GL/gl3w.h> // Initialize with gl3wInit()
#include <memory>
#include "EvaluationContext.h"
#include "Evaluators.h"
#include "NodeGraphControler.h"

static const unsigned int wrap[] = {GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, GL_MIRRORED_REPEAT};
static const unsigned int filter[] = {GL_LINEAR, GL_NEAREST};
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


EvaluationContext::EvaluationContext(EvaluationStages& evaluation,
                                     bool synchronousEvaluation,
                                     int defaultWidth,
                                     int defaultHeight)
    : mEvaluationStages(evaluation)
    , mbSynchronousEvaluation(synchronousEvaluation)
    , mDefaultWidth(defaultWidth)
    , mDefaultHeight(defaultHeight)
    , mRuntimeUniqueId(-1)
{
    mFSQuad.Init();

    // evaluation statedes
    glGenBuffers(1, &mEvaluationStateGLSLBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, mEvaluationStateGLSLBuffer);

    glBufferData(GL_UNIFORM_BUFFER, sizeof(EvaluationInfo), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, mEvaluationStateGLSLBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // parameters
    glGenBuffers(1, &mParametersGLSLBuffer);
}

EvaluationContext::~EvaluationContext()
{
    for (auto& stream : mWriteStreams)
    {
        stream.second->Finish();
        delete stream.second;
    }
    mWriteStreams.clear();
    mFSQuad.Finish();

    glDeleteBuffers(1, &mEvaluationStateGLSLBuffer);
    glDeleteBuffers(1, &mParametersGLSLBuffer);

    Clear();
}

static void SetKeyboardMouseInfos(EvaluationInfo& evaluationInfo, const EvaluationStage& evaluationStage)
{
    evaluationInfo.mouse[0] = evaluationStage.mRx;
    evaluationInfo.mouse[1] = evaluationStage.mRy;
    evaluationInfo.mouse[2] = evaluationStage.mLButDown ? 1.f : 0.f;
    evaluationInfo.mouse[3] = evaluationStage.mRButDown ? 1.f : 0.f;

    evaluationInfo.keyModifier[0] = evaluationStage.mbCtrl ? 1 : 0;
    evaluationInfo.keyModifier[1] = evaluationStage.mbAlt ? 1 : 0;
    evaluationInfo.keyModifier[2] = evaluationStage.mbShift ? 1 : 0;
    evaluationInfo.keyModifier[3] = 0;
}

void EvaluationContext::Clear()
{
    for (auto tgt : mStageTarget)
    {
        if (!tgt)
        {
            continue;
        }
        tgt->Destroy();
    }
    mStageTarget.clear();
    for (auto& buffer : mComputeBuffers)
    {
        glDeleteBuffers(1, &buffer.mBuffer);
    }
    mComputeBuffers.clear();
    mDirtyFlags.clear();
    mbProcessing.clear();
    mProgress.clear();
}

unsigned int EvaluationContext::GetEvaluationTexture(size_t target)
{
    if (target >= mStageTarget.size())
        return 0;
    if (!mStageTarget[target])
        return 0;
    return mStageTarget[target]->mGLTexID;
}

unsigned int bladesVertexArray;
unsigned int bladesVertexSize = 2 * sizeof(float);

unsigned int UploadIndices(const unsigned short* indices, unsigned int indexCount)
{
    unsigned int indexArray;
    glGenBuffers(1, &indexArray);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexArray);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned short), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return indexArray;
}

void UploadVertices(const void* vertices, unsigned int vertexArraySize)
{
    glGenBuffers(1, &bladesVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, bladesVertexArray);
    glBufferData(GL_ARRAY_BUFFER, vertexArraySize, vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static const int tess = 10;
static unsigned int bladeIA = -1;
void drawBlades(int indexCount, int instanceCount, int elementCount)
{
    // instances
    for (int i = 0; i < elementCount; i++)
        glVertexAttribDivisor(1 + i, 1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bladeIA);

    glDrawElementsInstanced(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_SHORT, (void*)0, instanceCount);

    for (int i = 0; i < elementCount; i++)
        glVertexAttribDivisor(1 + i, 0);


    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void EvaluationContext::BindTextures(const EvaluationStage& evaluationStage,
                                     unsigned int program,
                                     size_t nodeIndex,
                                     std::shared_ptr<RenderTarget> reusableTarget)
{
    const Input& input = evaluationStage.mInput;
    for (int inputIndex = 0; inputIndex < 8; inputIndex++)
    {
        glActiveTexture(GL_TEXTURE0 + inputIndex);
        int targetIndex = input.mOverrideInputs[inputIndex];
        if (targetIndex < 0)
            targetIndex = input.mInputs[inputIndex];
        if (targetIndex < 0)
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        else
        {
            unsigned int parameter = glGetUniformLocation(program, sampler2DName[inputIndex]);
            if (parameter == 0xFFFFFFFF)
                parameter = glGetUniformLocation(program, samplerCubeName[inputIndex]);
            if (parameter == 0xFFFFFFFF)
            {
                glBindTexture(GL_TEXTURE_2D, 0);
                continue;
            }
            glUniform1i(parameter, inputIndex);

            std::shared_ptr<RenderTarget> tgt;
            if (inputIndex == 0 && reusableTarget)
            {
                tgt = reusableTarget;
            }
            else
            {
                tgt = mStageTarget[targetIndex];
            }

            if (tgt)
            {
                const InputSampler& inputSampler = mEvaluationStages.mInputSamplers[nodeIndex][inputIndex];
                if (tgt->mImage->mNumFaces == 1)
                {
                    glBindTexture(GL_TEXTURE_2D, tgt->mGLTexID);
                    TexParam(filter[inputSampler.mFilterMin],
                             filter[inputSampler.mFilterMag],
                             wrap[inputSampler.mWrapU],
                             wrap[inputSampler.mWrapV],
                             GL_TEXTURE_2D);
                }
                else
                {
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
                }
            }
        }
    }
}

int EvaluationContext::GetBindedComputeBuffer(const EvaluationStage& evaluationStage) const
{
    const Input& input = evaluationStage.mInput;
    for (int inputIndex = 0; inputIndex < 8; inputIndex++)
    {
        int targetIndex = input.mInputs[inputIndex];
        if (targetIndex != -1 && targetIndex < mComputeBuffers.size() && mComputeBuffers[targetIndex].mBuffer &&
            mActive[targetIndex])
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

    // allocate buffer
    unsigned int feedbackVertexArray = 0;
    ComputeBuffer* destinationBuffer = NULL;
    ComputeBuffer* sourceBuffer = NULL;
    ComputeBuffer tempBuffer;
    int computeBufferIndex = GetBindedComputeBuffer(evaluationStage);
    if (computeBufferIndex != -1)
    {
        if (mComputeBuffers.size() <= nodeIndex || (!mComputeBuffers[nodeIndex].mBuffer))
        {
            // only allocate if needed
            AllocateComputeBuffer(int(nodeIndex),
                                  mComputeBuffers[computeBufferIndex].mElementCount,
                                  mComputeBuffers[computeBufferIndex].mElementSize);
        }
        sourceBuffer = &mComputeBuffers[computeBufferIndex];
    }
    else
    {
        //
        if (nodeIndex < mComputeBuffers.size())
        {
            Swap(mComputeBuffers[nodeIndex], tempBuffer);

            AllocateComputeBuffer(int(nodeIndex), tempBuffer.mElementCount, tempBuffer.mElementSize);
            sourceBuffer = &tempBuffer;
        }
    }

    if (mComputeBuffers.size() <= nodeIndex)
        return; // no compute buffer destination, no source either -> non connected node -> early exit

    /// build source VAO
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


    destinationBuffer = &mComputeBuffers[nodeIndex];

    // compute buffer
    if (destinationBuffer->mElementCount)
    {
        const Parameters& parameters = mEvaluationStages.mParameters[nodeIndex];
        glUseProgram(program);

        glBindBuffer(GL_UNIFORM_BUFFER, mEvaluationStateGLSLBuffer);
        evaluationInfo.mVertexSpace = evaluationStage.mVertexSpace;
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
    }
    if (feedbackVertexArray)
        glDeleteVertexArrays(1, &feedbackVertexArray);

    if (tempBuffer.mBuffer)
    {
        glDeleteBuffers(1, &tempBuffer.mBuffer);
    }
}

void EvaluationContext::EvaluateGLSL(const EvaluationStage& evaluationStage,
                                     size_t nodeIndex,
                                     EvaluationInfo& evaluationInfo)
{
    const Input& input = evaluationStage.mInput;

    auto tgt = mStageTarget[nodeIndex];

    const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mType);
    const unsigned int program = evaluator.mGLSLProgram;
    const int blendOps[] = {evaluationStage.mBlendingSrc, evaluationStage.mBlendingDst};
    unsigned int blend[] = {GL_ONE, GL_ZERO};

    if (!program)
    {
        glUseProgram(gDefaultShader.mNodeErrorShader);
        // mFSQuad.Render();
        evaluationStage.mGScene->Draw(this, evaluationInfo);
        return;
    }
    for (int i = 0; i < 2; i++)
    {
        if (blendOps[i] < BLEND_LAST)
            blend[i] = GLBlends[blendOps[i]];
    }

    // parameters
    glBindBuffer(GL_UNIFORM_BUFFER, mParametersGLSLBuffer);

    const Parameters& parameters = mEvaluationStages.mParameters[nodeIndex];
    glBufferData(
        GL_UNIFORM_BUFFER, parameters.size(), parameters.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, mParametersGLSLBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    /*
    if (!stage.mParametersBuffer)
    {
        glGenBuffers(1, &stage.mParametersBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, stage.mParametersBuffer);

        glBufferData(GL_UNIFORM_BUFFER, stage.mParameters.size(), stage.mParameters.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, stage.mParametersBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
    else
    {
        glBindBuffer(GL_UNIFORM_BUFFER, stage.mParametersBuffer);
        glBufferData(GL_UNIFORM_BUFFER, stage.mParameters.size(), stage.mParameters.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
    */
    glEnable(GL_BLEND);
    glBlendFunc(blend[0], blend[1]);

    glUseProgram(program);

    Camera* camera = mEvaluationStages.GetCameraParameter(nodeIndex);
    if (camera)
    {
        camera->ComputeViewProjectionMatrix(evaluationInfo.viewProjection, evaluationInfo.viewInverse);
    }

    int passCount = mEvaluationStages.GetIntParameter(nodeIndex, "passCount", 1);
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
                    tgt->BindCubeFace(face, mip, tgt->mImage->mWidth);

                memcpy(evaluationInfo.viewRot, rotMatrices[face], sizeof(float) * 16);
                memcpy(evaluationInfo.inputIndices, input.mInputs, sizeof(input.mInputs));
                float sizeDiv = float(mip + 1);
                evaluationInfo.viewport[0] = float(tgt->mImage->mWidth) / sizeDiv;
                evaluationInfo.viewport[1] = float(tgt->mImage->mHeight) / sizeDiv;
                evaluationInfo.passNumber = passNumber;
                evaluationInfo.mipmapNumber = mip;
                evaluationInfo.mipmapCount = mipmapCount;

                glBindBuffer(GL_UNIFORM_BUFFER, mEvaluationStateGLSLBuffer);
                evaluationInfo.mVertexSpace = evaluationStage.mVertexSpace;
                glBufferData(GL_UNIFORM_BUFFER, sizeof(EvaluationInfo), &evaluationInfo, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_UNIFORM_BUFFER, 0);


                glBindBufferBase(GL_UNIFORM_BUFFER, 1, mParametersGLSLBuffer);
                glBindBufferBase(GL_UNIFORM_BUFFER, 2, mEvaluationStateGLSLBuffer);

                BindTextures(evaluationStage, program, nodeIndex, passNumber ? transientTarget : std::shared_ptr<RenderTarget>());

                glDisable(GL_CULL_FACE);
                // glCullFace(GL_BACK);
                glClearDepth(1.f);
                if (evaluationStage.mbClearBuffer)
                {
                    glClear(GL_COLOR_BUFFER_BIT | (evaluationStage.mbDepthBuffer ? GL_DEPTH_BUFFER_BIT : 0));
                }
                if (evaluationStage.mbDepthBuffer)
                {
                    glDepthFunc(GL_LEQUAL);
                    glEnable(GL_DEPTH_TEST);
                }
                //
                if (evaluationStage.mTypename == "FurDisplay")
                {
                    /*const ComputeBuffer* buffer*/ int sourceBuffer = GetBindedComputeBuffer(evaluationStage);
                    if (sourceBuffer != -1)
                    {
                        const ComputeBuffer* buffer = &mComputeBuffers[sourceBuffer];
                        unsigned int vao;
                        glGenVertexArrays(1, &vao);
                        glBindVertexArray(vao);

                        // blade vertices
                        glBindBuffer(GL_ARRAY_BUFFER, bladesVertexArray);
                        glVertexAttribPointer(0 /*SemUV*/, 2, GL_FLOAT, GL_FALSE, bladesVertexSize, 0);
                        glEnableVertexAttribArray(0 /*SemUV*/);
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
    glDisable(GL_BLEND);
}

void EvaluationContext::EvaluateC(const EvaluationStage& evaluationStage, size_t nodeIndex, EvaluationInfo& evaluationInfo)
{
    try // todo: find a better solution than a try catch
    {
        const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mType);
        if (evaluator.mCFunction)
        {
            int res = evaluator.mCFunction((unsigned char*)mEvaluationStages.mParameters[nodeIndex].data(), &evaluationInfo, this);
            if (res == EVAL_DIRTY)
            {
                mStillDirty.push_back(uint32_t(nodeIndex));
            }
        }
    }
    catch (...)
    {
    }
}

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


void EvaluationContext::AllocRenderTargetsForEditingPreview()
{
    // alloc targets
    mStageTarget.resize(mEvaluationStages.GetStagesCount(), NULL);
    for (size_t i = 0; i < mEvaluationStages.GetStagesCount(); i++)
    {
        if (!mStageTarget[i])
        {
            mStageTarget[i] = std::make_shared<RenderTarget>();
        }
    }
}

void EvaluationContext::AllocRenderTargetsForBaking(const std::vector<size_t>& nodesToEvaluate)
{
    if (!mStageTarget.empty())
        return;

    // auto evaluationOrderList = mEvaluationStages.GetForwardEvaluationOrder();
    size_t stageCount = mEvaluationStages.GetStagesCount();
    mStageTarget.resize(stageCount, NULL);
    std::vector<std::shared_ptr<RenderTarget>> freeRenderTargets;
    std::vector<int> useCount(stageCount, 0);
    for (size_t i = 0; i < stageCount; i++)
    {
        useCount[i] = mEvaluationStages.GetEvaluationStage(i).mUseCountByOthers;
    }

    for (auto index : nodesToEvaluate)
    {
        const EvaluationStage& evaluation = mEvaluationStages.GetEvaluationStage(index);
        if (!evaluation.mUseCountByOthers)
            continue;

        if (freeRenderTargets.empty())
        {
            mStageTarget[index] = std::make_shared<RenderTarget>();
        }
        else
        {
            mStageTarget[index] = freeRenderTargets.back();
            freeRenderTargets.pop_back();
        }

        const Input& input = evaluation.mInput;
        for (auto targetIndex : input.mInputs)
        {
            if (targetIndex == -1)
                continue;

            useCount[targetIndex]--;
            if (!useCount[targetIndex])
            {
                freeRenderTargets.push_back(mStageTarget[targetIndex]);
            }
        }
    }
}
void EvaluationContext::PreRun()
{
    mDirtyFlags.resize(mEvaluationStages.GetStagesCount(), 0);
    mbProcessing.resize(mEvaluationStages.GetStagesCount(), 0);
    mProgress.resize(mEvaluationStages.GetStagesCount(), 0.f);
    mActive.resize(mEvaluationStages.GetStagesCount(), false);
}

void EvaluationContext::RunNode(size_t nodeIndex)
{
    auto& currentStage = mEvaluationStages.GetEvaluationStage(nodeIndex);
    const Input& input = currentStage.mInput;

    // check processing
    for (auto& inp : input.mInputs)
    {
        if (inp < 0)
            continue;
        if (mbProcessing[inp])
        {
            mbProcessing[nodeIndex] = 1;
            return;
        }
    }

    mbProcessing[nodeIndex] = 0;

    mEvaluationInfo.targetIndex = int(nodeIndex);
    mEvaluationInfo.mFrame = mCurrentTime;
    mEvaluationInfo.mDirtyFlag = mDirtyFlags[nodeIndex];
    memcpy(mEvaluationInfo.inputIndices, input.mInputs, sizeof(mEvaluationInfo.inputIndices));
    SetKeyboardMouseInfos(mEvaluationInfo, currentStage);

    if (currentStage.gEvaluationMask & EvaluationC)
        EvaluateC(currentStage, nodeIndex, mEvaluationInfo);

    if (currentStage.gEvaluationMask & EvaluationPython)
        EvaluatePython(currentStage, nodeIndex, mEvaluationInfo);

    if (currentStage.gEvaluationMask & EvaluationGLSLCompute)
    {
        EvaluateGLSLCompute(currentStage, nodeIndex, mEvaluationInfo);
    }

    if (currentStage.gEvaluationMask & EvaluationGLSL)
    {
        if (!mStageTarget[nodeIndex]->mGLTexID)
            mStageTarget[nodeIndex]->InitBuffer(mDefaultWidth, mDefaultHeight, currentStage.mbDepthBuffer);

        EvaluateGLSL(currentStage, nodeIndex, mEvaluationInfo);
    }
    mDirtyFlags[nodeIndex] = 0;
}

bool EvaluationContext::RunNodeList(const std::vector<size_t>& nodesToEvaluate)
{
    GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);

    // run C nodes
    bool anyNodeIsProcessing = false;
    for (size_t nodeIndex : nodesToEvaluate)
    {
        mActive[nodeIndex] = mCurrentTime >= mEvaluationStages.mStages[nodeIndex].mStartFrame &&
                             mCurrentTime <= mEvaluationStages.mStages[nodeIndex].mEndFrame;
        if (!mActive[nodeIndex])
            continue;
        RunNode(nodeIndex);
        anyNodeIsProcessing |= mbProcessing[nodeIndex] != 0;
    }
    // set dirty nodes that tell so
    for (auto index : mStillDirty)
        SetTargetDirty(index, Dirty::Input);
    mStillDirty.clear();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    return anyNodeIsProcessing;
}

void EvaluationContext::RunSingle(size_t nodeIndex, EvaluationInfo& evaluationInfo)
{
    GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);

    PreRun();

    mEvaluationInfo = evaluationInfo;

    RunNode(nodeIndex);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}

void EvaluationContext::RecurseBackward(size_t target, std::vector<size_t>& usedNodes)
{
    const EvaluationStage& evaluation = mEvaluationStages.GetEvaluationStage(target);
    const Input& input = evaluation.mInput;

    for (size_t inputIndex = 0; inputIndex < 8; inputIndex++)
    {
        int targetIndex = input.mInputs[inputIndex];
        if (targetIndex == -1)
            continue;
        RecurseBackward(targetIndex, usedNodes);
    }

    if (std::find(usedNodes.begin(), usedNodes.end(), target) == usedNodes.end())
        usedNodes.push_back(target);
}

void EvaluationContext::RunDirty()
{
    PreRun();
    memset(&mEvaluationInfo, 0, sizeof(EvaluationInfo));
    auto evaluationOrderList = mEvaluationStages.GetForwardEvaluationOrder();
    std::vector<size_t> nodesToEvaluate;
    for (size_t index = 0; index < evaluationOrderList.size(); index++)
    {
        size_t currentNodeIndex = evaluationOrderList[index];
        if (currentNodeIndex < mDirtyFlags.size() && mDirtyFlags[currentNodeIndex]) // TODOUNDO
            nodesToEvaluate.push_back(currentNodeIndex);
    }
    AllocRenderTargetsForEditingPreview();
    RunNodeList(nodesToEvaluate);
}

void EvaluationContext::DirtyAll()
{
    // tag all as dirty
    mDirtyFlags.resize(mEvaluationStages.GetStagesCount(), 0);
    for (auto& dirty : mDirtyFlags)
    {
        dirty = Dirty::All;
    }
}

void EvaluationContext::RunAll()
{
    PreRun();
    DirtyAll();
    // get list of nodes to run
    memset(&mEvaluationInfo, 0, sizeof(EvaluationInfo));
    auto evaluationOrderList = mEvaluationStages.GetForwardEvaluationOrder();
    AllocRenderTargetsForEditingPreview();
    RunNodeList(evaluationOrderList);
}

bool EvaluationContext::RunBackward(size_t nodeIndex)
{
    PreRun();
    memset(&mEvaluationInfo, 0, sizeof(EvaluationInfo));
    mEvaluationInfo.forcedDirty = true;
    std::vector<size_t> nodesToEvaluate;
    RecurseBackward(nodeIndex, nodesToEvaluate);
    AllocRenderTargetsForBaking(nodesToEvaluate);
    return RunNodeList(nodesToEvaluate);
}

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

void EvaluationContext::SetTargetDirty(size_t target, DirtyFlag dirtyFlag, bool onlyChild)
{
    mDirtyFlags.resize(mEvaluationStages.GetStagesCount(), 0);
    auto evaluationOrderList = mEvaluationStages.GetForwardEvaluationOrder();
    mDirtyFlags[target] = dirtyFlag;
    for (size_t i = 0; i < evaluationOrderList.size(); i++)
    {
        size_t currentNodeIndex = evaluationOrderList[i];
        if (currentNodeIndex != target)
            continue;

        for (i++; i < evaluationOrderList.size(); i++)
        {
            currentNodeIndex = evaluationOrderList[i];
            if (currentNodeIndex >= mDirtyFlags.size() || mDirtyFlags[currentNodeIndex]) // TODOUNDO
                continue;

            auto& currentEvaluation = mEvaluationStages.GetEvaluationStage(currentNodeIndex);
            for (auto inp : currentEvaluation.mInput.mInputs)
            {
                if (inp >= 0 && mDirtyFlags[inp])
                {
                    mDirtyFlags[currentNodeIndex] = Dirty::Input;
                    break;
                }
            }
        }
    }
    if (onlyChild)
    {
        mDirtyFlags[target] = false;
    }
}

void EvaluationContext::AllocateComputeBuffer(int target, int elementCount, int elementSize)
{
    if (mComputeBuffers.size() <= target)
        mComputeBuffers.resize(target + 1);
    ComputeBuffer& buffer = mComputeBuffers[target];
    buffer.mElementCount = elementCount;
    buffer.mElementSize = elementSize;
    if (!buffer.mBuffer)
        glGenBuffers(1, &buffer.mBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, buffer.mBuffer);
    glBufferData(GL_ARRAY_BUFFER, elementSize * elementCount, NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

const EvaluationContext::ComputeBuffer* EvaluationContext::GetComputeBuffer(size_t index) const
{
    ComputeBuffer res;
    if (mComputeBuffers.size() <= index)
        return nullptr;
    return &mComputeBuffers[index];
}

void EvaluationContext::StageSetProcessing(size_t target, int processing)
{
    mbProcessing.resize(mEvaluationStages.GetStagesCount(), 0);
    if (target >= mbProcessing.size())
    {
        return;
    }
    if (mbProcessing[target] != processing)
    {
        mProgress.resize(mEvaluationStages.GetStagesCount(), 0.f);
        mProgress[target] = 0.f;
    }
    mbProcessing[target] = processing;
}

void EvaluationContext::StageSetProgress(size_t target, float progress)
{
    mProgress.resize(mEvaluationStages.GetStagesCount(), 0.f);
    if (target >= mProgress.size())
    {
        return;
    }
    mProgress[target] = progress;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Builder::Builder() : mbRunning(true)
{
    mThread = std::thread([&]() { BuildEntries(); });
}

Builder::~Builder()
{
    mbRunning = false;
    mThread.join();
}

void Builder::Add(const char* graphName, EvaluationStages& stages)
{
    mMutex.lock();
    mEntries.push_back({graphName, 0.f, stages});
    mMutex.unlock();
}

EvaluationStages BuildEvaluationFromMaterial(Material& material)
{
    EvaluationStages evaluationStages;
    for (size_t i = 0; i < material.mMaterialNodes.size(); i++)
    {
        MaterialNode& node = material.mMaterialNodes[i];
        evaluationStages.AddSingleEvaluation(node.mType);
        auto& lastNode = evaluationStages.mStages.back();
        evaluationStages.SetParameters(i, node.mParameters);
        evaluationStages.SetSamplers(i, node.mInputSamplers);
    }
    for (size_t i = 0; i < material.mMaterialConnections.size(); i++)
    {
        MaterialConnection& materialConnection = material.mMaterialConnections[i];
        evaluationStages.AddEvaluationInput(
            materialConnection.mOutputNode, materialConnection.mInputSlot, materialConnection.mInputNode);
    }

    evaluationStages.SetAnimTrack(material.mAnimTrack);
    evaluationStages.mFrameMin = material.mFrameMin;
    evaluationStages.mFrameMax = material.mFrameMax;
    evaluationStages.SetParameterPins(material.mPinnedParameters);
    evaluationStages.SetIOPins(material.mPinnedIO);
    return evaluationStages;
}

void Builder::Add(Material* material)
{
    try
    {
        Add(material->mName.c_str(), BuildEvaluationFromMaterial(*material));
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
        Sleep(100);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace DrawUICallbacks
{
    void DrawUIProgress(EvaluationContext* context, size_t nodeIndex)
    {
        glUseProgram(gDefaultShader.mProgressShader);
        glUniform1f(glGetUniformLocation(gDefaultShader.mProgressShader, "time"),
                    float(double(SDL_GetTicks()) / 1000.0));
        context->mFSQuad.Render();
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
        glUseProgram(gDefaultShader.mDisplayCubemapShader);
        int tgt = glGetUniformLocation(gDefaultShader.mDisplayCubemapShader, "samplerCubemap");
        glUniform1i(tgt, 0);
        glActiveTexture(GL_TEXTURE0);
        TexParam(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_CUBE_MAP);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, context->GetEvaluationTexture(nodeIndex));
        context->mFSQuad.Render();
    }
} // namespace DrawUICallbacks
