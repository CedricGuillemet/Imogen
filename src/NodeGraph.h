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
#include <vector>
#include <stdint.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "Library.h"

struct NodeGraphControlerBase
{
    NodeGraphControlerBase() : mSelectedNodeIndex(-1), mCategories(nullptr)
    {
    }

    int mSelectedNodeIndex;
    const std::vector<std::string>* mCategories;

    //virtual void UpdateEvaluationList(const std::vector<size_t> nodeOrderList) = 0;
    //virtual void AddLink(int InputIdx, int InputSlot, int OutputIdx, int OutputSlot) = 0;
    //virtual void DelLink(int index, int slot) = 0;
    virtual unsigned int GetNodeTexture(size_t index) = 0;
    // A new node has been added in the graph. Do a push_back on your node array
    // add node for batch(loading graph)
    //virtual void AddSingleNode(size_t type) = 0;
    // add  by user interface
    //virtual void UserAddNode(size_t type) = 0;
    // node deleted
    //virtual void UserDeleteNode(size_t index) = 0;
    virtual ImVec2 GetEvaluationSize(size_t index) const = 0;

    virtual void SetParamBlock(size_t index, const std::vector<unsigned char>& paramBlock) = 0;
    virtual void SetTimeSlot(size_t index, int frameStart, int frameEnd) = 0;
    //virtual bool NodeHasUI(size_t nodeIndex) const = 0;
    virtual int NodeIsProcesing(size_t nodeIndex) const = 0;
    virtual float NodeProgress(size_t nodeIndex) const = 0;
    virtual bool NodeIsCubemap(size_t nodeIndex) const = 0;
    virtual bool NodeIs2D(size_t nodeIndex) const = 0;
    virtual bool NodeIsCompute(size_t nodeIndex) const = 0;
    virtual void DrawNodeImage(ImDrawList* drawList, const ImRect& rc, const ImVec2 marge, const size_t nodeIndex) = 0;
    // return false if background must be rendered by node graph
    virtual bool RenderBackground() = 0;

    // clipboard
    virtual void CopyNodes(const std::vector<size_t> nodes) = 0;
    virtual void CutNodes(const std::vector<size_t> nodes) = 0;
    virtual void PasteNodes() = 0;
};

class GraphModel;
void NodeGraph(GraphModel* model, NodeGraphControlerBase* delegate, bool enabled);
void NodeGraphClear(); // delegate is not called


void NodeGraphUpdateEvaluationOrder(GraphModel* model, NodeGraphControlerBase* delegate);
void NodeGraphUpdateScrolling(GraphModel* model);
void NodeGraphLayout(GraphModel* model);


ImRect GetFinalNodeDisplayRect(GraphModel* model);
ImRect GetNodesDisplayRect(GraphModel* model);
