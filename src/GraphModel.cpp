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

ImRect GraphModel::Node::GetDisplayRect() const
{
    int width = gMetaNodes[mType].mWidth;
    int height = gMetaNodes[mType].mHeight;
    return ImRect(mPos, mPos + ImVec2(float(width), float(height)));
}

GraphModel::GraphModel() : mbTransaction(false), mUndoRedo(nullptr), mStartFrame(0), mEndFrame(1)
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
}

std::vector<Input> GraphModel::GetInputs() const
{
    std::vector<Input> inputs;
    inputs.resize(mNodes.size());

    for(const auto& link: mLinks)
    {
        auto source = link.mInputNodeIndex;
        inputs[link.mOutputNodeIndex].mInputs[link.mInputSlotIndex] = source;
    }

    // overide
    for (auto i = 0; i < mNodes.size(); i++)
    {
        const auto& multiplexInput = mNodes[i].mMultiplexInput;
        for (auto j = 0; j < 8; j++)
        {
            int m = multiplexInput.mInputs[j];
            if (m != -1)
            {
                inputs[i].mInputs[j] = m;
            }
        }
    }
    return inputs;
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
                ? std::make_unique<URChange<Node>>(int(i), [this](int index) { return &mNodes[index]; }, [this](int index) {SetDirty(index, Dirty::VisualGraph);})
                : nullptr;
        node.mPos += delta;
        SetDirty(i, Dirty::VisualGraph);
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

    auto delNode = [this](int index) { SetDirty(index, Dirty::DeletedNode); };
    auto addNode = [this](int index) { SetDirty(index, Dirty::AddedNode); };

    auto urNode = mUndoRedo ? std::make_unique<URAdd<Node>>(int(nodeIndex), [this]() { return &mNodes; }, delNode, addNode) : nullptr;
    
    mNodes.push_back(Node(int(type), position, mStartFrame, mEndFrame));
    InitDefaultParameters(type, mNodes[nodeIndex].mParameters);
    mNodes.back().mSamplers.resize(gMetaNodes[type].mInputs.size());

    addNode(int(nodeIndex));
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

    auto inputChanged = [this](int index) { SetDirty(mLinks[index].mOutputNodeIndex, Dirty::Input); };
    size_t linkIndex = mLinks.size();

    auto ur = mUndoRedo ? std::make_unique<URAdd<Link>>(int(linkIndex), [&]() { return &mLinks; }, inputChanged, inputChanged)
                  : nullptr;

    AddLinkInternal(inputNodeIndex, inputSlotIndex, outputNodeIndex, outputSlotIndex);
    inputChanged(int(mLinks.size() - 1));
}

void GraphModel::DelLinkInternal(size_t linkIndex)
{
    mLinks.erase(mLinks.begin() + linkIndex);
}

void GraphModel::DelLink(size_t linkIndex)
{
    assert(mbTransaction);

    auto inputChanged = [this](int index) { SetDirty(mLinks[index].mOutputNodeIndex, Dirty::Input); };
    auto ur = mUndoRedo ? std::make_unique<URDel<Link>>(int(linkIndex), [&]() { return &mLinks; }, inputChanged, inputChanged)
                  : nullptr;

    inputChanged(int(linkIndex));
    DelLinkInternal(linkIndex);
}

void GraphModel::AddRug(const Rug& rug)
{
    assert(mbTransaction);

    auto rugDirty = [&](int index) {SetDirty(-1, Dirty::RugChanged); };
    auto ur = mUndoRedo
                  ? std::make_unique<URAdd<Rug>>(int(mRugs.size()), [&]() { return &mRugs; }, rugDirty, rugDirty)
                  : nullptr;

    mRugs.push_back(rug);
    rugDirty(-1);
}

void GraphModel::DelRug(size_t rugIndex)
{
    assert(mbTransaction);

    auto rugDirty = [&](int index) {SetDirty(-1, Dirty::RugChanged); };
    auto ur = mUndoRedo ? std::make_unique<URDel<Rug>>(int(rugIndex), [&]() { return &mRugs; }, rugDirty, rugDirty)
                        : nullptr;

    mRugs.erase(mRugs.begin() + rugIndex);
    rugDirty(-1);
}

void GraphModel::SetRug(size_t rugIndex, const Rug& rug)
{
    assert(mbTransaction);

    auto rugDirty = [&](int index) {SetDirty(-1, Dirty::RugChanged); };
    auto ur = mUndoRedo
                  ? std::make_unique<URChange<Rug>>(int(rugIndex), [&](int index) { return &mRugs[index]; }, rugDirty)
                  : nullptr;

    mRugs[rugIndex] = rug;
    rugDirty(-1);
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
        {
            continue;
        }
        
        auto delNode = [this](int index) { SetDirty(index, Dirty::DeletedNode); };
        auto addNode = [this](int index) { SetDirty(index, Dirty::AddedNode); };

        auto ur = mUndoRedo ? std::make_unique<URDel<Node>>(int(selection),
                                    [this]() { return &mNodes; },
                                    delNode, addNode) : nullptr;

        for (size_t i = 0; i < mLinks.size();)
        {
            auto& link = mLinks[i];
            if (link.mInputNodeIndex == selection || link.mOutputNodeIndex == selection)
            {
                DelLink(i);
            }
            else
            {
                i++;
            }
        }

        mNodes.erase(mNodes.begin() + selection);
        for (int id = 0; id < mLinks.size(); id++)
        {
            if (mLinks[id].mInputNodeIndex > selection)
            {
                auto ur = mUndoRedo ? std::make_unique<URChange<Link>>(id,
                    [this](int index) { return &mLinks[index]; }) : nullptr;

                mLinks[id].mInputNodeIndex--;
            }
            if (mLinks[id].mOutputNodeIndex > selection)
            {
                auto ur = mUndoRedo ? std::make_unique<URChange<Link>>(id,
                    [this](int index) { return &mLinks[index]; }) : nullptr;

                mLinks[id].mOutputNodeIndex--;
            }
        }
        RemoveAnimation(selection);
        SetDirty(selection, Dirty::DeletedNode);
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
    SetDirty(nodeIndex, Dirty::VisualGraph);
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
    SetDirty(nodeIndex, Dirty::Sampler);
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

    auto dirtyParameters = [this](int index) { SetDirty(index, Dirty::Parameter); };
    auto ur = mUndoRedo ? std::make_unique<URChange<Node>>(
                              int(nodeIndex),
                              [this](int index) { return &mNodes[index]; },
                              dirtyParameters)
                        : nullptr;

    mNodes[nodeIndex].mParameters = parameters;
    dirtyParameters(int(nodeIndex));
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
    ImRect rect;
    if (mNodes.empty())
    {
        return ImRect(ImVec2(0.f, 0.f), ImVec2(0.f, 0.f));
    }
    rect = mNodes[0].GetDisplayRect();
    for (auto& node : mNodes)
    {
        rect.Add(node.GetDisplayRect());
    }

    return DisplayRectMargin(rect);
}

ImRect GraphModel::GetFinalNodeDisplayRect(const std::vector<size_t>& orderList) const
{
    auto& evaluationOrder = orderList;
    ImRect rect(ImVec2(0.f, 0.f), ImVec2(0.f, 0.f));
    if (!evaluationOrder.empty() && !mNodes.empty())
    {
        auto& node = mNodes[evaluationOrder.back()];
        rect = node.GetDisplayRect();
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

void GraphModel::NodeGraphLayout(const std::vector<size_t>& orderList)
{
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
    if (from == to)
    {
        return true;
    }
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
        ? std::make_unique<URChange<Node>>(int(nodeIndex), [this](int index) { return &mNodes[index]; }, [this](int index) { SetDirty(index, Dirty::Input); })
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

void GraphModel::GetMultiplexedInputs(const std::vector<Input>& inputs, size_t nodeIndex, std::vector<size_t>& list) const
{
    for (auto& input : inputs[nodeIndex].mInputs)
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
            GetMultiplexedInputs(inputs, input, list);
        }
    }
}

bool GraphModel::GetMultiplexedInputs(const std::vector<Input>& inputs, size_t nodeIndex, size_t slotIndex, std::vector<size_t>& list) const
{
    int input = inputs[nodeIndex].mInputs[slotIndex];
    if (input == -1)
    {
        return false;
    }
    if (NodeTypeHasMultiplexer(mNodes[input].mType))
    {
        GetMultiplexedInputs(inputs, input, list);
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

void GraphModel::SetStartEndFrame(size_t nodeIndex, int startFrame, int endFrame)
{
    assert(mbTransaction);
    Node& node = mNodes[nodeIndex];
    node.mStartFrame = startFrame;
    node.mEndFrame = endFrame;
    SetDirty(nodeIndex, Dirty::StartEndTime);
}

