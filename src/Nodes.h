#pragma once
#include <vector>
struct NodeGraphDelegate
{
	NodeGraphDelegate() : mSelectedNodeIndex(-1), mBakeTargetIndex(-1)
	{}

	int mSelectedNodeIndex;
	int mBakeTargetIndex;

	virtual void UpdateEvaluationList(const std::vector<int> nodeOrderList) = 0;
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

	virtual unsigned char *GetParamBlock(int index, size_t& paramBlockSize) = 0;
	virtual void SetParamBlock(int index, unsigned char* paramBlock) = 0;
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
		Con mInputs[MaxCon];
		Con mOutputs[MaxCon];
		Con mParams[MaxCon];
	};
	virtual const MetaNode* GetMetaNodes(int &metaNodeCount) = 0;
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

void NodeGraph(NodeGraphDelegate *delegate);

void SaveNodes(const std::string &filename, NodeGraphDelegate *delegate);
void LoadNodes(const std::string &filename, NodeGraphDelegate *delegate);
