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

#pragma once
#include <vector>
#include <stdint.h>
struct NodeGraphDelegate
{
	NodeGraphDelegate() : mSelectedNodeIndex(-1), mBakeTargetIndex(-1), mCategoriesCount(0), mCategories(0)
	{}

	int mSelectedNodeIndex;
	int mBakeTargetIndex;
	int mCategoriesCount;
	const char ** mCategories;

	virtual void UpdateEvaluationList(const std::vector<size_t> nodeOrderList) = 0;
	virtual void AddLink(int InputIdx, int InputSlot, int OutputIdx, int OutputSlot) = 0;
	virtual void DelLink(int index, int slot) = 0;
	//virtual void EditNode(size_t index) = 0;
	virtual unsigned int GetNodeTexture(size_t index) = 0;
	//virtual void EditNode(size_t index) = 0;
	virtual bool AuthorizeConnexion(int typeA, int typeB) = 0;
	// A new node has been added in the graph. Do a push_back on your node array
	virtual void AddNode(size_t type) = 0;
	// node deleted
	virtual void DeleteNode(size_t index) = 0;

	virtual void DoForce() = 0;
	virtual unsigned char *GetParamBlock(size_t index, size_t& paramBlockSize) = 0;
	virtual void SetParamBlock(size_t index, unsigned char* paramBlock) = 0;
	virtual bool NodeHasUI(size_t nodeIndex) = 0;
	static const int MaxCon = 32;
	struct Con
	{
		const char *mName;
		int mType;
		float mRangeMinX, mRangeMaxX;
		float mRangeMinY, mRangeMaxY;
		bool mbRelative;
		const char* mEnumList;
	};
	struct MetaNode
	{
		const char *mName;
		uint32_t mHeaderColor;
		int mCategory;
		Con mInputs[MaxCon];
		Con mOutputs[MaxCon];
		Con mParams[MaxCon];
		bool mbHasUI;
	};
	virtual const MetaNode* GetMetaNodes(int &metaNodeCount) = 0;
};

struct Node
{
	int     mType;
	ImVec2  Pos, Size;
	int     InputsCount, OutputsCount;

	Node(int type, const ImVec2& pos, const NodeGraphDelegate::MetaNode* metaNodes);

	ImVec2 GetInputSlotPos(int slot_no) const { return ImVec2(Pos.x, Pos.y + Size.y * ((float)slot_no + 1) / ((float)InputsCount + 1)); }
	ImVec2 GetOutputSlotPos(int slot_no) const { return ImVec2(Pos.x + Size.x, Pos.y + Size.y * ((float)slot_no + 1) / ((float)OutputsCount + 1)); }
};
struct NodeLink
{
	int     InputIdx, InputSlot, OutputIdx, OutputSlot;
	NodeLink() {}
	NodeLink(int input_idx, int input_slot, int output_idx, int output_slot) { InputIdx = input_idx; InputSlot = input_slot; OutputIdx = output_idx; OutputSlot = output_slot; }
	bool operator == (const NodeLink& other) const
	{
		return InputIdx == other.InputIdx && InputSlot == other.InputSlot && OutputIdx == other.OutputIdx && OutputSlot == other.OutputSlot;
	}
};

struct UndoRedo
{
	virtual ~UndoRedo() {}
	virtual void Undo() = 0;
	virtual void Redo() = 0;
};

struct UndoRedoHandler
{
	void Undo()
	{
		if (mUndos.empty())
			return;
		mUndos.back()->Undo();
		mRedos.push_back(mUndos.back());
		mUndos.pop_back();
	}

	void Redo()
	{
		if (mRedos.empty())
			return;
		mRedos.back()->Redo();
		mUndos.push_back(mRedos.back());
		mRedos.pop_back();
	}

	void AddUndo(UndoRedo *undoRedo)
	{
		mUndos.push_back(undoRedo);
		for (auto redo : mRedos)
			delete redo;
		mRedos.clear();
	}

private:
	std::vector<UndoRedo *> mUndos;
	std::vector<UndoRedo *> mRedos;
};

extern UndoRedoHandler undoRedoHandler;

void NodeGraph(NodeGraphDelegate *delegate, bool enabled);
void NodeGraphClear(); // delegate is not called
const std::vector<NodeLink> NodeGraphGetLinks();
ImVec2 NodeGraphGetNodePos(size_t index);

void NodeGraphAddNode(NodeGraphDelegate *delegate, int type, void *parameters, int posx, int posy);
void NodeGraphAddLink(NodeGraphDelegate *delegate, int InputIdx, int InputSlot, int OutputIdx, int OutputSlot);
void NodeGraphUpdateEvaluationOrder(NodeGraphDelegate *delegate);
void NodeGraphUpdateScrolling();