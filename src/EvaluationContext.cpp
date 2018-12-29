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

#include <GL/gl3w.h>    // Initialize with gl3wInit()
#include <memory>
#include "EvaluationContext.h"
#include "Evaluators.h"
#include "NodesDelegate.h"

EvaluationContext *gCurrentContext = NULL;

static const unsigned int wrap[] = { GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, GL_MIRRORED_REPEAT };
static const unsigned int filter[] = { GL_LINEAR, GL_NEAREST };
static const char* sampler2DName[] = { "Sampler0", "Sampler1", "Sampler2", "Sampler3", "Sampler4", "Sampler5", "Sampler6", "Sampler7" };
static const char* samplerCubeName[] = { "CubeSampler0", "CubeSampler1", "CubeSampler2", "CubeSampler3", "CubeSampler4", "CubeSampler5", "CubeSampler6", "CubeSampler7" };

static const unsigned int GLBlends[] = { GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR,GL_SRC_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA, GL_SRC_ALPHA_SATURATE };

static const float rotMatrices[6][16] = {
    // toward +x
    { 0,0,-1,0,
    0,1,0,0,
    1,0,0,0,
    0,0,0,1
    },

    // -x
    { 0,0,1,0,
    0,1,0,0,
    -1,0,0,0,
    0,0,0,1 },

    //+y
    { 1,0,0,0,
    0,0,1,0,
    0,-1,0,0,
    0,0,0,1 },

    // -y
    { 1,0,0,0,
    0,0,-1,0,
    0,1,0,0,
    0,0,0,1 },

    // +z
    { 1,0,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,1 },

    //-z
    { -1,0,0,0,
    0,1,0,0,
    0,0,-1,0,
    0,0,0,1 }
};


EvaluationContext::EvaluationContext(Evaluation& evaluation, bool synchronousEvaluation, int defaultWidth, int defaultHeight) 
    : gEvaluation(evaluation)
    , mbSynchronousEvaluation(synchronousEvaluation)
    , mDefaultWidth(defaultWidth)
    , mDefaultHeight(defaultHeight)
{

}

EvaluationContext::~EvaluationContext()
{
    for (auto& stream : mWriteStreams)
    {
        stream.second->Finish();
        delete stream.second;
    }
    mWriteStreams.clear();
}

static void SetMouseInfos(EvaluationInfo &evaluationInfo, const EvaluationStage &evaluationStage)
{
    evaluationInfo.mouse[0] = evaluationStage.mRx;
    evaluationInfo.mouse[1] = evaluationStage.mRy;
    evaluationInfo.mouse[2] = evaluationStage.mLButDown ? 1.f : 0.f;
    evaluationInfo.mouse[3] = evaluationStage.mRButDown ? 1.f : 0.f;
}

void EvaluationContext::Clear()
{
    mStageTarget.clear();
    for (auto& buffer : mComputeBuffers)
        glDeleteBuffers(1, &buffer.mBuffer);
    mComputeBuffers.clear();
    mbDirty.clear();
    mbProcessing.clear();
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

unsigned int UploadIndices(const unsigned short *indices, unsigned int indexCount)
{
    unsigned int indexArray;
    glGenBuffers(1, &indexArray);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexArray);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned short), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return indexArray;
}

void UploadVertices(const void *vertices, unsigned int vertexArraySize)
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
    for (int i = 0;i<elementCount;i++)
        glVertexAttribDivisor(1 + i, 1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bladeIA);

    glDrawElementsInstanced(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_SHORT, (void*)0, instanceCount);

    for (int i = 0; i < elementCount; i++)
        glVertexAttribDivisor(1 + i, 0);


    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void EvaluationContext::BindTextures(const EvaluationStage& evaluationStage, unsigned int program)
{
    const Input& input = evaluationStage.mInput;
    for (int inputIndex = 0; inputIndex < 8; inputIndex++)
    {
        glActiveTexture(GL_TEXTURE0 + inputIndex);
        int targetIndex = input.mInputs[inputIndex];
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

            auto tgt = mStageTarget[targetIndex];
            if (tgt)
            {
                const InputSampler& inputSampler = evaluationStage.mInputSamplers[inputIndex];
                if (tgt->mImage.mNumFaces == 1)
                {
                    glBindTexture(GL_TEXTURE_2D, tgt->mGLTexID);
                    TexParam(filter[inputSampler.mFilterMin], filter[inputSampler.mFilterMag], wrap[inputSampler.mWrapU], wrap[inputSampler.mWrapV], GL_TEXTURE_2D);
                }
                else
                {
                    glBindTexture(GL_TEXTURE_CUBE_MAP, tgt->mGLTexID);
                    TexParam(filter[inputSampler.mFilterMin], filter[inputSampler.mFilterMag], wrap[inputSampler.mWrapU], wrap[inputSampler.mWrapV], GL_TEXTURE_CUBE_MAP);
                }
            }
        }
    }
}

int  EvaluationContext::GetBindedComputeBuffer(const EvaluationStage& evaluationStage) const
{
    const Input& input = evaluationStage.mInput;
    for (int inputIndex = 0; inputIndex < 8; inputIndex++)
    {
        int targetIndex = input.mInputs[inputIndex];
        if (targetIndex != -1 && targetIndex < mComputeBuffers.size() && mComputeBuffers[targetIndex].mBuffer)
            return targetIndex;
    }
    return -1;
}

void EvaluationContext::EvaluateGLSLCompute(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo)
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

    const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mNodeType);
    const unsigned int program = evaluator.mGLSLProgram;

    // allocate buffer
    unsigned int feedbackVertexArray = 0;
    ComputeBuffer* destinationBuffer = NULL;
    int computeBufferIndex = GetBindedComputeBuffer(evaluationStage);
    if (computeBufferIndex != -1)
    {
        AllocateComputeBuffer(int(index), mComputeBuffers[computeBufferIndex].mElementCount, mComputeBuffers[computeBufferIndex].mElementSize);
 
        /// build source VAO
        glGenVertexArrays(1, &feedbackVertexArray);
        glBindVertexArray(feedbackVertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, mComputeBuffers[computeBufferIndex].mBuffer);
        const int transformElementCount = mComputeBuffers[computeBufferIndex].mElementSize / (4 * sizeof(float));
        for (int i = 0; i < transformElementCount;i++)
        {
            glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4 * transformElementCount, (void*)(4 * sizeof(float) * i));
            glEnableVertexAttribArray(i);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
    }
    if (mComputeBuffers.size() <= index)
        return; // no compute buffer destination, no source either -> non connected node -> early exit
    destinationBuffer = &mComputeBuffers[index];

    // compute buffer
    glUseProgram(program);

    glBindBuffer(GL_UNIFORM_BUFFER, gEvaluators.gEvaluationStateGLSLBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(EvaluationInfo), &evaluationInfo, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    glBindBufferBase(GL_UNIFORM_BUFFER, 1, evaluationStage.mParametersBuffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, gEvaluators.gEvaluationStateGLSLBuffer);


    BindTextures(evaluationStage, program);
    glEnable(GL_RASTERIZER_DISCARD);
    glBindVertexArray(feedbackVertexArray);
    glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, destinationBuffer->mBuffer, 0, destinationBuffer->mElementCount * destinationBuffer->mElementSize);

    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, destinationBuffer->mElementCount);
    glEndTransformFeedback();

    glDisable(GL_RASTERIZER_DISCARD);
    glBindVertexArray(0);
    glUseProgram(0);

    if (feedbackVertexArray)
        glDeleteVertexArrays(1, &feedbackVertexArray);
}

void EvaluationContext::EvaluateGLSL(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo)
{
    const Input& input = evaluationStage.mInput;

    auto tgt = mStageTarget[index];
    if (!evaluationInfo.uiPass)
    {
        if (tgt->mImage.mNumFaces == 6)
            tgt->BindAsCubeTarget();
        else
            tgt->BindAsTarget();
    }
    /*if (evaluationStage.mNodeTypename == "TerrainPreview")
    {
        DebugBreak();
    }
    */
    const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mNodeType);
    const unsigned int program = evaluator.mGLSLProgram;
    const int blendOps[] = { evaluationStage.mBlendingSrc, evaluationStage.mBlendingDst };
    unsigned int blend[] = { GL_ONE, GL_ZERO };

    if (!program)
    {
        glUseProgram(gEvaluation.mNodeErrorShader);
        gFSQuad.Render();
        return;
    }
    for (int i = 0; i < 2; i++)
    {
        if (blendOps[i] < BLEND_LAST)
            blend[i] = GLBlends[blendOps[i]];
    }

    glEnable(GL_BLEND);
    glBlendFunc(blend[0], blend[1]);

    glUseProgram(program);

    Camera *camera = gNodeDelegate.GetCameraParameter(index);
    if (camera)
    {
        camera->ComputeViewProjectionMatrix(evaluationInfo.viewProjection, evaluationInfo.viewInverse);
    }

    size_t faceCount = evaluationInfo.uiPass ? 1 : tgt->mImage.mNumFaces;
    for (size_t face = 0; face < faceCount; face++)
    {
        if (tgt->mImage.mNumFaces == 6)
            tgt->BindCubeFace(face);

        memcpy(evaluationInfo.viewRot, rotMatrices[face], sizeof(float) * 16);
        memcpy(evaluationInfo.inputIndices, input.mInputs, sizeof(input.mInputs));
            
        glBindBuffer(GL_UNIFORM_BUFFER, gEvaluators.gEvaluationStateGLSLBuffer);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(EvaluationInfo), &evaluationInfo, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);


        glBindBufferBase(GL_UNIFORM_BUFFER, 1, evaluationStage.mParametersBuffer);
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, gEvaluators.gEvaluationStateGLSLBuffer);

        BindTextures(evaluationStage, program);

        //
#if 0
        if (evaluationStage.mNodeTypename == "FurDisplay")
        {
            glClearDepth(1.f);
            glDepthFunc(GL_LEQUAL);
            glEnable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT|(evaluationStage.mbDepthBuffer?GL_DEPTH_BUFFER_BIT:0));
            
            /*const ComputeBuffer* buffer*/int sourceBuffer = GetBindedComputeBuffer(evaluationStage);
            if (sourceBuffer != -1)
            {
                const ComputeBuffer* buffer = &mComputeBuffers[sourceBuffer];
                unsigned int vao;
                glGenVertexArrays(1, &vao);
                glBindVertexArray(vao);

                // blade vertices
                glBindBuffer(GL_ARRAY_BUFFER, bladesVertexArray);
                glVertexAttribPointer(0/*SemUV*/, 2, GL_FLOAT, GL_FALSE, bladesVertexSize, 0);
                glEnableVertexAttribArray(0/*SemUV*/);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                // blade instances
                const size_t transformElementCount = buffer->mElementSize / (sizeof(float) * 4);
                glBindBuffer(GL_ARRAY_BUFFER, buffer->mBuffer);
                for (unsigned int vp = 0; vp < transformElementCount; vp++)
                {
                    glVertexAttribPointer(1 + vp, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4 * transformElementCount, (void*)(4 * sizeof(float) * vp));
                    glEnableVertexAttribArray(1 + vp);
                }

                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glBindVertexArray(vao);
                drawBlades(tess * 2, buffer->mElementCount, transformElementCount);
                glBindVertexArray(0);
                glDeleteVertexArrays(1, &vao);
            }
        }
        else
#endif
        {
            gFSQuad.Render();
        }
    }
    glDisable(GL_BLEND);
}

void EvaluationContext::EvaluateC(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo)
{
    try // todo: find a better solution than a try catch
    {
        const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mNodeType);
        if (evaluator.mCFunction)
            evaluator.mCFunction((unsigned char*)evaluationStage.mParameters.data(), &evaluationInfo);
    }
    catch (...)
    {

    }
}

void EvaluationContext::EvaluatePython(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo)
{
    try // todo: find a better solution than a try catch
    {
        const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mNodeType);
        evaluator.RunPython();
    }
    catch (...)
    {

    }
}


void EvaluationContext::AllocRenderTargetsForEditingPreview()
{
    // alloc targets
    mStageTarget.resize(gEvaluation.GetStagesCount(), NULL);
    for (size_t i = 0; i < gEvaluation.GetStagesCount(); i++)
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

    //auto evaluationOrderList = gEvaluation.GetForwardEvaluationOrder();
    size_t stageCount = gEvaluation.GetStagesCount();
    mStageTarget.resize(stageCount, NULL);
    std::vector<std::shared_ptr<RenderTarget> > freeRenderTargets;
    std::vector<int> useCount(stageCount, 0);
    for (size_t i = 0; i < stageCount; i++)
    {
        useCount[i] = gEvaluation.GetEvaluationStage(i).mUseCountByOthers;
    }

    for (auto index : nodesToEvaluate)
    {
        const EvaluationStage& evaluation = gEvaluation.GetEvaluationStage(index);
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
    mbDirty.resize(gEvaluation.GetStagesCount(), false);
    mbProcessing.resize(gEvaluation.GetStagesCount(), false);
}

void EvaluationContext::RunNode(size_t nodeIndex)
{
    auto& currentStage = gEvaluation.GetEvaluationStage(nodeIndex);
    const Input& input = currentStage.mInput;

    // check processing 
    for (auto& inp : input.mInputs)
    {
        if (inp < 0)
            continue;
        if (mbProcessing[inp])
        {
            mbProcessing[nodeIndex] = true;
            return;
        }
    }

    mbProcessing[nodeIndex] = false;

    mEvaluationInfo.targetIndex = int(nodeIndex);
    mEvaluationInfo.mFrame = gEvaluationTime;
    memcpy(mEvaluationInfo.inputIndices, input.mInputs, sizeof(mEvaluationInfo.inputIndices));
    SetMouseInfos(mEvaluationInfo, currentStage);

    if (currentStage.gEvaluationMask&EvaluationC)
        EvaluateC(currentStage, nodeIndex, mEvaluationInfo);

    if (currentStage.gEvaluationMask&EvaluationPython)
        EvaluatePython(currentStage, nodeIndex, mEvaluationInfo);

    if (currentStage.gEvaluationMask&EvaluationGLSLCompute)
    {
        EvaluateGLSLCompute(currentStage, nodeIndex, mEvaluationInfo);
    }

    if (currentStage.gEvaluationMask&EvaluationGLSL)
    {
        if (!mStageTarget[nodeIndex]->mGLTexID)
            mStageTarget[nodeIndex]->InitBuffer(mDefaultWidth, mDefaultHeight, currentStage.mbDepthBuffer);

        EvaluateGLSL(currentStage, nodeIndex, mEvaluationInfo);
    }
    mbDirty[nodeIndex] = false;
}

void EvaluationContext::RunNodeList(const std::vector<size_t>& nodesToEvaluate)
{
    // run C nodes
    for (size_t nodeIndex : nodesToEvaluate)
    {
        RunNode(nodeIndex);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}

void EvaluationContext::RunSingle(size_t nodeIndex, EvaluationInfo& evaluationInfo)
{
    PreRun();

    mEvaluationInfo = evaluationInfo;

    RunNode(nodeIndex);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}

void EvaluationContext::RecurseBackward(size_t target, std::vector<size_t>& usedNodes)
{
    const EvaluationStage& evaluation = gEvaluation.GetEvaluationStage(target);
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
    auto evaluationOrderList = gEvaluation.GetForwardEvaluationOrder();
    std::vector<size_t> nodesToEvaluate;
    for (size_t index = 0; index < evaluationOrderList.size(); index++)
    {
        size_t currentNodeIndex = evaluationOrderList[index];
        if (currentNodeIndex < mbDirty.size() && mbDirty[currentNodeIndex]) // TODOUNDO
            nodesToEvaluate.push_back(currentNodeIndex);
    }
    AllocRenderTargetsForEditingPreview();
    RunNodeList(nodesToEvaluate);
}

void EvaluationContext::RunAll()
{
    PreRun();
    // get list of nodes to run
    memset(&mEvaluationInfo, 0, sizeof(EvaluationInfo));
    auto evaluationOrderList = gEvaluation.GetForwardEvaluationOrder();
    AllocRenderTargetsForEditingPreview();
    RunNodeList(evaluationOrderList);
}

void EvaluationContext::RunBackward(size_t nodeIndex)
{
    PreRun();
    memset(&mEvaluationInfo, 0, sizeof(EvaluationInfo));
    mEvaluationInfo.forcedDirty = true;
    std::vector<size_t> nodesToEvaluate;
    RecurseBackward(nodeIndex, nodesToEvaluate);
    AllocRenderTargetsForBaking(nodesToEvaluate);
    RunNodeList(nodesToEvaluate);
}

FFMPEGCodec::Encoder *EvaluationContext::GetEncoder(const std::string &filename, int width, int height)
{
    FFMPEGCodec::Encoder *encoder;
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

void EvaluationContext::SetTargetDirty(size_t target, bool onlyChild)
{
    mbDirty.resize(gEvaluation.GetStagesCount(), false);
    auto evaluationOrderList = gEvaluation.GetForwardEvaluationOrder();
    mbDirty[target] = true;
    for (size_t i = 0; i < evaluationOrderList.size(); i++)
    {
        size_t currentNodeIndex = evaluationOrderList[i];
        if (currentNodeIndex != target)
            continue;

        for (i++; i < evaluationOrderList.size(); i++)
        {
            currentNodeIndex = evaluationOrderList[i];
            if (currentNodeIndex >= mbDirty.size() || mbDirty[currentNodeIndex]) // TODOUNDO
                continue;

            auto& currentEvaluation = gEvaluation.GetEvaluationStage(currentNodeIndex);
            for (auto inp : currentEvaluation.mInput.mInputs)
            {
                if (inp >= 0 && mbDirty[inp])
                {
                    mbDirty[currentNodeIndex] = true;
                    break;
                }
            }
        }
    }
    if (onlyChild)
        mbDirty[target] = false;
}

void EvaluationContext::UserAddStage()
{
    URAdd<std::shared_ptr<RenderTarget>> undoRedoAddRenderTarget(int(mStageTarget.size()), []() {return &gCurrentContext->mStageTarget; });
    URAdd<bool> undoRedoAddDirty(int(mbDirty.size()), []() {return &gCurrentContext->mbDirty; });
    URAdd<bool> undoRedoAddProcessing(int(mbProcessing.size()), []() {return &gCurrentContext->mbProcessing; });

    mStageTarget.push_back(std::make_shared<RenderTarget>());
    mbDirty.push_back(true);
    mbProcessing.push_back(false);
}

void EvaluationContext::UserDeleteStage(size_t index)
{
    URDel<std::shared_ptr<RenderTarget>> undoRedoDelRenderTarget(int(index), []() {return &gCurrentContext->mStageTarget; });
    URDel<bool> undoRedoDelDirty(int(index), []() {return &gCurrentContext->mbDirty; });
    URDel<bool> undoRedoDelProcessing(int(index), []() {return &gCurrentContext->mbProcessing; });

    mStageTarget.erase(mStageTarget.begin() + index);
    mbDirty.erase(mbDirty.begin() + index);
    mbProcessing.erase(mbProcessing.begin() + index);
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
