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

#include "NodeGraph.h"
#include "EvaluationStages.h"
#include "ImCurveEdit.h"
#include "ImGradient.h"
#include "Library.h"
#include "EvaluationContext.h"

struct NodeGraphControler : public NodeGraphControlerBase
{
    NodeGraphControler();

    void Clear();

    virtual void AddSingleNode(size_t type);
    virtual void UserAddNode(size_t type);
    virtual void AddLink(int InputIdx, int InputSlot, int OutputIdx, int OutputSlot) { mEvaluationStages.AddEvaluationInput(OutputIdx, OutputSlot, InputIdx); mEditingContext.SetTargetDirty(OutputIdx); }
    virtual void DelLink(int index, int slot) { mEvaluationStages.DelEvaluationInput(index, slot); mEditingContext.SetTargetDirty(index); }
    virtual void UserDeleteNode(size_t index);
    virtual void SetParamBlock(size_t index, const std::vector<unsigned char>& parameters);

    virtual unsigned int GetNodeTexture(size_t index) { return mEditingContext.GetEvaluationTexture(index); }

    

    virtual void SetTimeSlot(size_t index, int frameStart, int frameEnd);
    void SetTimeDuration(size_t index, int duration);

    void InvalidateParameters();

    void SetMouse(float rx, float ry, float dx, float dy, bool lButDown, bool rButDown, float wheel);

    bool NodeHasUI(size_t nodeIndex) { return gMetaNodes[mEvaluationStages.mStages[nodeIndex].mType].mbHasUI; }
    virtual int NodeIsProcesing(size_t nodeIndex) { return mEditingContext.StageIsProcessing(nodeIndex); }
    virtual float NodeProgress(size_t nodeIndex) { return mEditingContext.StageGetProgress(nodeIndex); }
    virtual bool NodeIsCubemap(size_t nodeIndex);
    virtual bool NodeIs2D(size_t nodeIndex);
    virtual bool NodeIsCompute(size_t nodeIndex);
    virtual void UpdateEvaluationList(const std::vector<size_t> nodeOrderList) { mEvaluationStages.SetEvaluationOrder(nodeOrderList);    }
    virtual ImVec2 GetEvaluationSize(size_t nodeIndex);
    virtual void DrawNodeImage(ImDrawList *drawList, const ImRect &rc, const ImVec2 marge, const size_t nodeIndex);

    virtual void CopyNodes(const std::vector<size_t> nodes);
    virtual void CutNodes(const std::vector<size_t> nodes);
    virtual void PasteNodes();

    // animation
    const std::vector<AnimTrack>& GetAnimTrack() const { return mEvaluationStages.GetAnimTrack(); }
    

    void MakeKey(int frame, uint32_t nodeIndex, uint32_t parameterIndex);
    void GetKeyedParameters(int frame, uint32_t nodeIndex, std::vector<bool>& keyed);

    AnimTrack* GetAnimTrack(uint32_t nodeIndex, uint32_t parameterIndex);

    void PinnedEdit();


    EvaluationContext mEditingContext;
    EvaluationStages mEvaluationStages;
    std::vector<EvaluationStage> mStagesClipboard;
    bool mbMouseDragging;
    URChange<std::vector<unsigned char> > *mUndoRedoParamSetMouse;

    EvaluationStage* Get(ASyncId id) { return GetByAsyncId(id, mEvaluationStages.mStages); }
    void NodeEdit();

protected:
    bool EditSingleParameter(unsigned int nodeIndex, unsigned int parameterIndex, void *paramBuffer, const MetaParameter& param);
    void NodeIsAdded(int index);
    void UpdateDirtyParameter(int index);
    void EditNodeParameters();
    void HandlePin(uint32_t parameterPair);
};
