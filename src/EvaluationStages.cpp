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

void EvaluationStages::AddEvaluation(size_t nodeIndex, size_t nodeType)
{
    EvaluationStage evaluation;
    //#ifdef _DEBUG needed for fur
    evaluation.mTypename = gMetaNodes[nodeType].mName;
    //#endif
#if USE_FFMPEG
    evaluation.mDecoder = NULL;
#endif
    evaluation.mType = uint16_t(nodeType);
    evaluation.mLocalTime = 0;
    evaluation.mScene = nullptr;
    evaluation.mGScene = Scene::BuildDefaultScene();
    evaluation.renderer = nullptr;
    evaluation.mRuntimeUniqueId = GetRuntimeId();
    const size_t inputCount = gMetaNodes[nodeType].mInputs.size();
    
    mParameters.insert(mParameters.begin() + nodeIndex, Parameters());
    mStages.insert(mStages.begin() + nodeIndex, evaluation);
    mInputSamplers.insert(mInputSamplers.begin() + nodeIndex, InputSamplers());
    mInputSamplers[nodeIndex].resize(gMetaNodes[nodeType].mInputs.size());
    mInputs.insert(mInputs.begin() + nodeIndex, Input());
}

void EvaluationStages::DelEvaluation(size_t nodeIndex)
{
    mStages.erase(mStages.begin() + nodeIndex);
    mInputs.erase(mInputs.begin() + nodeIndex);
    mInputSamplers.erase(mInputSamplers.begin() + nodeIndex);
    mParameters.erase(mParameters.begin() + nodeIndex);
}

void EvaluationStages::SetParameters(size_t nodeIndex, const Parameters& parameters)
{
    EvaluationStage& stage = mStages[nodeIndex];
    mParameters[nodeIndex] = parameters;
#if USE_FFMPEG
    if (stage.mDecoder)
        stage.mDecoder = NULL;
#endif
}

void EvaluationStages::Clear()
{
    mStages.clear();
    mInputs.clear();
    mInputSamplers.clear();
    mParameters.clear();
    mAnimTrack.clear();
    mOrderList.clear();
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
        SetParameters(nodeIndex, parameters);
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
        SetParameters(i, mParameters[i]);
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
        AddEvaluation(i, node.mType);
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
    ComputeEvaluationOrder();
    //SetAnimTrack(material.mAnimTrack);
}

size_t EvaluationStages::PickBestNode(const std::vector<EvaluationStages::NodeOrder>& orders) const
{
    for (auto& order : orders)
    {
        if (order.mNodePriority == 0)
            return order.mNodeIndex;
    }
    // issue!
    assert(0);
    return -1;
}

void EvaluationStages::RecurseSetPriority(std::vector<EvaluationStages::NodeOrder>& orders,
    size_t currentIndex,
    size_t currentPriority,
    size_t& undeterminedNodeCount) const
{
    if (!orders[currentIndex].mNodePriority)
        undeterminedNodeCount--;

    orders[currentIndex].mNodePriority = std::max(orders[currentIndex].mNodePriority, currentPriority + 1);
    for (auto input : mInputs[currentIndex].mInputs)
    {
        if (input == -1)
        {
            continue;
        }

        RecurseSetPriority(orders, input, currentPriority + 1, undeterminedNodeCount);
    }
}

std::vector<EvaluationStages::NodeOrder> EvaluationStages::ComputeEvaluationOrders()
{
    size_t nodeCount = mStages.size();

    std::vector<NodeOrder> orders(nodeCount);
    for (size_t i = 0; i < nodeCount; i++)
    {
        orders[i].mNodeIndex = i;
        orders[i].mNodePriority = 0;
    }
    size_t undeterminedNodeCount = nodeCount;
    while (undeterminedNodeCount)
    {
        size_t currentIndex = PickBestNode(orders);
        RecurseSetPriority(orders, currentIndex, orders[currentIndex].mNodePriority, undeterminedNodeCount);
    };
    //
    return orders;
}

void EvaluationStages::ComputeEvaluationOrder()
{
    mOrderList.clear();

    auto orders = ComputeEvaluationOrders();
    std::sort(orders.begin(), orders.end());
    mOrderList.resize(orders.size());
    for (size_t i = 0; i < orders.size(); i++)
    {
        mOrderList[i] = orders[i].mNodeIndex;
    }
}
