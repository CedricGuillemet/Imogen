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
#pragma once

#include "imgui.h"
#include <vector>
#include <string>
#include <stdint.h>
#include <set>
#include "Library.h"
#include "EvaluationStages.h"
#include "ParameterBlock.h"
struct UndoRedoHandler;
struct UndoRedo;

typedef std::vector<InputSampler> Samplers;

class GraphModel
{
public:
    GraphModel();
    ~GraphModel();

    struct Rug
    {
        ImVec2 mPos, mSize;
        uint32_t mColor;
        std::string mText;
    };

    struct Node
    {
        Node(int nodeType, ImVec2 position, int startFrame, int endFrame) : mNodeType(nodeType)
            , mPos(position)
            , mbSelected(false)
            , mPinnedIO(0)
            , mPinnedParameters(0)
            , mStartFrame(startFrame)
            , mEndFrame(endFrame)
            , mParameterBlock(nodeType)
        {
        }

        Node() : mNodeType(-1), mbSelected(false), mPinnedIO(0), mPinnedParameters(0), mParameterBlock(-1)
        {
        }

        int mNodeType;
        ImVec2 mPos;
        uint32_t mPinnedIO;
        uint32_t mPinnedParameters;
        int mStartFrame, mEndFrame;
        ParameterBlock mParameterBlock;
        MultiplexInput mMultiplexInput;
        Samplers mSamplers;
        bool mbSelected;
		RuntimeId mRuntimeUniqueId;
        // Helpers
        ImRect GetDisplayRect() const;
    };

    struct Link
    {
        int mInputNodeIndex, mInputSlotIndex, mOutputNodeIndex, mOutputSlotIndex;
        bool operator==(const Link& other) const
        {
            return mInputNodeIndex == other.mInputNodeIndex && mInputSlotIndex == other.mInputSlotIndex &&
                   mOutputNodeIndex == other.mOutputNodeIndex && mOutputSlotIndex == other.mOutputSlotIndex;
        }
    };

    // Transaction
    void Clear();
    void BeginTransaction(bool undoable);
    bool InTransaction() const;
    void EndTransaction();

    // undo/redo
    void Undo();
    void Redo();

    // setters
    size_t AddNode(size_t type, ImVec2 position);
    void SelectNode(NodeIndex nodeIndex, bool selected);
    void UnselectNodes();
    void MoveSelectedNodes(const ImVec2 delta);
    void SetNodePosition(NodeIndex nodeIndex, const ImVec2 position);
    void DeleteSelectedNodes();
	void DeleteNode(NodeIndex nodeIndex);
    void AddLink(NodeIndex inputNodeIndex, SlotIndex inputSlotIndex, NodeIndex outputNodeIndex, SlotIndex outputSlotIndex);
	void DelLink(NodeIndex inputNodeIndex, SlotIndex inputSlotIndex);
    void DelLink(size_t linkIndex);
    void AddRug(const Rug& rug);
    void DelRug(size_t rugIndex);
    void SetRug(size_t rugIndex, const Rug& rug);
    void SetSamplers(NodeIndex nodeIndex, const Samplers& samplers);
	void SetSampler(NodeIndex nodeIndex, size_t input, const InputSampler& sampler);
    void SetParameter(NodeIndex nodeIndex, const std::string& parameterName, const std::string& parameterValue);
    void SetParameterBlock(NodeIndex nodeIndex, const ParameterBlock& parameterBlock);
    void MakeKey(int frame, uint32_t nodeIndex, uint32_t parameterIndex);
    void SetIOPin(NodeIndex nodeIndex, size_t io, bool forOutput, bool pinned);
    void SetParameterPin(NodeIndex nodeIndex, size_t parameterIndex, bool pinned);
    void SetMultiplexed(NodeIndex nodeIndex, SlotIndex slotIndex, int multiplex);
    void SetMultiplexInputs(const std::vector<MultiplexInput>& multiplexInputs);
    void SetParameterPins(const std::vector<uint32_t>& pins);
    void SetIOPins(const std::vector<uint32_t>& pins);
    void SetAnimTrack(const std::vector<AnimTrack>& animTrack);
    void SetStartEndFrame(int startFrame, int endFrame);
    void SetStartEndFrame(NodeIndex nodeIndex, int startFrame, int endFrame);
	void SetCameraLookAt(NodeIndex nodeIndex, const Vec4& eye, const Vec4& target, const Vec4& up = Vec4(0.f, 1.f, 0.f, 0.f));
    // transaction is handled is the function
    void NodeGraphLayout(const std::vector<size_t>& orderList);

    // getters
    size_t GetNodeCount() const { return mNodes.size(); }
    int GetNodeType(NodeIndex nodeIndex) const { return mNodes[nodeIndex].mNodeType; }
    bool NodeHasUI(NodeIndex nodeIndex) const;
    const std::vector<Rug>& GetRugs() const { return mRugs; }
    const std::vector<Node>& GetNodes() const { return mNodes; }
    const std::vector<Link>& GetLinks() const { return mLinks; }
    ImVec2 GetNodePos(NodeIndex nodeIndex) const;
    bool IsIOUsed(NodeIndex nodeIndex, int slotIndex, bool forOutput) const;
    bool IsIOPinned(NodeIndex nodeIndex, size_t io, bool forOutput) const;
    bool IsParameterPinned(NodeIndex nodeIndex, size_t parameterIndex) const;
    const std::vector<AnimTrack>& GetAnimTrack() const { return mAnimTrack; }
    void GetKeyedParameters(int frame, uint32_t nodeIndex, std::vector<bool>& keyed) const;
    const std::vector<uint32_t> GetParameterPins() const;
    const std::vector<uint32_t> GetIOPins() const;
    const std::vector<MultiplexInput> GetMultiplexInputs() const;
    const ParameterBlock& GetParameterBlock(NodeIndex nodeIndex) const;
    const Samplers& GetSamplers(NodeIndex nodeIndex) const { return mNodes[nodeIndex].mSamplers; }
    ImRect GetNodesDisplayRect() const;
    ImRect GetFinalNodeDisplayRect(const std::vector<size_t>& orderList) const;
    bool RecurseIsLinked(int from, int to) const;
    NodeIndex GetMultiplexed(NodeIndex nodeIndex, size_t slotIndex) const { return mNodes[nodeIndex].mMultiplexInput.mInputs[slotIndex]; }
    bool GetMultiplexedInputs(const std::vector<Input>& inputs, NodeIndex nodeIndex, SlotIndex slotIndex, std::vector<NodeIndex>& list) const;
    AnimTrack* GetAnimTrack(uint32_t nodeIndex, uint32_t parameterIndex);
    void GetStartEndFrame(int& startFrame, int& endFrame) const { startFrame = mStartFrame; endFrame = mEndFrame; }
    void GetStartEndFrame(NodeIndex nodeIndex, int& startFrame, int& endFrame) const { startFrame = mNodes[nodeIndex].mStartFrame; endFrame = mNodes[nodeIndex].mEndFrame; }
    void GetInputs(std::vector<Input>& multiplexedInPuts, std::vector<Input>& directInputs) const;
    NodeIndex GetNodeIndex(RuntimeId runtimeUniqueId) const;

    // dirty
    const std::vector<DirtyList>& GetDirtyList() const { return mDirtyList; }
    void ClearDirtyList() { mDirtyList.clear(); }

    // clipboard
    void CopySelectedNodes();
    void CutSelectedNodes();
    void PasteNodes(ImVec2 viewOffsetPosition);
    bool IsClipboardEmpty() const;

    
private:

    // ser datas
    int mStartFrame, mEndFrame;
    std::vector<Node> mNodes;
    std::vector<Link> mLinks;
    std::vector<Rug> mRugs;
    std::vector<AnimTrack> mAnimTrack;

    // non ser data / runtime datas
    std::vector<Node> mNodesClipboard;
    std::vector<DirtyList> mDirtyList;
    //int mSelectedNodeIndex;

    // undo and transaction
    bool mbTransaction;
    UndoRedo* mUndoRedo;

	void SetDirty(NodeIndex nodeIndex, Dirty::Type flags, SlotIndex slotIndex = {InvalidSlotIndex}) 
	{ 
		if (flags == Dirty::Input)
		{
			assert(slotIndex != -1);
		}
		mDirtyList.push_back({nodeIndex, slotIndex, flags});
	}
    void AddNodeHelper(int nodeIndex);

    void DelLinkInternal(size_t linkIndex);
    void AddLinkInternal(NodeIndex inputNodeIndex, SlotIndex inputSlotIndex, NodeIndex outputNodeIndex, SlotIndex outputSlotIndex);
    void RemoveAnimation(NodeIndex nodeIndex);

    void GetMultiplexedInputs(const std::vector<Input>& inputs, NodeIndex nodeIndex, std::set<NodeIndex>& list) const;

    // layout
    struct NodePosition
    {
        int mLayer;
        int mStackIndex;
        int mNodeIndex; // used for sorting

        bool operator <(const NodePosition& other) const
        {
            if (mLayer < other.mLayer)
            {
                return true;
            }
            if (mLayer > other.mLayer)
            {
                return false;
            }
            if (mStackIndex<other.mStackIndex)
            {
                return true;
            }
            return false;
        }
    };

    void RecurseNodeGraphLayout(std::vector<NodePosition>& positions,
        std::map<int, int>& stacks,
        size_t currentIndex,
        int currentLayer);
    bool HasSelectedNodes() const;


};