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
#include "EvaluationStages.h"
#include "EvaluationContext.h"
#include "Evaluators.h"
#include <vector>
#include <algorithm>
#include <map>


EvaluationStages::EvaluationStages()
{
}

void EvaluationStages::AddSingleEvaluation(size_t nodeType)
{
    EvaluationStage evaluation;
    //#ifdef _DEBUG needed for fur
    evaluation.mTypename = gMetaNodes[nodeType].mName;
    //#endif
#if USE_FFMPEG
    evaluation.mDecoder = NULL;
#endif
    evaluation.mUseCountByOthers = 0;
    evaluation.mType = uint16_t(nodeType);
    evaluation.mLocalTime = 0;
    static std::shared_ptr<Scene> defaultScene;
    if (!defaultScene)
    {
        defaultScene = std::make_shared<Scene>();

    }
    evaluation.mScene = nullptr;
    evaluation.mGScene = defaultScene;
    evaluation.renderer = nullptr;
    evaluation.mRuntimeUniqueId = GetRuntimeId();
    const size_t inputCount = gMetaNodes[nodeType].mInputs.size();
    
    mParameters.push_back(Parameters());
    mStages.push_back(evaluation);
    mInputSamplers.push_back(Samplers(inputCount));
    mMultiplexInputs.push_back(MultiplexInput());
}

void EvaluationStages::DelSingleEvaluation(size_t nodeIndex)
{
    mStages.erase(mStages.begin() + nodeIndex);
    mParameters.erase(mParameters.begin() + nodeIndex);
    mInputSamplers.erase(mInputSamplers.begin() + nodeIndex);
    mMultiplexInputs.erase(mMultiplexInputs.begin() + nodeIndex);
}

void EvaluationStages::SetEvaluationParameters(size_t nodeIndex, const Parameters& parameters)
{
#if USE_FFMPEG
    EvaluationStage& stage = mStages[nodeIndex];
    mParameters[nodeIndex] = parameters;

    if (stage.mDecoder)
        stage.mDecoder = NULL;
#endif
}

void EvaluationStages::SetSamplers(size_t nodeIndex, const std::vector<InputSampler>& inputSamplers)
{
    mInputSamplers[nodeIndex] = inputSamplers;
}

void EvaluationStages::ClearInputs()
{
    mInputs.resize(mStages.size());
    mUseCountByOthers.resize(mStages.size());
    for (size_t i = 0;i<mStages.size();i++)
    {
        mInputs[i] = Input();
        mUseCountByOthers[i] = 0;
    }
}

void EvaluationStages::SetEvaluationInput(size_t nodeIndex, int slot, int source)
{
    mInputs[nodeIndex].mInputs[slot] = source;
    mUseCountByOthers[source]++;
}

void EvaluationStages::SetEvaluationOrder(const std::vector<size_t>& nodeOrderList)
{
    mEvaluationOrderList = nodeOrderList;
}

void EvaluationStages::Clear()
{
    mEvaluationOrderList.clear();

    mStages.clear();
    mAnimTrack.clear();
    mInputSamplers.clear();
    mParameters.clear();
}

size_t EvaluationStages::GetEvaluationImageDuration(size_t target)
{
    #if USE_FFMPEG
    auto& stage = mStages[target];
    if (!stage.mDecoder)
        return 1;
    if (stage.mDecoder->mFrameCount > 2000)
    {
        int a = 1;
    }
    return stage.mDecoder->mFrameCount;
    #else
    return 1;
    #endif
}

void EvaluationStages::SetStageLocalTime(EvaluationContext* evaluationContext,
                                         size_t target,
                                         int localTime,
                                         bool updateDecoder)
{
    auto& stage = mStages[target];
    int newLocalTime = ImMin(localTime, int(GetEvaluationImageDuration(target)));
    #if USE_FFMPEG
    if (stage.mDecoder && updateDecoder && stage.mLocalTime != newLocalTime)
    {
        stage.mLocalTime = newLocalTime;
        Image image = stage.DecodeImage();
        EvaluationAPI::SetEvaluationImage(evaluationContext, int(target), &image);
        Image::Free(&image);
    }
    else
    #endif
    {
        stage.mLocalTime = newLocalTime;
    }
}
#if USE_FFMPEG
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
#endif






#if USE_FFMPEG
Image EvaluationStage::DecodeImage()
{
    return Image::DecodeImage(mDecoder.get(), mLocalTime);
}
#endif
void EvaluationStages::ApplyAnimationForNode(EvaluationContext* context, size_t nodeIndex, int frame)
{
    bool animatedNodes = false;
    EvaluationStage& stage = mStages[nodeIndex];
    Parameters parameters = mParameters[nodeIndex];
    for (auto& animTrack : mAnimTrack)
    {
        if (animTrack.mNodeIndex == nodeIndex)
        {
            size_t parameterOffset = GetParameterOffset(uint32_t(stage.mType), animTrack.mParamIndex);
            animTrack.mAnimation->GetValue(frame, &parameters[parameterOffset]);

            animatedNodes = true;
        }
    }
    if (animatedNodes)
    {
        SetEvaluationParameters(nodeIndex, parameters);
        context->SetTargetDirty(nodeIndex, Dirty::Parameter);
    }
}

void EvaluationStages::ApplyAnimation(EvaluationContext* context, int frame)
{
    std::vector<bool> animatedNodes;
    animatedNodes.resize(mStages.size(), false);
    for (auto& animTrack : mAnimTrack)
    {
        EvaluationStage& stage = mStages[animTrack.mNodeIndex];

        animatedNodes[animTrack.mNodeIndex] = true;
        size_t parameterOffset = GetParameterOffset(uint32_t(stage.mType), animTrack.mParamIndex);
        animTrack.mAnimation->GetValue(frame, &mParameters[animTrack.mNodeIndex][parameterOffset]);
    }
    for (size_t i = 0; i < animatedNodes.size(); i++)
    {
        if (!animatedNodes[i])
            continue;
        SetEvaluationParameters(i, mParameters[i]);
        context->SetTargetDirty(i, Dirty::Parameter);
    }
}




void EvaluationStages::SetTime(EvaluationContext* evaluationContext, int time, bool updateDecoder)
{
    for (size_t i = 0; i < mStages.size(); i++)
    {
        const auto& stage = mStages[i];
        SetStageLocalTime(evaluationContext,
                          i,
                          ImClamp(time - stage.mStartFrame, 0, stage.mEndFrame - stage.mStartFrame),
                          updateDecoder);
        // bool enabled = time >= node.mStartFrame && time <= node.mEndFrame;
        evaluationContext->SetTargetDirty(i, Dirty::Time);
    }
}

void EvaluationStages::BuildEvaluationFromMaterial(Material& material)
{
    mStages.clear();
    mInputs.clear();
    mInputSamplers.clear();
    mParameters.clear();

    auto nodeCount = material.mMaterialNodes.size();
    mStages.reserve(nodeCount);
    mInputs.reserve(nodeCount);
    mInputSamplers.reserve(nodeCount);
    mParameters.reserve(nodeCount);

    for (size_t i = 0; i < nodeCount; i++)
    {
        MaterialNode& node = material.mMaterialNodes[i];
        AddSingleEvaluation(node.mType);
        auto& lastNode = mStages.back();
        mParameters[i] = node.mParameters;
        //mInputSamplers[i] = node.mInputSamplers;
    }
    for (size_t i = 0; i < material.mMaterialConnections.size(); i++)
    {
        MaterialConnection& materialConnection = material.mMaterialConnections[i];
        //SetEvaluationInput(
        //    materialConnection.mOutputNodeIndex, materialConnection.mInputSlotIndex, materialConnection.mInputNodeIndex);
    }

    //SetAnimTrack(material.mAnimTrack);
}

