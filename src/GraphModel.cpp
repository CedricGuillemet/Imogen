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
#include <memory>
#include <set>
#include "GraphModel.h"
#include "UndoRedo.h"

extern UndoRedoHandler gUndoRedoHandler;
size_t GetUndoRedoMemoryFootPrint()
{
    return gUndoRedoHandler.GetMemUsed();
}


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

void GraphModel::GetInputs(std::vector<Input>& multiplexedInPuts, std::vector<Input>& directInputs) const
{
    std::vector<Input> inputs;
    inputs.resize(mNodes.size());

    for(const auto& link: mLinks)
    {
        auto source = link.mInputNodeIndex;
		assert(source < mNodes.size() && source > -1);
		assert(link.mOutputSlotIndex < 8 && link.mOutputSlotIndex > -1);
        inputs[link.mOutputNodeIndex].mInputs[link.mOutputSlotIndex] = source;
    }

    directInputs = inputs;

    // overide
    for (auto i = 0; i < mNodes.size(); i++)
    {
        const auto& multiplexInput = mNodes[i].mMultiplexInput;
        for (auto j = 0; j < 8; j++)
        {
            auto m = multiplexInput.mInputs[j];
            if (m.IsValid())
            {
                inputs[i].mInputs[j] = m;
            }
        }
    }
    multiplexedInPuts = inputs;
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
    for (auto i = 0 ; i < mNodes.size(); i++)
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

void GraphModel::SetNodePosition(NodeIndex nodeIndex, const ImVec2 position)
{
    assert(mbTransaction);
    auto ur = mUndoRedo
        ? std::make_unique<URChange<Node>>(int(nodeIndex), [this](int index) { return &mNodes[index]; }, [this](int index) {SetDirty(index, Dirty::VisualGraph); })
        : nullptr;
    mNodes[nodeIndex].mPos = position;
    SetDirty(nodeIndex, Dirty::VisualGraph);
}

// called when a node is added by user, or with undo/redo
void GraphModel::AddNodeHelper(int nodeIndex)
{
    SetDirty(nodeIndex, Dirty::AddedNode);
}

void GraphModel::RemoveAnimation(NodeIndex nodeIndex)
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

	NodeIndex nodeIndex = mNodes.size();

    auto delNode = [this](int index) { SetDirty(index, Dirty::DeletedNode); };
    auto addNode = [this](int index) { SetDirty(index, Dirty::AddedNode); };

    auto urNode = mUndoRedo ? std::make_unique<URAdd<Node>>(int(nodeIndex), [this]() { return &mNodes; }, delNode, addNode) : nullptr;
    
    mNodes.push_back(Node(int(type), position, mStartFrame, mEndFrame));
    InitDefaultParameters(type, mNodes[nodeIndex].mParameters);
    mNodes.back().mSamplers.resize(gMetaNodes[type].mInputs.size());

    addNode(int(nodeIndex));
    return nodeIndex;
}

void GraphModel::AddLinkInternal(NodeIndex inputNodeIndex, SlotIndex inputSlotIndex, NodeIndex outputNodeIndex, SlotIndex outputSlotIndex)
{
    size_t linkIndex = mLinks.size();
    Link link;
    link.mInputNodeIndex = inputNodeIndex;
    link.mInputSlotIndex = inputSlotIndex;
    link.mOutputNodeIndex = outputNodeIndex;
    link.mOutputSlotIndex = outputSlotIndex;
    mLinks.push_back(link);
}

void GraphModel::AddLink(NodeIndex inputNodeIndex, SlotIndex inputSlotIndex, NodeIndex outputNodeIndex, SlotIndex outputSlotIndex)
{
    assert(mbTransaction);

    if (inputNodeIndex >= mNodes.size() || outputNodeIndex >= mNodes.size())
    {
        // Log("Error : Link node index doesn't correspond to an existing node.");
        return;
    }

    auto inputChanged = [this, outputSlotIndex](int index) { SetDirty(mLinks[index].mOutputNodeIndex, Dirty::Input, outputSlotIndex); };
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

    auto inputChanged = [this](int index) { SetDirty(mLinks[index].mOutputNodeIndex, Dirty::Input, mLinks[index].mOutputSlotIndex); };
    auto ur = mUndoRedo ? std::make_unique<URDel<Link>>(int(linkIndex), [&]() { return &mLinks; }, inputChanged, inputChanged)
                  : nullptr;

    inputChanged(int(linkIndex));
    DelLinkInternal(linkIndex);
}

void GraphModel::DelLink(NodeIndex inputNodeIndex, SlotIndex inputSlotIndex)
{
	assert(mbTransaction);
	
	for (size_t linkIndex = 0; linkIndex < mLinks.size(); linkIndex++)
	{
		const Link& link = mLinks[linkIndex];
		if (link.mOutputNodeIndex == inputNodeIndex && link.mOutputSlotIndex == inputSlotIndex)
		{
			auto inputChanged = [this, inputSlotIndex](int index) { SetDirty(mLinks[index].mOutputNodeIndex, Dirty::Input, inputSlotIndex); };
			auto ur = mUndoRedo ? std::make_unique<URDel<Link>>(int(linkIndex), [&]() { return &mLinks; }, inputChanged, inputChanged)
				: nullptr;

			inputChanged(int(linkIndex));
			DelLinkInternal(linkIndex);
			return;
		}
	}
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
		NodeIndex nodeIndex = selection;
		DeleteNode(nodeIndex);
    }
}

void GraphModel::DeleteNode(NodeIndex nodeIndex)
{
	auto delNode = [this](int index) { SetDirty(index, Dirty::DeletedNode); };
	auto addNode = [this](int index) { SetDirty(index, Dirty::AddedNode); };

	auto ur = mUndoRedo ? std::make_unique<URDel<Node>>(nodeIndex,
		[this]() { return &mNodes; },
		delNode, addNode) : nullptr;

	for (size_t i = 0; i < mLinks.size();)
	{
		auto& link = mLinks[i];
		if (link.mInputNodeIndex == nodeIndex || link.mOutputNodeIndex == nodeIndex)
		{
			DelLink(i);
		}
		else
		{
			i++;
		}
	}

	mNodes.erase(mNodes.begin() + nodeIndex);
	for (int id = 0; id < mLinks.size(); id++)
	{
		if (mLinks[id].mInputNodeIndex > nodeIndex)
		{
			auto ur = mUndoRedo ? std::make_unique<URChange<Link>>(id,
				[this](int index) { return &mLinks[index]; }) : nullptr;

			mLinks[id].mInputNodeIndex--;
		}
		if (mLinks[id].mOutputNodeIndex > nodeIndex)
		{
			auto ur = mUndoRedo ? std::make_unique<URChange<Link>>(id,
				[this](int index) { return &mLinks[index]; }) : nullptr;

			mLinks[id].mOutputNodeIndex--;
		}
	}
	RemoveAnimation(nodeIndex);
	SetDirty(nodeIndex, Dirty::DeletedNode);
}

bool GraphModel::IsIOUsed(NodeIndex nodeIndex, int slotIndex, bool forOutput) const
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

void GraphModel::SelectNode(NodeIndex nodeIndex, bool selected)
{
    // assert(mbTransaction);
    mNodes[nodeIndex].mbSelected = selected;
    SetDirty(nodeIndex, Dirty::VisualGraph);
}

void GraphModel::UnselectNodes()
{
    for (size_t i = 0; i < GetNodeCount(); i++)
    {
        mNodes[i].mbSelected = false;
        SetDirty(i, Dirty::VisualGraph);
    }
}

ImVec2 GraphModel::GetNodePos(NodeIndex nodeIndex) const
{
    return mNodes[nodeIndex].mPos;
}

void GraphModel::SetSamplers(NodeIndex nodeIndex, const std::vector<InputSampler>& samplers)
{
    assert(mbTransaction);

    auto ur = mUndoRedo ? std::make_unique<URChange<Node>>(
                              int(nodeIndex),
                              [this](int index) { return &mNodes[index]; },
                              [this](int index) { SetDirty(index, Dirty::Sampler); })
                        : nullptr;
    mNodes[nodeIndex].mSamplers = samplers;
    SetDirty(nodeIndex, Dirty::Sampler);
}

void GraphModel::SetSampler(NodeIndex nodeIndex, size_t input, const InputSampler& sampler)
{
	assert(mbTransaction);
	if (input >= mNodes[nodeIndex].mSamplers.size())
	{
		return ;
	}
	auto ur = mUndoRedo ? std::make_unique<URChange<Node>>(
		int(nodeIndex),
		[this](int index) { return &mNodes[index]; },
		[this](int index) { SetDirty(index, Dirty::Sampler); })
		: nullptr;
	mNodes[nodeIndex].mSamplers[input] = sampler;
	SetDirty(nodeIndex, Dirty::Sampler);
}

bool GraphModel::NodeHasUI(NodeIndex nodeIndex) const
{
    return gMetaNodes[mNodes[nodeIndex].mType].mbHasUI;
}

void GraphModel::SetParameter(NodeIndex nodeIndex, const std::string& parameterName, const std::string& parameterValue)
{
    assert(mbTransaction);
	if (!nodeIndex.IsValid() || nodeIndex >= mNodes.size())
	{
		return;
	}
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

void GraphModel::SetCameraLookAt(NodeIndex nodeIndex, const Vec4& eye, const Vec4& target, const Vec4& up)
{
	assert(mbTransaction);
	uint32_t nodeType = uint32_t(mNodes[nodeIndex].mType);
	Camera* camera = GetCameraParameter(nodeType, mNodes[nodeIndex].mParameters);
	if (!camera)
	{
		return;
	}
	camera->LookAt(eye, target, up);
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
        uint32_t parameterType = gMetaNodes[mNodes[nodeIndex].mType].mParams[parameterIndex].mType;
        AnimTrack newTrack;
        newTrack.mNodeIndex = nodeIndex;
        newTrack.mParamIndex = parameterIndex;
        newTrack.mValueType = parameterType;
        newTrack.mAnimation = AllocateAnimation(parameterType);
        mAnimTrack.push_back(newTrack);
        animTrack = &mAnimTrack.back();
    }

    auto ur = mUndoRedo ? std::make_unique<URChange<AnimTrack>>(
        int(animTrack - mAnimTrack.data()),
        [&](int index) { return &mAnimTrack[index]; })
        : nullptr;

    size_t parameterOffset = GetParameterOffset(uint32_t(mNodes[nodeIndex].mType), parameterIndex);
    animTrack->mAnimation->SetValue(frame, &mNodes[nodeIndex].mParameters[parameterOffset]);
}

void GraphModel::GetKeyedParameters(int frame, uint32_t nodeIndex, std::vector<bool>& keyed) const
{
}

void GraphModel::SetIOPin(NodeIndex nodeIndex, size_t io, bool forOutput, bool pinned)
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

void GraphModel::SetParameterPin(NodeIndex nodeIndex, size_t parameterIndex, bool pinned)
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

void GraphModel::SetParameters(NodeIndex nodeIndex, const std::vector<unsigned char>& parameters)
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
    auto firstNodeIndex = mNodes.size();

    ImVec2 min(FLT_MAX, FLT_MAX);
    for (auto& clipboardNode : mNodesClipboard)
    {
        min.x = ImMin(clipboardNode.mPos.x, min.x);
        min.y = ImMin(clipboardNode.mPos.y, min.y);
    }
    const ImVec2 offset = viewOffsetPosition - min;
    for (auto& selnode : mNodes)
    {
        selnode.mbSelected = false;
    }

    for (size_t i = 0; i < mNodesClipboard.size(); i++)
    {
		NodeIndex nodeIndex = mNodes.size();

        auto delNode = [this](int index) { SetDirty(index, Dirty::DeletedNode); };
        auto addNode = [this](int index) { SetDirty(index, Dirty::AddedNode); };

        auto urNode = mUndoRedo ? std::make_unique<URAdd<Node>>(int(nodeIndex), [this]() { return &mNodes; }, delNode, addNode) : nullptr;

        mNodes.push_back(mNodesClipboard[i]);
        mNodes.back().mPos += offset;
        mNodes.back().mbSelected = true;
        addNode(nodeIndex);
    }

    EndTransaction();
}

const Parameters& GraphModel::GetParameters(NodeIndex nodeIndex) const
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
	if (mNodes.empty())
	{
		return;
	}
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
		NodeIndex nodeIndex = orderList[mNodes.size() - i - 1];
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
		NodeIndex nodeIndex = layout.mNodeIndex;
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
    for (auto i = 0; i < mNodes.size(); i++)
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

void GraphModel::SetMultiplexed(NodeIndex nodeIndex, SlotIndex slotIndex, int multiplex)
{
    assert(mbTransaction);
    auto ur = mUndoRedo
        ? std::make_unique<URChange<Node>>(nodeIndex, [this](int index) { return &mNodes[index]; }, [this, slotIndex](int index) { SetDirty(index, Dirty::Input, slotIndex); })
        : nullptr;

    mNodes[nodeIndex].mMultiplexInput.mInputs[slotIndex] = multiplex;
    SetDirty(nodeIndex, Dirty::Input, slotIndex);
}

bool GraphModel::IsParameterPinned(NodeIndex nodeIndex, size_t parameterIndex) const
{
    uint32_t mask = 1 << parameterIndex;
    return mNodes[nodeIndex].mPinnedParameters & mask;
}

bool GraphModel::IsIOPinned(NodeIndex nodeIndex, size_t io, bool forOutput) const
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

void GraphModel::GetMultiplexedInputs(const std::vector<Input>& inputs, NodeIndex nodeIndex, std::set<NodeIndex>& list) const
{
    for (auto input : inputs[nodeIndex].mInputs)
    {
        if (!input.IsValid())
        {
            continue;
        }
        if (!NodeTypeHasMultiplexer(mNodes[input].mType))
        {
            list.insert(input);
        }
        else
        {
            GetMultiplexedInputs(inputs, input, list);
        }
    }
}

bool GraphModel::GetMultiplexedInputs(const std::vector<Input>& inputs, NodeIndex nodeIndex, SlotIndex slotIndex, std::vector<NodeIndex>& list) const
{
    NodeIndex input = inputs[nodeIndex].mInputs[slotIndex];
    if (!input.IsValid())
    {
        return false;
    }
    if (NodeTypeHasMultiplexer(mNodes[input].mType))
    {
		std::set<NodeIndex> uniqueList;
        GetMultiplexedInputs(inputs, input, uniqueList);
		for (auto nodeIndex : uniqueList)
		{
			list.push_back(nodeIndex);
		}
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

	auto urStart = mUndoRedo ? std::make_unique<URChangeValue<int>>(mStartFrame): nullptr;
	auto urEnd = mUndoRedo ? std::make_unique<URChangeValue<int>>(mEndFrame) : nullptr;

    mStartFrame = startFrame; 
    mEndFrame = endFrame; 
}

void GraphModel::SetStartEndFrame(NodeIndex nodeIndex, int startFrame, int endFrame)
{
    assert(mbTransaction);
	auto ur = mUndoRedo
		? std::make_unique<URChange<Node>>(int(nodeIndex), [this](int index) { return &mNodes[index]; }, [this](int index) {SetDirty(index, Dirty::StartEndTime); })
		: nullptr;

    Node& node = mNodes[nodeIndex];
    node.mStartFrame = startFrame;
    node.mEndFrame = endFrame;
    SetDirty(nodeIndex, Dirty::StartEndTime);
}

NodeIndex GraphModel::GetNodeIndex(RuntimeId runtimeUniqueId) const
{
    for (int nodeIndex = 0; nodeIndex < mNodes.size(); nodeIndex ++)
    {
        if (mNodes[nodeIndex].mRuntimeUniqueId == runtimeUniqueId)
        {
            return nodeIndex;
        }
    }
    return InvalidNodeIndex;
}
