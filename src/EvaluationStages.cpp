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

#include "EvaluationStages.h"
#include "EvaluationContext.h"
#include "Evaluators.h"
#include <vector>
#include <algorithm>
#include <map>
#include <GL/gl3w.h>    // Initialize with gl3wInit()

EvaluationStages::EvaluationStages() : mFrameMin(0), mFrameMax(1)
{
    
}

void EvaluationStages::AddSingleEvaluation(size_t nodeType)
{
    EvaluationStage evaluation;
#ifdef _DEBUG
    evaluation.mTypename          = gMetaNodes[nodeType].mName;;
#endif
    evaluation.mDecoder               = NULL;
    evaluation.mUseCountByOthers      = 0;
    evaluation.mType              = nodeType;
    evaluation.mParametersBuffer      = 0;
    evaluation.mBlendingSrc           = ONE;
    evaluation.mBlendingDst           = ZERO;
    evaluation.mLocalTime             = 0;
    evaluation.gEvaluationMask        = gEvaluators.GetMask(nodeType);
    evaluation.mbDepthBuffer          = false;
    evaluation.scene = nullptr;
    evaluation.renderer = nullptr;
    mStages.push_back(evaluation);
}

void EvaluationStages::StageIsAdded(int index)
{
    for (size_t i = 0;i< mStages.size();i++)
    {
        if (i == index)
            continue;
        auto& evaluation = mStages[i];
        for (auto& inp : evaluation.mInput.mInputs)
        {
            if (inp >= index)
                inp++;
        }
    }
}

void EvaluationStages::StageIsDeleted(int index)
{
    EvaluationStage& ev = mStages[index];
    ev.Clear();

    // shift all connections
    for (auto& evaluation : mStages)
    {
        for (auto& inp : evaluation.mInput.mInputs)
        {
            if (inp >= index)
                inp--;
        }
    }
}

void EvaluationStages::UserAddEvaluation(size_t nodeType)
{
    URAdd<EvaluationStage> undoRedoAddStage(int(mStages.size()), [&]() {return &mStages; },
        [&](int index) {StageIsDeleted(index); }, [&](int index) {StageIsAdded(index); });

    AddSingleEvaluation(nodeType);
}

void EvaluationStages::UserDeleteEvaluation(size_t target)
{
    URDel<EvaluationStage> undoRedoDelStage(int(target), [&]() {return &mStages; },
        [&](int index) {StageIsDeleted(index); }, [&](int index) {StageIsAdded(index); });

    StageIsDeleted(int(target));
    mStages.erase(mStages.begin() + target);
}

void EvaluationStages::SetEvaluationParameters(size_t target, const std::vector<unsigned char> &parameters)
{
    EvaluationStage& stage = mStages[target];
    stage.mParameters = parameters;

    if (stage.gEvaluationMask&EvaluationGLSL)
        BindGLSLParameters(stage);
    if (stage.mDecoder)
        stage.mDecoder = NULL;
}

void EvaluationStages::SetEvaluationSampler(size_t target, const std::vector<InputSampler>& inputSamplers)
{
    mStages[target].mInputSamplers = inputSamplers;
}

void EvaluationStages::AddEvaluationInput(size_t target, int slot, int source)
{
    if (mStages[target].mInput.mInputs[slot] == source)
        return;
    mStages[target].mInput.mInputs[slot] = source;
    mStages[source].mUseCountByOthers++;
}

void EvaluationStages::DelEvaluationInput(size_t target, int slot)
{
    mStages[mStages[target].mInput.mInputs[slot]].mUseCountByOthers--;
    mStages[target].mInput.mInputs[slot] = -1;
}

void EvaluationStages::SetEvaluationOrder(const std::vector<size_t> nodeOrderList)
{
    mEvaluationOrderList = nodeOrderList;
}

void EvaluationStages::Clear()
{
    for (auto& ev : mStages)
        ev.Clear();

    mStages.clear();
    mEvaluationOrderList.clear();
    mAnimTrack.clear();
}

void EvaluationStages::SetMouse(int target, float rx, float ry, bool lButDown, bool rButDown)
{
    for (auto& ev : mStages)
    {
        ev.mRx = -9999.f;
        ev.mRy = -9999.f;
        ev.mLButDown = false;
        ev.mRButDown = false;
    }
    auto& ev = mStages[target];
    ev.mRx = rx;
    ev.mRy = 1.f - ry; // inverted for UI
    ev.mLButDown = lButDown;
    ev.mRButDown = rButDown;
}

size_t EvaluationStages::GetEvaluationImageDuration(size_t target)
{
    auto& stage = mStages[target];
    if (!stage.mDecoder)
        return 1;
    if (stage.mDecoder->mFrameCount > 2000)
    {
        int a = 1;
    }
    return stage.mDecoder->mFrameCount;
}

void EvaluationStages::SetStageLocalTime(size_t target, int localTime, bool updateDecoder)
{
    auto& stage = mStages[target];
    int newLocalTime = ImMin(localTime, int(GetEvaluationImageDuration(target)));
    if (stage.mDecoder && updateDecoder && stage.mLocalTime != newLocalTime)
    {
        stage.mLocalTime = newLocalTime;
        Image image = stage.DecodeImage();
        //SetEvaluationImage(this, int(target), &image); TODO
        Image::Free(&image);
    }
    else
    {
        stage.mLocalTime = newLocalTime;
    }
}
/* TODO
int EvaluationStages::Evaluate(int target, int width, int height, Image *image)
{
    EvaluationContext *previousContext = gCurrentContext;
    EvaluationContext context(*this, true, width, height);
    gCurrentContext = &context;
    while (context.RunBackward(target))
    {
        // processing... maybe good on next run
    }
    GetEvaluationImage(target, image);
    gCurrentContext = previousContext;
    return EVAL_OK;
}
*/
FFMPEGCodec::Decoder* EvaluationStages::FindDecoder(const std::string& filename)
{
    for (auto& evaluation : mStages)
    {
        if (evaluation.mDecoder && evaluation.mDecoder->GetFilename() == filename)
            return evaluation.mDecoder.get();
    }
    auto decoder = new FFMPEGCodec::Decoder;
    decoder->Open(filename);
    return decoder;
}


Camera *EvaluationStages::GetCameraParameter(size_t index)
{
    if (index >= mStages.size())
        return NULL;
    EvaluationStage& stage = mStages[index];
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[stage.mType];
    const size_t paramsSize = ComputeNodeParametersSize(stage.mType);
    stage.mParameters.resize(paramsSize);
    unsigned char *paramBuffer = stage.mParameters.data();
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (param.mType == Con_Camera)
        {
            Camera *cam = (Camera*)paramBuffer;
            return cam;
        }
        paramBuffer += GetParameterTypeSize(param.mType);
    }

    return NULL;
}
// TODO : create parameter struct with templated accessors
int EvaluationStages::GetIntParameter(size_t index, const char *parameterName, int defaultValue)
{
    if (index >= mStages.size())
        return NULL;
    EvaluationStage& stage = mStages[index];
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[stage.mType];
    const size_t paramsSize = ComputeNodeParametersSize(stage.mType);
    stage.mParameters.resize(paramsSize);
    unsigned char *paramBuffer = stage.mParameters.data();
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (param.mType == Con_Int)
        {
            if (!strcmp(param.mName.c_str(), parameterName))
            {
                int *value = (int*)paramBuffer;
                return *value;
            }
        }
        paramBuffer += GetParameterTypeSize(param.mType);
    }
    return defaultValue;
}


Image EvaluationStage::DecodeImage()
{
    return Image::DecodeImage(mDecoder.get(), mLocalTime);
}


void EvaluationStages::BindGLSLParameters(EvaluationStage& stage)
{
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
}

void EvaluationStage::Clear()
{
    if (gEvaluationMask&EvaluationGLSL)
        glDeleteBuffers(1, &mParametersBuffer);
    mParametersBuffer = 0;
}
