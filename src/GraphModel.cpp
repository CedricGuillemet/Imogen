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

#include "GraphModel.h"
#include <assert.h>
#include "UndoRedo.h"
#include <algorithm>

extern UndoRedoHandler gUndoRedoHandler;

GraphModel::GraphModel() : mbTransaction(false), mSelectedNodeIndex(-1), mUndoRedo(nullptr)
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
    mEvaluationStages.Clear();
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
    
    // set inputs
    mEvaluationStages.ClearInputs();
    for(const auto& link: mLinks)
    {
        mEvaluationStages.SetEvaluationInput(link.mOutputNodeIndex, link.mInputSlotIndex, link.mInputNodeIndex);
    }

    // inputs are fresh, can compute evaluation order
    mEvaluationStages.ComputeEvaluationOrder();
}

void GraphModel::Undo()
{
    assert(!mbTransaction);
    gUndoRedoHandler.Undo();
}

void GraphModel::Redo()
{
    assert(!mbTransaction);
    gUndoRedoHandler.Redo();
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
    auto& animTracks = mEvaluationStages.mAnimTrack;
    if (animTracks.empty())
        return;
    std::vector<int> tracks;
    for (int i = 0; i < int(animTracks.size()); i++)
    {
        const AnimTrack& animTrack = animTracks[i];
        if (animTrack.mNodeIndex == nodeIndex)
            tracks.push_back(i);
    }
    if (tracks.empty())
        return;

    for (int i = 0; i < int(tracks.size()); i++)
    {
        int index = tracks[i] - i;
        auto urAnimTrack = mUndoRedo ? std::make_unique<URDel<AnimTrack>>(index, [&] { return &animTracks; }):nullptr;
        animTracks.erase(animTracks.begin() + index);
    }
}
size_t GraphModel::AddNode(size_t type, ImVec2 position)
{
    assert(mbTransaction);

    size_t nodeIndex = mNodes.size();

    auto delNode = [&](int index) { DeleteNodeHelper(index); };
    auto addNode = [&](int index) { AddNodeHelper(index); };

    auto urNode = mUndoRedo ? std::make_unique<URAdd<Node>>(int(nodeIndex), [&]() { return &mNodes; }, delNode, addNode) : nullptr;
    auto urStage = mUndoRedo ? std::make_unique<URAdd<EvaluationStage>>(int(nodeIndex), [&]() { return &mEvaluationStages.mStages; }) : nullptr;
    auto urPinnedeParameters = mUndoRedo ? std::make_unique<URAdd<uint32_t>>(int(nodeIndex), [&]() { return &mEvaluationStages.mPinnedParameters; })
                            : nullptr;
    auto urPinnedIO = mUndoRedo ? std::make_unique<URAdd<uint32_t>>(int(nodeIndex), [&]() { return &mEvaluationStages.mPinnedIO; })
                        : nullptr;
    auto urParameters = mUndoRedo ? std::make_unique<URAdd<Parameters>>(int(nodeIndex), [&]() { return &mEvaluationStages.mParameters; })
                            : nullptr;
    auto urInputSamplers = mUndoRedo ? std::make_unique<URAdd<Samplers>>(int(nodeIndex), [&]() { return &mEvaluationStages.mInputSamplers; })
                    : nullptr;

    auto urMultiplexInputs = mUndoRedo ? std::make_unique<URAdd<MultiplexInput>>(int(nodeIndex), [&]() { return &mEvaluationStages.mMultiplexInputs; })
        : nullptr;
    
    mNodes.push_back(Node(int(type), position));
    mEvaluationStages.AddSingleEvaluation(type);
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

    assert(outputNodeIndex < mEvaluationStages.mStages.size());

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

        // delete stage data
        auto urStage = mUndoRedo ? std::make_unique<URDel<EvaluationStage>>(int(selection), [&]() { return &mEvaluationStages.mStages; }) : nullptr;
        auto urPinnedeParameters = mUndoRedo ? std::make_unique<URDel<uint32_t>>(int(selection), [&]() { return &mEvaluationStages.mPinnedParameters; })
            : nullptr;
        auto urPinnedIO = mUndoRedo ? std::make_unique<URDel<uint32_t>>(int(selection), [&]() { return &mEvaluationStages.mPinnedIO; })
            : nullptr;
        auto urParameters = mUndoRedo ? std::make_unique<URDel<Parameters>>(int(selection), [&]() { return &mEvaluationStages.mParameters; })
            : nullptr;
        auto urInputSamplers = mUndoRedo ? std::make_unique<URDel<Samplers>>(int(selection), [&]() { return &mEvaluationStages.mInputSamplers; })
            : nullptr;
        auto urMultiplexInputs = mUndoRedo ? std::make_unique<URDel<MultiplexInput>>(int(selection), [&]() { return &mEvaluationStages.mMultiplexInputs; })
            : nullptr;

        mNodes.erase(mNodes.begin() + selection);
        mEvaluationStages.DelSingleEvaluation(selection);
        DeleteNodeHelper(selection);
    }

    assert(mEvaluationStages.mStages.size() == mNodes.size());
    assert(mEvaluationStages.mPinnedParameters.size() == mNodes.size());
    assert(mEvaluationStages.mParameters.size() == mNodes.size());
    assert(mEvaluationStages.mPinnedIO.size() == mNodes.size());
    assert(mEvaluationStages.mInputSamplers.size() == mNodes.size());
    assert(mEvaluationStages.mMultiplexInputs.size() == mNodes.size());
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

    auto ur = mUndoRedo ? std::make_unique<URChange<std::vector<InputSampler>>>(
                              int(nodeIndex),
                              [&](int index) { return &mEvaluationStages.mInputSamplers[index]; },
                              [&](int index) {
                                  // auto& stage = mEvaluationStages.mStages[index];
                                  // mEvaluationStages.SetSamplers(index, stage.mInputSamplers);
                                  // mEditingContext.SetTargetDirty(index, Dirty::Sampler);
                              })
                        : nullptr;
    mEvaluationStages.SetSamplers(nodeIndex, samplers);
}

void GraphModel::SetEvaluationOrder(const std::vector<size_t>& nodeOrderList)
{
    assert(!mbTransaction);
    mEvaluationStages.SetEvaluationOrder(nodeOrderList);
}

bool GraphModel::NodeHasUI(size_t nodeIndex) const
{
    if (mEvaluationStages.mStages.size() <= nodeIndex)
        return false;
    return gMetaNodes[mEvaluationStages.mStages[nodeIndex].mType].mbHasUI;
}

bool GraphModel::IsIOPinned(size_t nodeIndex, size_t io, bool forOutput) const
{
    return mEvaluationStages.IsIOPinned(nodeIndex, io, forOutput);
}

bool GraphModel::IsParameterPinned(size_t nodeIndex, size_t parameterIndex) const
{
    return mEvaluationStages.IsParameterPinned(nodeIndex, parameterIndex);
}

void GraphModel::SetParameter(size_t nodeIndex, const std::string& parameterName, const std::string& parameterValue)
{
    assert(mbTransaction);
    if (nodeIndex < 0 || nodeIndex >= mEvaluationStages.mStages.size())
    {
        return;
    }
    uint32_t nodeType = uint32_t(mEvaluationStages.mStages[nodeIndex].mType);
    int parameterIndex = GetParameterIndex(nodeType, parameterName.c_str());
    if (parameterIndex == -1)
    {
        return;
    }
    ConTypes parameterType = GetParameterType(nodeType, parameterIndex);
    size_t paramOffset = GetParameterOffset(nodeType, parameterIndex);
    ParseStringToParameter(
        parameterValue, parameterType, &mEvaluationStages.mParameters[nodeIndex][paramOffset]);
    SetDirty(nodeIndex, Dirty::Parameter);
}

AnimTrack* GraphModel::GetAnimTrack(uint32_t nodeIndex, uint32_t parameterIndex)
{
    for (auto& animTrack : mEvaluationStages.mAnimTrack)
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
        uint32_t parameterType = gMetaNodes[mEvaluationStages.mStages[nodeIndex].mType].mParams[parameterIndex].mType;
        AnimTrack newTrack;
        newTrack.mNodeIndex = nodeIndex;
        newTrack.mParamIndex = parameterIndex;
        newTrack.mValueType = parameterType;
        newTrack.mAnimation = AllocateAnimation(parameterType);
        mEvaluationStages.mAnimTrack.push_back(newTrack);
        animTrack = &mEvaluationStages.mAnimTrack.back();
    }
    /*URChange<AnimTrack> urChange(int(animTrack - mEvaluationStages.mAnimTrack.data()),
                                 [&](int index) { return &mEvaluationStages.mAnimTrack[index]; });*/
    EvaluationStage& stage = mEvaluationStages.mStages[nodeIndex];
    size_t parameterOffset = GetParameterOffset(uint32_t(stage.mType), parameterIndex);
    animTrack->mAnimation->SetValue(frame, &mEvaluationStages.mParameters[parameterOffset]);
}

void GraphModel::GetKeyedParameters(int frame, uint32_t nodeIndex, std::vector<bool>& keyed) const
{
}

void GraphModel::SetIOPin(size_t nodeIndex, size_t io, bool forOutput, bool pinned)
{
    assert(mbTransaction);
    auto ur = mUndoRedo ? std::make_unique<URChange<uint32_t>>(
        int(nodeIndex),
        [&](int index) { return &mEvaluationStages.mPinnedIO[index]; })
        : nullptr;
    mEvaluationStages.SetIOPin(nodeIndex, io, forOutput, pinned);
}

void GraphModel::SetParameterPin(size_t nodeIndex, size_t parameterIndex, bool pinned)
{
    assert(mbTransaction);
    auto ur = mUndoRedo ? std::make_unique<URChange<uint32_t>>(
        int(nodeIndex),
        [&](int index) { return &mEvaluationStages.mPinnedParameters[index]; })
        : nullptr;
    mEvaluationStages.SetParameterPin(nodeIndex, parameterIndex, pinned);
}

void GraphModel::SetParameters(size_t nodeIndex, const std::vector<unsigned char>& parameters)
{
    assert(mbTransaction);

    auto ur = mUndoRedo ? std::make_unique<URChange<Parameters>>(
                              int(nodeIndex),
                              [&](int index) { return &mEvaluationStages.mParameters[index]; },
                              [&](int index) {
                                  // auto& stage = mEvaluationStages.mStages[index];
                                  // mEvaluationStages.SetSamplers(index, stage.mInputSamplers);
                                  // mEditingContext.SetTargetDirty(index, Dirty::Sampler);
                              })
                        : nullptr;

    mEvaluationStages.mParameters[nodeIndex] = parameters;
    SetDirty(nodeIndex, Dirty::Parameter);
}

void GraphModel::SetTimeSlot(size_t nodeIndex, int frameStart, int frameEnd)
{
    assert(mbTransaction);

    auto& stage = mEvaluationStages.mStages[nodeIndex];
    stage.mStartFrame = frameStart;
    stage.mEndFrame = frameEnd;
}

void GraphModel::SetKeyboardMouse(size_t nodeIndex, const UIInput& input)
{
    mEvaluationStages.SetKeyboardMouse(nodeIndex, input);
}

void GraphModel::CopySelectedNodes()
{
    mStagesClipboard.clear();
    mNodesClipboard.clear();
    for (auto i = 0; i < mNodes.size(); i++)
    {
        if (!mNodes[i].mbSelected)
        {
            continue;
        }
        mStagesClipboard.push_back(mEvaluationStages.mStages[i]);
        mNodesClipboard.push_back(mNodes[i]);
    }
}

void GraphModel::CutSelectedNodes()
{
    if (!HasSelectedNodes())
    {
        return;
    }
    mStagesClipboard.clear();
    CopySelectedNodes();
    BeginTransaction(true);
    DeleteSelectedNodes();
    EndTransaction();
}

bool GraphModel::IsClipboardEmpty() const
{
    return mStagesClipboard.empty();
}

void GraphModel::PasteNodes(ImVec2 viewOffsetPosition)
{
    if (IsClipboardEmpty())
    {
        return;
    }
    BeginTransaction(true);
    for (size_t i = 0;i<mNodesClipboard.size();i++)
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

const std::vector<unsigned char>& GraphModel::GetParameters(size_t nodeIndex) const
{
    return mEvaluationStages.mParameters[nodeIndex];
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
    auto& evaluationOrder = mEvaluationStages.GetForwardEvaluationOrder();
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
    auto orderList = mEvaluationStages.GetForwardEvaluationOrder();

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
        ? std::make_unique<URChange<MultiplexInput>>(int(nodeIndex), [&](int index) { return &mEvaluationStages.mMultiplexInputs[index]; }, [&](int nodeIndex) { SetDirty(nodeIndex, Dirty::Input); })
        : nullptr;

    mEvaluationStages.mMultiplexInputs[nodeIndex].mInputs[slotIndex] = multiplex;
    SetDirty(nodeIndex, Dirty::Input);
}
