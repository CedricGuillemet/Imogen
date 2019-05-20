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
#include <string>
#include "imgui.h"
#include "imgui_internal.h"
#include "Library.h"

struct NodeGraphControlerBase
{
    NodeGraphControlerBase() : mSelectedNodeIndex(-1)
    {
    }

    int mSelectedNodeIndex;

    virtual unsigned int GetNodeTexture(size_t index) = 0;
    virtual ImVec2 GetEvaluationSize(size_t index) const = 0;

    virtual int NodeIsProcesing(size_t nodeIndex) const = 0;
    virtual float NodeProgress(size_t nodeIndex) const = 0;
    virtual bool NodeIsCubemap(size_t nodeIndex) const = 0;
    virtual bool NodeIs2D(size_t nodeIndex) const = 0;
    virtual bool NodeIsCompute(size_t nodeIndex) const = 0;
    virtual void DrawNodeImage(ImDrawList* drawList, const ImRect& rc, const ImVec2 marge, const size_t nodeIndex) = 0;
    virtual void ContextMenu(ImVec2 offset, int nodeHovered) = 0;
    // return false if background must be rendered by node graph
    virtual bool RenderBackground() = 0;

};

class GraphModel;
void NodeGraph(GraphModel* model, NodeGraphControlerBase* delegate, bool enabled);
void NodeGraphClear(); // delegate is not called

void NodeGraphUpdateScrolling(GraphModel* model);
void NodeGraphLayout(GraphModel* model);

