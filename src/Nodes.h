#pragma once

struct NodeGraphDelegate
{
	virtual void AddLink(int InputIdx, int InputSlot, int OutputIdx, int OutputSlot) = 0;
	virtual void EditNode(size_t index) = 0;
	virtual unsigned int GetNodeTexture(size_t index) = 0;
	//virtual void EditNode(size_t index) = 0;
	virtual bool AuthorizeConnexion(int typeA, int typeB) = 0;
	// A new node has been added in the graph. Do a push_back on your node array
	virtual void AddNode(size_t type) = 0;
	// node deleted
	virtual void DeleteNode(size_t index) = 0;

	static const int MaxCon = 32;
	struct Con
	{
		const char *mName;
		int mType;
	};
	struct MetaNode
	{
		const char *mName;
		Con mInputs[MaxCon];
		Con mOutputs[MaxCon];
		Con mParams[MaxCon];
	};
	virtual const MetaNode* GetMetaNodes(int &metaNodeCount) = 0;
};

void NodeGraph(NodeGraphDelegate *delegate);