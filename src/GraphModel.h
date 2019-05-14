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

struct UndoRedoHandler;



class GraphModel
{
public:
    GraphModel();
    ~GraphModel();

		
	struct NodeRug
    {
        ImVec2 mPos, mSize;
        uint32_t mColor;
        std::string mText;
    };


    struct Node
    {
        Node(int type, ImVec2 position) : mType(type), mPos(position), mbSelected(false)
        {
        }
        Node() : mbSelected(false)
        {
        }
        int mType;
        ImVec2 mPos;
        bool mbSelected;
    };

    struct NodeLink
    {
        int mInputIdx, mInputSlot, mOutputIdx, mOutputSlot;
    };


	// Transaction
	void Clear();
	void BeginTransaction(bool undoable);
    void EndTransaction();

	// undo/redo

	void Undo();
    void Redo();

	// setters
    int AddNode(size_t type, ImVec2 position);
    void DelNode(size_t nodeIndex);
    void DeleteSelectedNodes();
    void AddLink(size_t inputNodeIndex, size_t inputSlotIndex, size_t outputNodeIndex, size_t outputSlotIndex);
    void DelLink(size_t index, size_t slot);
    void AddRug(ImVec2 position, ImVec2 size, uint32_t color, const std::string& comment);
    void DelRug(size_t rugIndex);
    void SetRug(size_t rugIndex, const NodeRug& rug);


	// getters

	// linked to runtime...not sure
    bool NodeIsCubemap(size_t nodeIndex) const;
    bool NodeIs2D(size_t nodeIndex) const;
    bool NodeIsCompute(size_t nodeIndex) const;
    bool NodeHasUI(size_t nodeIndex) const;
    const std::vector<NodeRug>& GetRugs() const { return mRugs; }
    const std::vector<Node>& GetNodes() const
    {
        return mNodes;
    }
    const std::vector<NodeLink>& GetLinks() const
    {
        return mLinks;
    }
	bool IsIOUsed(int nodeIndex, int slotIndex, bool forOutput) const;

	// clipboard
    void CopyNodes(const std::vector<size_t> nodes);
    void CutNodes(const std::vector<size_t> nodes);
    void PasteNodes();




private:
    bool mbTransaction;
    UndoRedoHandler* mUndoRedoHandler;

	int mSelectedNodeIndex;
	static std::vector<Node> mNodes;
    static std::vector<NodeLink> mLinks;
    static std::vector<NodeRug> mRugs;
};