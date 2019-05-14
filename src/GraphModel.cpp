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


GraphModel::GraphModel() : mbTransaction(false), mUndoRedoHandler(new UndoRedoHandler), mSelectedNodeIndex(-1)
{
}

GraphModel::~GraphModel()
{
    delete mUndoRedoHandler;
}

void GraphModel::Clear()
{
    mNodes.clear();
    mLinks.clear();
    mRugs.clear();
    mUndoRedoHandler->Clear();
    mSelectedNodeIndex = -1;
}

void GraphModel::BeginTransaction(bool undoable)
    {
    assert(!mbTransaction);

    mbTransaction = true;
}

void GraphModel::EndTransaction()
{
    assert(mbTransaction);
    mbTransaction = false;
}

void GraphModel::Undo()
{
    assert(!mbTransaction);
    mUndoRedoHandler->Undo();
}

void GraphModel::Redo()
{
    assert(!mbTransaction);
    mUndoRedoHandler->Redo();
}


int GraphModel::AddNode(size_t type, ImVec2 position)
{
    assert(mbTransaction);

    // size_t index = nodes.size();
    mNodes.push_back(Node(type, position));

	return mNodes.size() - 1;
}
void GraphModel::DelNode(size_t nodeIndex)
{
    assert(mbTransaction);
}
void GraphModel::AddLink(size_t inputNodeIndex, size_t inputSlotIndex, size_t outputNodeIndex, size_t outputSlotIndex)
{
    assert(mbTransaction);

    if (inputNodeIndex >= mNodes.size() || outputNodeIndex >= mNodes.size())
    {
        // Log("Error : Link node index doesn't correspond to an existing node.");
        return;
    }

    NodeLink nl;
    nl.mInputIdx = inputNodeIndex;
    nl.mInputSlot = inputSlotIndex;
    nl.mOutputIdx = outputNodeIndex;
    nl.mOutputSlot = outputSlotIndex;
    mLinks.push_back(nl);
}
void GraphModel::DelLink(size_t index, size_t slot)
{
    assert(mbTransaction);
}
void GraphModel::AddRug(ImVec2 position, ImVec2 size, uint32_t color, const std::string& comment)
{
    assert(mbTransaction);

    mRugs.push_back({position, size, color, comment});
}
void GraphModel::DelRug(size_t rugIndex)
{
    assert(mbTransaction);
}

void GraphModel::SetRug(size_t rugIndex, const NodeRug& rug)
{
    assert(mbTransaction);
    mRugs[rugIndex] = rug;
}

void GraphModel::DeleteSelectedNodes()
{
    URDummy urDummy;
    for (int selection = int(mNodes.size()) - 1; selection >= 0; selection--)
    {
        if (!mNodes[selection].mbSelected)
            continue;
        URDel<Node> undoRedoDelNode(int(selection),
                                    []() { return &mNodes; },
                                    [this](int index) {
                                        // recompute link indices
                                        for (int id = 0; id < mLinks.size(); id++)
                                        {
                                            if (mLinks[id].mInputIdx > index)
                                                mLinks[id].mInputIdx--;
                                            if (mLinks[id].mOutputIdx > index)
                                                mLinks[id].mOutputIdx--;
                                        }
                                        //NodeGraphUpdateEvaluationOrder(controler); todo
                                        mSelectedNodeIndex = -1;
                                    },
                                    [this](int index) {
                                        // recompute link indices
                                        for (int id = 0; id < mLinks.size(); id++)
                                        {
                                            if (mLinks[id].mInputIdx >= index)
                                                mLinks[id].mInputIdx++;
                                            if (mLinks[id].mOutputIdx >= index)
                                                mLinks[id].mOutputIdx++;
                                        }

                                        //NodeGraphUpdateEvaluationOrder(controler); todo
                                        mSelectedNodeIndex = -1;
                                    });

        for (int id = 0; id < mLinks.size(); id++)
        {
            if (mLinks[id].mInputIdx == selection || mLinks[id].mOutputIdx == selection)
                DelLink(mLinks[id].mOutputIdx, mLinks[id].mOutputSlot);
        }
        // auto iter = links.begin();
        for (size_t i = 0; i < mLinks.size();)
        {
            auto& link = mLinks[i];
            if (link.mInputIdx == selection || link.mOutputIdx == selection)
            {
                URDel<NodeLink> undoRedoDelNodeLink(
                    int(i),
                    [this]() { return &mLinks; },
                    [this](int index) {
                        NodeLink& link = mLinks[index];
                        DelLink(link.mOutputIdx, link.mOutputSlot);
                    },
                    [this](int index) {
                        NodeLink& link = mLinks[index];
                        AddLink(link.mInputIdx, link.mInputSlot, link.mOutputIdx, link.mOutputSlot);
                    });

                mLinks.erase(mLinks.begin() + i);
            }
            else
            {
                i++;
            }
        }

        // recompute link indices
        for (int id = 0; id < mLinks.size(); id++)
        {
            if (mLinks[id].mInputIdx > selection)
                mLinks[id].mInputIdx--;
            if (mLinks[id].mOutputIdx > selection)
                mLinks[id].mOutputIdx--;
        }

        // delete links
        mNodes.erase(mNodes.begin() + selection);
        //NodeGraphUpdateEvaluationOrder(controler); todo

        // inform delegate
        //controler->UserDeleteNode(selection); todo
    }
}
bool GraphModel::IsIOUsed(int nodeIndex, int slotIndex, bool forOutput) const
{
    /*for (auto& link : mLinks)
    {
        if ((link.InputIdx == nodeIndex && link.InputSlot == slotIndex && forOutput) ||
            (link.OutputIdx == nodeIndex && link.OutputSlot == slotIndex && !forOutput))
        {
            return true;
        }
    }
        */
    return false;
}
