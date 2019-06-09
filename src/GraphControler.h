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

#include "GraphEditor.h"
#include "EvaluationStages.h"
#include "EvaluationContext.h"
#include "GraphModel.h"

struct GraphControler : public GraphEditorDelegate
{
    GraphControler();

    void Clear();

    void SetKeyboardMouse(const UIInput& input, bool bValidInput);

    // accessors
    virtual int NodeIsProcesing(size_t nodeIndex) const { return mEditingContext.StageIsProcessing(nodeIndex); }
    virtual float NodeProgress(size_t nodeIndex) const { return mEditingContext.StageGetProgress(nodeIndex); }
    virtual bool NodeIsCubemap(size_t nodeIndex) const;
    virtual bool NodeIs2D(size_t nodeIndex) const;
    virtual bool NodeIsCompute(size_t nodeIndex) const;
    virtual ImVec2 GetEvaluationSize(size_t nodeIndex) const;
    virtual bool RecurseIsLinked(int from, int to) const { return mModel.RecurseIsLinked(from, to); }
    virtual bool IsIOPinned(size_t nodeIndex, size_t io, bool forOutput) const { return mModel.IsIOPinned(nodeIndex, io, forOutput); }
    virtual unsigned int GetBitmapInfo(size_t nodeIndex) const;

    // operations
    virtual bool InTransaction() { return mModel.InTransaction(); }
    virtual void BeginTransaction(bool undoable) { mModel.BeginTransaction(undoable); }
    virtual void EndTransaction() { mModel.EndTransaction(); }

    virtual void DelRug(size_t rugIndex) { mModel.DelRug(rugIndex); }
    virtual void SelectNode(size_t nodeIndex, bool selected) { mModel.SelectNode(nodeIndex, selected); }
    virtual void MoveSelectedNodes(const ImVec2 delta) { mModel.MoveSelectedNodes(delta); }

    virtual void AddLink(size_t inputNodeIndex, size_t inputSlotIndex, size_t outputNodeIndex, size_t outputSlotIndex) { mModel.AddLink(inputNodeIndex, inputSlotIndex, outputNodeIndex, outputSlotIndex); }
    virtual void DelLink(size_t linkIndex) { mModel.DelLink(linkIndex); }

    virtual void SetRug(size_t rugIndex, const ImRect& rect, const char *szText, uint32_t color) { mModel.SetRug(rugIndex, GraphModel::Rug{rect.Min, rect.GetSize(), color, std::string(szText)}); }

    // accessors
    virtual const std::vector<Node>& GetNodes() const override  { return mNodes; }
    virtual const std::vector<Rug>& GetRugs()   const override  { return mRugs;  }
    virtual const std::vector<Link>& GetLinks() const override  { return mLinks; }



    // UI
    void NodeEdit();
    virtual void DrawNodeImage(ImDrawList* drawList, const ImRect& rc, const ImVec2 marge, const size_t nodeIndex);
    virtual bool RenderBackground();
    virtual void ContextMenu(ImVec2 offset, int nodeHovered);

    EvaluationContext mEditingContext;
    EvaluationStages mEvaluationStages;
    int mBackgroundNode;
    

    /*EvaluationStage* Get(ASyncId id)
    {
        return GetByAsyncId(id, mModel.mEvaluationStages.mStages);
    }
    */
    void ApplyDirtyList();
    GraphModel mModel;

protected:
    bool mbMouseDragging;
    bool mbUsingMouse;
    
    std::vector<Node> mNodes;
    std::vector<Rug> mRugs;
    std::vector<Link> mLinks;

    bool EditSingleParameter(unsigned int nodeIndex,
                             unsigned int parameterIndex,
                             void* paramBuffer,
                             const MetaParameter& param);
    void PinnedEdit();
    void EditNodeParameters();
    void HandlePin(size_t nodeIndex, size_t parameterIndex);
    void HandlePinIO(size_t nodeIndex, size_t slotIndex, bool forOutput);
    int ShowMultiplexed(const std::vector<size_t>& inputs, int currentMultiplexedOveride);
    void ComputeGraphArrays();
};
