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
    int NodeIsProcesing(size_t nodeIndex) const override { return mEditingContext.StageIsProcessing(nodeIndex); }
    float NodeProgress(size_t nodeIndex) const override { return mEditingContext.StageGetProgress(nodeIndex); }
    bool NodeIsCubemap(size_t nodeIndex) const override;
    bool NodeIs2D(size_t nodeIndex) const override;
    bool NodeIsCompute(size_t nodeIndex) const override;
    ImVec2 GetEvaluationSize(size_t nodeIndex) const override;
    bool RecurseIsLinked(int from, int to) const override { return mModel.RecurseIsLinked(from, to); }
    bool IsIOPinned(size_t nodeIndex, size_t io, bool forOutput) const override { return mModel.IsIOPinned(nodeIndex, io, forOutput); }
    TextureHandle GetBitmapInfo(size_t nodeIndex) const override;

    // operations
    bool InTransaction() override { return mModel.InTransaction(); }
    void BeginTransaction(bool undoable) override { mModel.BeginTransaction(undoable); }
    void EndTransaction() override { mModel.EndTransaction(); }

    void DelRug(size_t rugIndex) override { mModel.DelRug(rugIndex); }
    void SelectNode(size_t nodeIndex, bool selected) override { mModel.SelectNode(nodeIndex, selected); }
    void MoveSelectedNodes(const ImVec2 delta) override { mModel.MoveSelectedNodes(delta); }

    void AddLink(size_t inputNodeIndex, size_t inputSlotIndex, size_t outputNodeIndex, size_t outputSlotIndex) override { mModel.AddLink(inputNodeIndex, inputSlotIndex, outputNodeIndex, outputSlotIndex); }
    void DelLink(size_t linkIndex) override { mModel.DelLink(linkIndex); }

    void SetRug(size_t rugIndex, const ImRect& rect, const char *szText, uint32_t color) override { mModel.SetRug(rugIndex, GraphModel::Rug{rect.Min, rect.GetSize(), color, std::string(szText)}); }

    // accessors
    const std::vector<Node>& GetNodes() const override  { return mNodes; }
    const std::vector<Rug>& GetRugs()   const override  { return mRugs;  }
    const std::vector<Link>& GetLinks() const override  { return mLinks; }



    // UI
    void NodeEdit();
    virtual void DrawNodeImage(ImDrawList* drawList, const ImRect& rc, const ImVec2 marge, const size_t nodeIndex) override;
    virtual bool RenderBackground() override;
    virtual void ContextMenu(ImVec2 rightclickPos, ImVec2 worldMousePos, int nodeHovered) override;

    EvaluationStages mEvaluationStages;
    EvaluationContext mEditingContext;
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
