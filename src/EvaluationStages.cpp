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

void EvaluationStages::AddEvaluation(NodeIndex nodeIndex, size_t nodeType)
{
    EvaluationStage evaluation;
    //#ifdef _DEBUG needed for fur
    evaluation.mTypename = gMetaNodes[nodeType].mName;

    evaluation.mType = uint16_t(nodeType);
    evaluation.mLocalTime = 0;
    evaluation.mScene = nullptr;
    evaluation.mGScene = Scene::BuildDefaultScene();
    evaluation.renderer = nullptr;
    const size_t inputCount = gMetaNodes[nodeType].mInputs.size();
    
    mParameterBlocks.insert(mParameterBlocks.begin() + nodeIndex, ParameterBlock(nodeType));
    mStages.insert(mStages.begin() + nodeIndex, evaluation);
    mInputSamplers.insert(mInputSamplers.begin() + nodeIndex, InputSamplers());
    mInputSamplers[nodeIndex].resize(gMetaNodes[nodeType].mInputs.size());
    mInputs.insert(mInputs.begin() + nodeIndex, Input());
	mMultiplex.insert(mMultiplex.begin() + nodeIndex, MultiplexArray());
}

void EvaluationStages::DelEvaluation(NodeIndex nodeIndex)
{
    mStages.erase(mStages.begin() + nodeIndex);
    mInputs.erase(mInputs.begin() + nodeIndex);
    mInputSamplers.erase(mInputSamplers.begin() + nodeIndex);
    mParameterBlocks.erase(mParameterBlocks.begin() + nodeIndex);
	mMultiplex.erase(mMultiplex.begin() + nodeIndex);
}

void EvaluationStages::SetParameterBlock(NodeIndex nodeIndex, const ParameterBlock& parameters)
{
    EvaluationStage& stage = mStages[nodeIndex];
    mParameterBlocks[nodeIndex] = parameters;
}

void EvaluationStages::Clear()
{
    mStages.clear();
    mInputs.clear();
    mInputSamplers.clear();
    mParameterBlocks.clear();
    mOrderList.clear();
}

void EvaluationStages::SetMaterialUniqueId(RuntimeId runtimeId)
{
	mMaterialUniqueId = runtimeId;
}

size_t EvaluationStages::GetEvaluationImageDuration(NodeIndex nodeIndex)
{
    return 1;
}

void EvaluationStages::SetStageLocalTime(EvaluationContext* evaluationContext,
	NodeIndex nodeIndex,
                                         int localTime,
                                         bool updateDecoder)
{
    auto& stage = mStages[nodeIndex];
    int newLocalTime = ImMin(localTime, int(GetEvaluationImageDuration(nodeIndex)));
    {
        stage.mLocalTime = newLocalTime;
    }
}

void EvaluationStages::BuildEvaluationFromMaterial(Material& material)
{
    mStages.clear();
    mInputs.clear();
    mInputSamplers.clear();
    mParameterBlocks.clear();

    auto nodeCount = material.mMaterialNodes.size();
    mStages.reserve(nodeCount);
    mInputs.reserve(nodeCount);
    mInputSamplers.reserve(nodeCount);
    mParameterBlocks.reserve(nodeCount);

    for (size_t i = 0; i < nodeCount; i++)
    {
        MaterialNode& node = material.mMaterialNodes[i];
        AddEvaluation(i, node.mNodeType);
        auto& lastNode = mStages.back();
        mParameterBlocks[i] = ParameterBlock(node.mNodeType, node.mParameters);
        //mInputSamplers[i] = node.mInputSamplers;
    }
    for (size_t i = 0; i < material.mMaterialConnections.size(); i++)
    {
        MaterialConnection& materialConnection = material.mMaterialConnections[i];
        //SetEvaluationInput(
        //    materialConnection.mOutputNodeIndex, materialConnection.mInputSlotIndex, materialConnection.mInputNodeIndex);
    }
    ComputeEvaluationOrder();
	mMaterialUniqueId = material.mRuntimeUniqueId;
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
        if (!input.IsValid())
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
