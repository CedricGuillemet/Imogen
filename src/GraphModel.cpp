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

#include <assert.h>
#include <algorithm>
#include "GraphModel.h"
#include "UndoRedo.h"

extern UndoRedoHandler gUndoRedoHandler;

GraphModel::GraphModel() : mbTransaction(false), mSelectedNodeIndex(-1), mUndoRedo(nullptr), mStartFrame(0), mEndFrame(1)
{
}

GraphModel::~GraphModel()
{
    assert(!mbTransaction);
}

void GraphModel::Clear()
{
    assert(!mbTransaction);

    mNodes.clear();
    mLinks.clear();
    mRugs.clear();
    gUndoRedoHandler.Clear();
    mSelectedNodeIndex = -1;
    mDirtyList.clear();
}

void GraphModel::BeginTransaction(bool undoable)
{
    assert(!mbTransaction);

    mbTransaction = true;
    if (undoable)
    {
        mUndoRedo = new URDummy;
    }
}

bool GraphModel::InTransaction() const
{
    return mbTransaction;
}

void GraphModel::EndTransaction()
{
    assert(mbTransaction);
    mbTransaction = false;
    if (mUndoRedo)
    {
        // undo/redo but nothing has been updated
        assert(mUndoRedo->HasSubUndoRedo());
    }
    delete mUndoRedo;
    mUndoRedo = nullptr;
    
    UpdateEvaluation();
    
}

void GraphModel::UpdateEvaluation()
{
    // set inputs
    mInputs.resize(mNodes.size());
    for (size_t i = 0; i < mNodes.size(); i++)
    {
        mInputs[i] = Input();
    }
    for(const auto& link: mLinks)
    {
        auto source = link.mInputNodeIndex;
        mInputs[link.mOutputNodeIndex].mInputs[link.mInputSlotIndex] = source;
    }

    // inputs are fresh, can compute evaluation order
    ComputeEvaluationOrder();
}

void GraphModel::Undo()
{
    assert(!mbTransaction);
    gUndoRedoHandler.Undo();
    UpdateEvaluation();
}

void GraphModel::Redo()
{
    assert(!mbTransaction);
    gUndoRedoHandler.Redo();
    UpdateEvaluation();
}

void GraphModel::MoveSelectedNodes(const ImVec2 delta)
{
    assert(mbTransaction);
    for (size_t i = 0 ; i < mNodes.size(); i++)
    {
        auto& node = mNodes[i];
        if (!node.mbSelected)
        {
            continue;
        }
        auto ur = mUndoRedo
                ? std::make_unique<URChange<Node>>(int(i), [&](int index) { return &mNodes[index]; }, [](int) {})
                : nullptr;
        node.mPos += delta;
    }
}

void GraphModel::SetNodePosition(size_t nodeIndex, const ImVec2 position)
{
    assert(mbTransaction);
    auto ur = mUndoRedo
        ? std::make_unique<URChange<Node>>(int(nodeIndex), [&](int index) { return &mNodes[index]; }, [](int) {})
        : nullptr;
    mNodes[nodeIndex].mPos = position;
}

// called when a node is added by user, or with undo/redo
void GraphModel::AddNodeHelper(int nodeIndex)
{
    SetDirty(nodeIndex, Dirty::AddedNode);
}

void GraphModel::DeleteNodeHelper(int nodeIndex)
{
    for (int id = 0; id < mLinks.size(); id++)
    {
        if (mLinks[id].mInputNodeIndex > nodeIndex)
        {
            mLinks[id].mInputNodeIndex--;
        }
        if (mLinks[id].mOutputNodeIndex > nodeIndex)
        {
            mLinks[id].mOutputNodeIndex--;
        }
    }
    RemoveAnimation(nodeIndex);
    SetDirty(nodeIndex, Dirty::DeletedNode);
}

void GraphModel::RemoveAnimation(size_t nodeIndex)
{
    if (mAnimTrack.empty())
        return;
    std::vector<int> tracks;
    for (int i = 0; i < int(mAnimTrack.size()); i++)
    {
        const AnimTrack& animTrack = mAnimTrack[i];
        if (animTrack.mNodeIndex == nodeIndex)
            tracks.push_back(i);
    }
    if (tracks.empty())
        return;

    for (int i = 0; i < int(tracks.size()); i++)
    {
        int index = tracks[i] - i;
        auto urAnimTrack = mUndoRedo ? std::make_unique<URDel<AnimTrack>>(index, [&] { return &mAnimTrack; }):nullptr;
        mAnimTrack.erase(mAnimTrack.begin() + index);
    }
}
size_t GraphModel::AddNode(size_t type, ImVec2 position)
{
    assert(mbTransaction);

    size_t nodeIndex = mNodes.size();

    auto delNode = [&](int index) { DeleteNodeHelper(index); };
    auto addNode = [&](int index) { AddNodeHelper(index); };

    auto urNode = mUndoRedo ? std::make_unique<URAdd<Node>>(int(nodeIndex), [&]() { return &mNodes; }, delNode, addNode) : nullptr;
    
    mNodes.push_back(Node(int(type), position, mStartFrame, mEndFrame));
    InitDefaultParameters(type, mNodes[nodeIndex].mParameters);
    AddNodeHelper(int(nodeIndex));
    return nodeIndex;
}

void GraphModel::AddLinkInternal(size_t inputNodeIndex, size_t inputSlotIndex, size_t outputNodeIndex, size_t outputSlotIndex)
{
    size_t linkIndex = mLinks.size();
    Link link;
    link.mInputNodeIndex = int(inputNodeIndex);
    link.mInputSlotIndex = int(inputSlotIndex);
    link.mOutputNodeIndex = int(outputNodeIndex);
    link.mOutputSlotIndex = int(outputSlotIndex);
    mLinks.push_back(link);
}

void GraphModel::AddLink(size_t inputNodeIndex, size_t inputSlotIndex, size_t outputNodeIndex, size_t outputSlotIndex)
{
    assert(mbTransaction);

    if (inputNodeIndex >= mNodes.size() || outputNodeIndex >= mNodes.size())
    {
        // Log("Error : Link node index doesn't correspond to an existing node.");
        return;
    }

    size_t linkIndex = mLinks.size();

    auto ur = mUndoRedo ? std::make_unique<URAdd<Link>>(int(linkIndex), [&]() { return &mLinks; })
                  : nullptr;

    AddLinkInternal(inputNodeIndex, inputSlotIndex, outputNodeIndex, outputSlotIndex);
}

void GraphModel::DelLinkInternal(size_t linkIndex)
{
    mLinks.erase(mLinks.begin() + linkIndex);
}

void GraphModel::DelLink(size_t linkIndex)
{
    assert(mbTransaction);

    auto ur = mUndoRedo ? std::make_unique<URDel<Link>>(int(linkIndex), [&]() { return &mLinks; })
                  : nullptr;
    DelLinkInternal(linkIndex);
}

void GraphModel::AddRug(const Rug& rug)
{
    assert(mbTransaction);

    auto ur = mUndoRedo
                  ? std::make_unique<URAdd<Rug>>(int(mRugs.size()), [&]() { return &mRugs; })
                  : nullptr;

    mRugs.push_back(rug);
}

void GraphModel::DelRug(size_t rugIndex)
{
    assert(mbTransaction);

    auto ur = mUndoRedo ? std::make_unique<URDel<Rug>>(int(rugIndex), [&]() { return &mRugs; })
                        : nullptr;

    mRugs.erase(mRugs.begin() + rugIndex);
}

void GraphModel::SetRug(size_t rugIndex, const Rug& rug)
{
    assert(mbTransaction);

    auto ur = mUndoRedo
                  ? std::make_unique<URChange<Rug>>(int(rugIndex), [&](int index) { return &mRugs[index]; }, [](int) {})
                  : nullptr;

    mRugs[rugIndex] = rug;
}

bool GraphModel::HasSelectedNodes() const
{
    bool hasSelectedNode = false;
    for (int selection = int(mNodes.size()) - 1; selection >= 0; selection--)
    {
        if (mNodes[selection].mbSelected)
        {
            return true;
        }
    }
    return false;    
}

void GraphModel::DeleteSelectedNodes()
{
    assert(HasSelectedNodes());

    for (int selection = int(mNodes.size()) - 1; selection >= 0; selection--)
    {
        if (!mNodes[selection].mbSelected)
            continue;
        
        auto ur = mUndoRedo ? std::make_unique<URDel<Node>>(int(selection),
                                    [this]() { return &mNodes; },
                                    [this](int index) {
                                        mSelectedNodeIndex = -1;
                                        DeleteNodeHelper(index);
                                    },
                                    [this](int index) {
                                        // recompute link indices
                                        for (int id = 0; id < mLinks.size(); id++)
                                        {
                                            if (mLinks[id].mInputNodeIndex >= index)
                                            {
                                                mLinks[id].mInputNodeIndex++;
                                            }
                                            if (mLinks[id].mOutputNodeIndex >= index)
                                            {
                                                mLinks[id].mOutputNodeIndex++;
                                            }
                                        }
                                        mSelectedNodeIndex = -1;
                                        AddNodeHelper(index);
                                    }) : nullptr;

        for (int id = 0; id < mLinks.size(); id++)
        {
            if (mLinks[id].mInputNodeIndex == selection || mLinks[id].mOutputNodeIndex == selection)
            {
                DelLink(id);
            }
        }

        for (size_t i = 0; i < mLinks.size();)
        {
            auto& link = mLinks[i];
            if (link.mInputNodeIndex == selection || link.mOutputNodeIndex == selection)
            {
                auto ur = mUndoRedo ? std::make_unique<URDel<Link>>(
                    int(i),
                    [this]() { return &mLinks; },
                    [this](int index) {
                        DelLinkInternal(index);
                    },
                    [this](int index) {
                        Link& link = mLinks[index];
                        AddLinkInternal(link.mInputNodeIndex, link.mInputSlotIndex, link.mOutputNodeIndex, link.mOutputSlotIndex);
                    }) : nullptr;

                mLinks.erase(mLinks.begin() + i);
            }
            else
            {
                i++;
            }
        }

        mNodes.erase(mNodes.begin() + selection);
        DeleteNodeHelper(selection);
    }
}

bool GraphModel::IsIOUsed(size_t nodeIndex, int slotIndex, bool forOutput) const
{
    for (auto& link : mLinks)
    {
        if ((link.mInputNodeIndex == int(nodeIndex) && link.mInputSlotIndex == slotIndex && forOutput) ||
            (link.mOutputNodeIndex == int(nodeIndex) && link.mOutputSlotIndex == slotIndex && !forOutput))
        {
            return true;
        }
    }
    return false;
}

void GraphModel::SelectNode(size_t nodeIndex, bool selected)
{
    // assert(mbTransaction);
    mNodes[nodeIndex].mbSelected = selected;
}

ImVec2 GraphModel::GetNodePos(size_t nodeIndex) const
{
    return mNodes[nodeIndex].mPos;
}

void GraphModel::SetSamplers(size_t nodeIndex, const std::vector<InputSampler>& samplers)
{
    assert(mbTransaction);

    auto ur = mUndoRedo ? std::make_unique<URChange<Node>>(
                              int(nodeIndex),
                              [&](int index) { return &mNodes[index]; },
                              [&](int index) {
                                  // auto& stage = mEvaluationStages.mStages[index];
                                  // mEvaluationStages.SetSamplers(index, stage.mInputSamplers);
                                  // mEditingContext.SetTargetDirty(index, Dirty::Sampler);
                              })
                        : nullptr;
    mNodes[nodeIndex].mSamplers = samplers;
}

bool GraphModel::NodeHasUI(size_t nodeIndex) const
{
    return gMetaNodes[mNodes[nodeIndex].mType].mbHasUI;
}

void GraphModel::SetParameter(size_t nodeIndex, const std::string& parameterName, const std::string& parameterValue)
{
    assert(mbTransaction);
    uint32_t nodeType = uint32_t(mNodes[nodeIndex].mType);
    int parameterIndex = GetParameterIndex(nodeType, parameterName.c_str());
    if (parameterIndex == -1)
    {
        return;
    }
    ConTypes parameterType = GetParameterType(nodeType, parameterIndex);
    size_t paramOffset = GetParameterOffset(nodeType, parameterIndex);
    ParseStringToParameter(
        parameterValue, parameterType, &mNodes[nodeIndex].mParameters[paramOffset]);
    SetDirty(nodeIndex, Dirty::Parameter);
}

AnimTrack* GraphModel::GetAnimTrack(uint32_t nodeIndex, uint32_t parameterIndex)
{
    for (auto& animTrack : mAnimTrack)
    {
        if (animTrack.mNodeIndex == nodeIndex && animTrack.mParamIndex == parameterIndex)
            return &animTrack;
    }
    return NULL;
}

void GraphModel::MakeKey(int frame, uint32_t nodeIndex, uint32_t parameterIndex)
{
    assert(mbTransaction);
    if (nodeIndex == -1)
    {
        return;
    }
    // URDummy urDummy;

    AnimTrack* animTrack = GetAnimTrack(nodeIndex, parameterIndex);
    if (!animTrack)
    {
        // URAdd<AnimTrack> urAdd(int(mEvaluationStages.mAnimTrack.size()), [&] { return
        // &mEvaluationStages.mAnimTrack;
        // });
        uint32_t parameterType = gMetaNodes[mNodes[nodeIndex].mType].mParams[parameterIndex].mType;
        AnimTrack newTrack;
        newTrack.mNodeIndex = nodeIndex;
        newTrack.mParamIndex = parameterIndex;
        newTrack.mValueType = parameterType;
        newTrack.mAnimation = AllocateAnimation(parameterType);
        mAnimTrack.push_back(newTrack);
        animTrack = &mAnimTrack.back();
    }
    /*URChange<AnimTrack> urChange(int(animTrack - mEvaluationStages.mAnimTrack.data()),
                                 [&](int index) { return &mEvaluationStages.mAnimTrack[index]; });*/
    //EvaluationStage& stage = mEvaluationStages.mStages[nodeIndex];
    size_t parameterOffset = GetParameterOffset(uint32_t(mNodes[nodeIndex].mType), parameterIndex);
    animTrack->mAnimation->SetValue(frame, &mNodes[nodeIndex].mParameters[parameterOffset]);
}

void GraphModel::GetKeyedParameters(int frame, uint32_t nodeIndex, std::vector<bool>& keyed) const
{
}

void GraphModel::SetIOPin(size_t nodeIndex, size_t io, bool forOutput, bool pinned)
{
    assert(mbTransaction);
    auto ur = mUndoRedo ? std::make_unique<URChange<Node>>(
        int(nodeIndex),
        [&](int index) { return &mNodes[index]; })
        : nullptr;

    uint32_t mask = 0;
    if (forOutput)
    {
        mask = (1 << io) & 0xFF;
    }
    else
    {
        mask = (1 << (8 + io));
    }
    mNodes[nodeIndex].mPinnedIO &= ~mask;
    mNodes[nodeIndex].mPinnedIO += pinned ? mask : 0;

}

void GraphModel::SetAnimTrack(const std::vector<AnimTrack>& animTrack)
{
    //assert(mbTransaction);
    mAnimTrack = animTrack;
}

void GraphModel::SetMultiplexInputs(const std::vector<MultiplexInput>& multiplexInputs)
{
    assert(multiplexInputs.size() == mNodes.size());
    for (auto i = 0; i < mNodes.size() ; i++)
    {
        mNodes[i].mMultiplexInput = multiplexInputs[i];
    }
}

void GraphModel::SetParameterPin(size_t nodeIndex, size_t parameterIndex, bool pinned)
{
    assert(mbTransaction);
    auto ur = mUndoRedo ? std::make_unique<URChange<Node>>(
        int(nodeIndex),
        [&](int index) { return &mNodes[index]; })
        : nullptr;

    uint32_t mask = 1 << parameterIndex;
    mNodes[nodeIndex].mPinnedParameters &= ~mask;
    mNodes[nodeIndex].mPinnedParameters += pinned ? mask : 0;
}

void GraphModel::SetParameters(size_t nodeIndex, const std::vector<unsigned char>& parameters)
{
    assert(mbTransaction);

    auto ur = mUndoRedo ? std::make_unique<URChange<Node>>(
                              int(nodeIndex),
                              [&](int index) { return &mNodes[index]; },
                              [&](int index) {
                                  // auto& stage = mEvaluationStages.mStages[index];
                                  // mEvaluationStages.SetSamplers(index, stage.mInputSamplers);
                                  // mEditingContext.SetTargetDirty(index, Dirty::Sampler);
                              })
                        : nullptr;

    mNodes[nodeIndex].mParameters = parameters;
    SetDirty(nodeIndex, Dirty::Parameter);
}

void GraphModel::SetTimeSlot(size_t nodeIndex, int frameStart, int frameEnd)
{
    assert(mbTransaction);

    auto& node = mNodes[nodeIndex];
    node.mStartFrame = frameStart;
    node.mEndFrame = frameEnd;
}

void GraphModel::CopySelectedNodes()
{
    mNodesClipboard.clear();
    for (auto i = 0; i < mNodes.size(); i++)
    {
        if (!mNodes[i].mbSelected)
        {
            continue;
        }
        mNodesClipboard.push_back(mNodes[i]);
    }
}

void GraphModel::CutSelectedNodes()
{
    if (!HasSelectedNodes())
    {
        return;
    }
    CopySelectedNodes();
    BeginTransaction(true);
    DeleteSelectedNodes();
    EndTransaction();
}

bool GraphModel::IsClipboardEmpty() const
{
    return mNodesClipboard.empty();
}

void GraphModel::PasteNodes(ImVec2 viewOffsetPosition)
{
    if (IsClipboardEmpty())
    {
        return;
    }
    BeginTransaction(true);
    for (size_t i = 0; i < mNodesClipboard.size(); i++)
    {
        const Node& node = mNodes[i];
        //mEditingContext.UserAddStage();
        //size_t target = mEvaluationStages.mStages.size();
        //AddSingleNode(sourceNode.mType); todo
        auto nodeIndex = AddNode(node.mType, node.mPos);
        //mEvaluationStages.AddSingleEvaluation(sourceNode.mType);
        //auto& stage = mEvaluationStages.mStages[nodeIndex];
        //stage.mParameters = sourceNode.mParameters;
        //stage.mInputSamplers = sourceNode.mInputSamplers;
        //stage.mStartFrame = sourceNode.mStartFrame;
        //stage.mEndFrame = sourceNode.mEndFrame;

        //mEvaluationStages.SetEvaluationParameters(nodeIndex, stage.mParameters);
        //mEvaluationStages.SetSamplers(nodeIndex, stage.mInputSamplers);
        //mEvaluationStages.SetIOPin(nodeIndex, )
        //mEvaluationStages.SetTime(&mEditingContext, mEditingContext.GetCurrentTime(), true);
        //mEditingContext.SetTargetDirty(target, Dirty::All);
    }
    ImVec2 min(FLT_MAX, FLT_MAX);
    for (auto& clipboardNode : mNodesClipboard)
    {
        min.x = ImMin(clipboardNode.mPos.x, min.x);
        min.y = ImMin(clipboardNode.mPos.y, min.y);
    }
    for (auto& selnode : mNodes)
    {
        selnode.mbSelected = false;
    }
    for (auto& clipboardNode : mNodesClipboard)
    {
        auto ur = mUndoRedo ? std::make_unique<URAdd<Node>>(int(mNodes.size()), [&]() { return &mNodes; }) : nullptr;
        mNodes.push_back(clipboardNode);
        mNodes.back().mPos += viewOffsetPosition;//(io.MousePos - offset) / factor - min;
        mNodes.back().mbSelected = true;
    }
    EndTransaction();
}

const Parameters& GraphModel::GetParameters(size_t nodeIndex) const
{
    return mNodes[nodeIndex].mParameters;
}

static ImRect DisplayRectMargin(ImRect rect)
{
    extern ImVec2 captureOffset;
    // margins
    static const float margin = 10.f;
    rect.Min += captureOffset;
    rect.Max += captureOffset;
    rect.Min -= ImVec2(margin, margin);
    rect.Max += ImVec2(margin, margin);
    return rect;
}

ImRect GraphModel::GetNodesDisplayRect() const
{
    ImRect rect(ImVec2(0.f, 0.f), ImVec2(0.f, 0.f));
    for (auto& node : mNodes)
    {
        int width = gMetaNodes[node.mType].mWidth;
        int height = gMetaNodes[node.mType].mHeight;
        rect.Add(ImRect(node.mPos, node.mPos + ImVec2(float(width), float(height))));
    }

    return DisplayRectMargin(rect);
}

ImRect GraphModel::GetFinalNodeDisplayRect() const
{
    auto& evaluationOrder = mEvaluationOrderList;
    ImRect rect(ImVec2(0.f, 0.f), ImVec2(0.f, 0.f));
    if (!evaluationOrder.empty() && !mNodes.empty())
    {
        auto& node = mNodes[evaluationOrder.back()];
        int width = gMetaNodes[node.mType].mWidth;
        int height = gMetaNodes[node.mType].mHeight;
        rect = ImRect(node.mPos, node.mPos + ImVec2(float(width), float(height)));
    }
    return DisplayRectMargin(rect);
}

void GraphModel::RecurseNodeGraphLayout(std::vector<NodePosition>& positions,
    std::map<int, int>& stacks,
    size_t currentIndex,
    int currentLayer)
{
    const auto& nodes = mNodes;
    const auto& links = mLinks;

    if (positions[currentIndex].mLayer == -1)
    {
        positions[currentIndex].mLayer = currentLayer;
        int layer = positions[currentIndex].mLayer = currentLayer;
        if (stacks.find(layer) != stacks.end())
            stacks[layer]++;
        else
            stacks[layer] = 0;
        positions[currentIndex].mStackIndex = stacks[currentLayer];
    }
    else
    {
        // already hooked node
        if (currentLayer > positions[currentIndex].mLayer)
        {
            // remove stack at current pos
            int currentStack = positions[currentIndex].mStackIndex;
            for (auto& pos : positions)
            {
                if (pos.mLayer == positions[currentIndex].mLayer && pos.mStackIndex > currentStack)
                {
                    pos.mStackIndex--;
                    stacks[pos.mLayer]--;
                }
            }
            // apply new one
            int layer = positions[currentIndex].mLayer = currentLayer;
            if (stacks.find(layer) != stacks.end())
                stacks[layer]++;
            else
                stacks[layer] = 0;
            positions[currentIndex].mStackIndex = stacks[currentLayer];
        }
    }

    size_t InputsCount = gMetaNodes[nodes[currentIndex].mType].mInputs.size();
    std::vector<int> inputNodes(InputsCount, -1);
    for (auto& link : links)
    {
        if (link.mOutputNodeIndex != currentIndex)
            continue;
        inputNodes[link.mOutputSlotIndex] = link.mInputNodeIndex;
    }
    for (auto inputNode : inputNodes)
    {
        if (inputNode == -1)
            continue;
        RecurseNodeGraphLayout(positions, stacks, inputNode, currentLayer + 1);
    }
}

void GraphModel::NodeGraphLayout()
{
    auto orderList = mEvaluationOrderList;

    // get stack/layer pos
    std::vector<NodePosition> nodePositions(mNodes.size(), { -1, -1, -1 });
    std::map<int, int> stacks;
    ImRect sourceRect, destRect;
    BeginTransaction(true);
    std::vector<ImVec2> nodePos(mNodes.size());

    // compute source bounds
    for (unsigned int i = 0; i < mNodes.size(); i++)
    {
        const auto& node = mNodes[i];
        sourceRect.Add(ImRect(node.mPos, node.mPos + ImVec2(100, 100)));
    }

    for (unsigned int i = 0; i < mNodes.size(); i++)
    {
        size_t nodeIndex = orderList[mNodes.size() - i - 1];
        RecurseNodeGraphLayout(nodePositions, stacks, nodeIndex, 0);
    }

    // set corresponding node index in nodePosition
    for (unsigned int i = 0; i < mNodes.size(); i++)
    {
        int nodeIndex = int(orderList[i]);
        auto& layout = nodePositions[nodeIndex];
        layout.mNodeIndex = nodeIndex;
    }

    // sort nodePositions
    std::sort(nodePositions.begin(), nodePositions.end());

    // set x,y position from layer/stack
    float currentStackHeight = 0.f;
    int currentLayerIndex = -1;
    for (unsigned int i = 0; i < nodePositions.size(); i++)
    {
        auto& layout = nodePositions[i];
        if (currentLayerIndex != layout.mLayer)
        {
            currentLayerIndex = layout.mLayer;
            currentStackHeight = 0.f;
        }
        size_t nodeIndex = layout.mNodeIndex;
        const auto& node = mNodes[nodeIndex];
        float height = float(gMetaNodes[node.mType].mHeight);
        nodePos[nodeIndex] = ImVec2(-layout.mLayer * 180.f, currentStackHeight);
        currentStackHeight += height + 40.f;
    }

    // new bounds
    for (unsigned int i = 0; i < mNodes.size(); i++)
    {
        ImVec2 newPos = nodePos[i];
        // todo: support height more closely with metanodes
        destRect.Add(ImRect(newPos, newPos + ImVec2(100, 100)));
    }

    // move all nodes
    ImVec2 offset = sourceRect.GetCenter() - destRect.GetCenter();
    for (auto& pos : nodePos)
    {
        pos += offset;
    }
    for (unsigned int i = 0; i < mNodes.size(); i++)
    {
        SetNodePosition(i, nodePos[i]);
    }

    // finish undo
    EndTransaction();
}

bool GraphModel::RecurseIsLinked(int from, int to) const
{
    for (auto& link : mLinks)
    {
        if (link.mInputNodeIndex == from)
        {
            if (link.mOutputNodeIndex == to)
                return true;

            if (RecurseIsLinked(link.mOutputNodeIndex, to))
                return true;
        }
    }
    return false;
}

void GraphModel::SetMultiplexed(size_t nodeIndex, size_t slotIndex, int multiplex)
{
    assert(mbTransaction);
    auto ur = mUndoRedo
        ? std::make_unique<URChange<Node>>(int(nodeIndex), [&](int index) { return &mNodes[index]; }, [&](int nodeIndex) { SetDirty(nodeIndex, Dirty::Input); })
        : nullptr;

    mNodes[nodeIndex].mMultiplexInput.mInputs[slotIndex] = multiplex;
    SetDirty(nodeIndex, Dirty::Input);
}

bool GraphModel::IsParameterPinned(size_t nodeIndex, size_t parameterIndex) const
{
    uint32_t mask = 1 << parameterIndex;
    return mNodes[nodeIndex].mPinnedParameters & mask;
}

bool GraphModel::IsIOPinned(size_t nodeIndex, size_t io, bool forOutput) const
{
    uint32_t mask = 0;
    if (forOutput)
    {
        mask = (1 << io) & 0xFF;
    }
    else
    {
        mask = (1 << (8 + io));
    }
    return mNodes[nodeIndex].mPinnedIO & mask;
}

static bool NodeTypeHasMultiplexer(size_t nodeType)
{
    for (const auto& parameter : gMetaNodes[nodeType].mParams)
    {
        if (parameter.mType == Con_Multiplexer)
        {
            return true;
        }
    }
    return false;
}

void GraphModel::GetMultiplexedInputs(size_t nodeIndex, std::vector<size_t>& list) const
{
    for (auto& input : mInputs[nodeIndex].mInputs)
    {
        if (input == -1)
        {
            continue;
        }
        if (!NodeTypeHasMultiplexer(mNodes[input].mType))
        {
            list.push_back(input);
        }
        else
        {
            GetMultiplexedInputs(input, list);
        }
    }
}

bool GraphModel::GetMultiplexedInputs(size_t nodeIndex, size_t slotIndex, std::vector<size_t>& list) const
{
    int input = mInputs[nodeIndex].mInputs[slotIndex];
    if (input == -1)
    {
        return false;
    }
    if (NodeTypeHasMultiplexer(mNodes[input].mType))
    {
        GetMultiplexedInputs(input, list);
        return true;
    }
    return false;
}

void GraphModel::SetParameterPins(const std::vector<uint32_t>& pins)
{
    for (auto i = 0; i < pins.size(); i++)
    {
        mNodes[i].mPinnedParameters = pins[i];
    }

}
void GraphModel::SetIOPins(const std::vector<uint32_t>& pins)
{
    for (auto i = 0; i < pins.size(); i++)
    {
        mNodes[i].mPinnedIO = pins[i];
    }
}

const std::vector<uint32_t> GraphModel::GetParameterPins() const
{
    std::vector<uint32_t> ret;
    ret.reserve(mNodes.size());
    for (auto& node : mNodes)
    {
        ret.push_back(node.mPinnedParameters);
    }
    return ret;
}
const std::vector<uint32_t> GraphModel::GetIOPins() const
{
    std::vector<uint32_t> ret;
    ret.reserve(mNodes.size());
    for (auto& node : mNodes)
    {
        ret.push_back(node.mPinnedIO);
    }
    return ret;

}

size_t GraphModel::PickBestNode(const std::vector<GraphModel::NodeOrder>& orders) const
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

void GraphModel::RecurseSetPriority(std::vector<GraphModel::NodeOrder>& orders,
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

std::vector<GraphModel::NodeOrder> GraphModel::ComputeEvaluationOrders()
{
    size_t nodeCount = mNodes.size();

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

void GraphModel::ComputeEvaluationOrder()
{
    mEvaluationOrderList.clear();

    auto orders = ComputeEvaluationOrders();
    std::sort(orders.begin(), orders.end());
    mEvaluationOrderList.resize(orders.size());
    for (size_t i = 0; i < orders.size(); i++)
    {
        mEvaluationOrderList[i] = orders[i].mNodeIndex;
    }
}

const std::vector<MultiplexInput> GraphModel::GetMultiplexInputs() const
{
    std::vector<MultiplexInput> ret;
    ret.reserve(mNodes.size());
    for (const auto& node : mNodes)
    {
        ret.push_back(node.mMultiplexInput);
    }
    return ret;
}

void GraphModel::SetStartEndFrame(int startFrame, int endFrame)
{
    assert(mbTransaction);
    mStartFrame = startFrame; 
    mEndFrame = endFrame; 
}
