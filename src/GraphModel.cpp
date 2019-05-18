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


GraphModel::GraphModel()
    : mbTransaction(false), mUndoRedoHandler(new UndoRedoHandler), mSelectedNodeIndex(-1), mUndoRedo(nullptr)
{
}

GraphModel::~GraphModel()
{
    assert(!mbTransaction);
    delete mUndoRedoHandler;
}

void GraphModel::Clear()
{
    assert(!mbTransaction);

    mNodes.clear();
    mLinks.clear();
    mRugs.clear();
    mUndoRedoHandler->Clear();
    mSelectedNodeIndex = -1;
    mEvaluationStages.Clear();
}

void GraphModel::BeginTransaction(bool undoable)
{
    assert(!mbTransaction);

    mbTransaction = true;
    if (undoable)
    {
        mUndoRedo = new URDummy;
    }
}

bool GraphModel::InTransaction() const
{
    return mbTransaction;
}

void GraphModel::EndTransaction()
{
    assert(mbTransaction);
    mbTransaction = false;
    delete mUndoRedo;
    mUndoRedo = nullptr;
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

void GraphModel::MoveSelectedNodes(const ImVec2 delta)
{
    assert(mbTransaction);
    for (auto& node : mNodes)
    {
        if (!node.mbSelected)
        {
            continue;
        }
        node.mPos += delta;
    }
}

void GraphModel::SetNodePosition(size_t nodeIndex, const ImVec2 position)
{
    assert(mbTransaction);
    mNodes[nodeIndex].mPos = position;
}

size_t GraphModel::AddNode(size_t type, ImVec2 position)
{
    assert(mbTransaction);

    // size_t index = nodes.size();
    mNodes.push_back(Node(type, position));
    mEvaluationStages.AddSingleEvaluation(type);

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

    assert(outputNodeIndex < mEvaluationStages.mStages.size());

    Link nl;
    nl.mInputIdx = inputNodeIndex;
    nl.mInputSlot = inputSlotIndex;
    nl.mOutputIdx = outputNodeIndex;
    nl.mOutputSlot = outputSlotIndex;
    mLinks.push_back(nl);


    mEvaluationStages.AddEvaluationInput(outputNodeIndex, outputSlotIndex, inputNodeIndex);
    mEvaluationStages.SetIOPin(inputNodeIndex, inputSlotIndex, true, false);
    mEvaluationStages.SetIOPin(outputNodeIndex, outputSlotIndex, false, false);
}

void GraphModel::DelLink(size_t nodeIndex, size_t slotIndex)
{
    assert(mbTransaction);

    mEvaluationStages.DelEvaluationInput(nodeIndex, slotIndex);
}

void GraphModel::AddRug(const Rug& rug)
{
    assert(mbTransaction);

    mRugs.push_back(rug);
}

void GraphModel::DelRug(size_t rugIndex)
{
    assert(mbTransaction);
}

void GraphModel::SetRug(size_t rugIndex, const Rug& rug)
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
                                    [this]() { return &mNodes; },
                                    [this](int index) {
                                        // recompute link indices
                                        for (int id = 0; id < mLinks.size(); id++)
                                        {
                                            if (mLinks[id].mInputIdx > index)
                                                mLinks[id].mInputIdx--;
                                            if (mLinks[id].mOutputIdx > index)
                                                mLinks[id].mOutputIdx--;
                                        }
                                        // NodeGraphUpdateEvaluationOrder(controler); todo
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

                                        // NodeGraphUpdateEvaluationOrder(controler); todo
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
                URDel<Link> undoRedoDelNodeLink(
                    int(i),
                    [this]() { return &mLinks; },
                    [this](int index) {
                        Link& link = mLinks[index];
                        DelLink(link.mOutputIdx, link.mOutputSlot);
                    },
                    [this](int index) {
                        Link& link = mLinks[index];
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
        // NodeGraphUpdateEvaluationOrder(controler); todo

        // inform delegate
        // controler->UserDeleteNode(selection); todo
    }
}

bool GraphModel::IsIOUsed(int nodeIndex, int slotIndex, bool forOutput) const
{
    for (auto& link : mLinks)
    {
        if ((link.mInputIdx == nodeIndex && link.mInputSlot == slotIndex && forOutput) ||
            (link.mOutputIdx == nodeIndex && link.mOutputSlot == slotIndex && !forOutput))
        {
            return true;
        }
    }
    return false;
}

void GraphModel::SelectNode(size_t nodeIndex, bool selected)
{
    // assert(mbTransaction);
    mNodes[nodeIndex].mbSelected = selected;
}

ImVec2 GraphModel::GetNodePos(size_t nodeIndex) const
{
    return mNodes[nodeIndex].mPos;
}

void GraphModel::SetSamplers(size_t nodeIndex, const std::vector<InputSampler>& samplers)
{
    assert(mbTransaction);
    mEvaluationStages.SetSamplers(nodeIndex, samplers);
}

void GraphModel::SetEvaluationOrder(const std::vector<size_t>& nodeOrderList)
{
    assert(!mbTransaction);
    mEvaluationStages.SetEvaluationOrder(nodeOrderList);
}

bool GraphModel::NodeHasUI(size_t nodeIndex) const
{
    if (mEvaluationStages.mStages.size() <= nodeIndex)
        return false;
    return gMetaNodes[mEvaluationStages.mStages[nodeIndex].mType].mbHasUI;
}

bool GraphModel::IsIOPinned(size_t nodeIndex, size_t io, bool forOutput) const
{
    return mEvaluationStages.IsIOPinned(nodeIndex, io, forOutput);
}

bool GraphModel::IsParameterPinned(size_t nodeIndex, size_t parameterIndex) const
{
    return mEvaluationStages.IsParameterPinned(nodeIndex, parameterIndex);
}

void GraphModel::SetParameter(int nodeIndex, const std::string& parameterName, const std::string& parameterValue)
{
    assert(mbTransaction);
    if (nodeIndex < 0 || nodeIndex >= mEvaluationStages.mStages.size())
    {
        return;
    }
    size_t nodeType = mEvaluationStages.mStages[nodeIndex].mType;
    int parameterIndex = GetParameterIndex(nodeType, parameterName.c_str());
    if (parameterIndex == -1)
    {
        return;
    }
    ConTypes parameterType = GetParameterType(nodeType, parameterIndex);
    size_t paramOffset = GetParameterOffset(nodeType, parameterIndex);
    ParseStringToParameter(
        parameterValue, parameterType, &mEvaluationStages.mStages[nodeIndex].mParameters[paramOffset]);
    // mEditingContext.SetTargetDirty(nodeIndex, Dirty::Parameter);
}

AnimTrack* GraphModel::GetAnimTrack(uint32_t nodeIndex, uint32_t parameterIndex)
{
    for (auto& animTrack : mEvaluationStages.mAnimTrack)
    {
        if (animTrack.mNodeIndex == nodeIndex && animTrack.mParamIndex == parameterIndex)
            return &animTrack;
    }
    return NULL;
}

void GraphModel::MakeKey(int frame, uint32_t nodeIndex, uint32_t parameterIndex)
{
    assert(mbTransaction);
    if (nodeIndex == -1)
    {
        return;
    }
    // URDummy urDummy;

    AnimTrack* animTrack = GetAnimTrack(nodeIndex, parameterIndex);
    if (!animTrack)
    {
        // URAdd<AnimTrack> urAdd(int(mEvaluationStages.mAnimTrack.size()), [&] { return &mEvaluationStages.mAnimTrack;
        // });
        uint32_t parameterType = gMetaNodes[mEvaluationStages.mStages[nodeIndex].mType].mParams[parameterIndex].mType;
        AnimTrack newTrack;
        newTrack.mNodeIndex = nodeIndex;
        newTrack.mParamIndex = parameterIndex;
        newTrack.mValueType = parameterType;
        newTrack.mAnimation = AllocateAnimation(parameterType);
        mEvaluationStages.mAnimTrack.push_back(newTrack);
        animTrack = &mEvaluationStages.mAnimTrack.back();
    }
    /*URChange<AnimTrack> urChange(int(animTrack - mEvaluationStages.mAnimTrack.data()),
                                 [&](int index) { return &mEvaluationStages.mAnimTrack[index]; });*/
    EvaluationStage& stage = mEvaluationStages.mStages[nodeIndex];
    size_t parameterOffset = GetParameterOffset(uint32_t(stage.mType), parameterIndex);
    animTrack->mAnimation->SetValue(frame, &stage.mParameters[parameterOffset]);
}

void GraphModel::GetKeyedParameters(int frame, uint32_t nodeIndex, std::vector<bool>& keyed) const
{
}

void GraphModel::SetIOPin(size_t nodeIndex, size_t io, bool forOutput, bool pinned)
{
    assert(mbTransaction);
    mEvaluationStages.SetIOPin(nodeIndex, io, forOutput, pinned);
}

void GraphModel::SetParameterPin(size_t nodeIndex, size_t parameterIndex, bool pinned)
{
    assert(mbTransaction);
    mEvaluationStages.SetParameterPin(nodeIndex, parameterIndex, pinned);
}

void GraphModel::SetNodeParameter(size_t nodeIndex, const std::vector<unsigned char>& parameters)
{
    assert(mbTransaction);
    auto& stage = mEvaluationStages.mStages[nodeIndex];
    stage.mParameters = parameters;
    mEvaluationStages.SetEvaluationParameters(nodeIndex, parameters);
    // mModel.mEvaluationStages.SetSamplers(nodeIndex, stage.mInputSamplers);
    // mEditingContext.SetTargetDirty(index, Dirty::Parameter);
}

void GraphModel::SetTimeSlot(size_t nodeIndex, int frameStart, int frameEnd)
{
    assert(mbTransaction);

    auto& stage = mEvaluationStages.mStages[nodeIndex];
    stage.mStartFrame = frameStart;
    stage.mEndFrame = frameEnd;
}

void GraphModel::SetKeyboardMouse(
    size_t nodeIndex, float rx, float ry, bool lButDown, bool rButDown, bool bCtrl, bool bAlt, bool bShift)
{
    mEvaluationStages.SetKeyboardMouse(
        nodeIndex, rx, ry, lButDown, rButDown, bCtrl, bAlt, bShift);
}


void GraphModel::CopySelectedNodes()
{
    mStagesClipboard.clear();
    mNodesClipboard.clear();
    for (auto i = 0; i < mNodes.size(); i++)
    {
        if (!mNodes[i].mbSelected)
        {
            continue;
        }
        mStagesClipboard.push_back(mEvaluationStages.mStages[i]);
        mNodesClipboard.push_back(mNodes[i]);
    }
}

void GraphModel::CutSelectedNodes()
{
    mStagesClipboard.clear();
}

bool GraphModel::IsClipboardEmpty() const
{
    return mStagesClipboard.empty();
}

void GraphModel::PasteNodes()
{
    /*for (auto& sourceNode : mStagesClipboard) todo
    {
        URAdd<EvaluationStage> undoRedoAddNode(int(mEvaluationStages.mStages.size()),
                                               [&]() { return &mEvaluationStages.mStages; },
                                               [](int) {},
                                               [&](int index) { NodeIsAdded(index); });

        mEditingContext.UserAddStage();
        size_t target = mEvaluationStages.mStages.size();
        //AddSingleNode(sourceNode.mType); todo

        auto& stage = mEvaluationStages.mStages.back();
        stage.mParameters = sourceNode.mParameters;
        stage.mInputSamplers = sourceNode.mInputSamplers;
        stage.mStartFrame = sourceNode.mStartFrame;
        stage.mEndFrame = sourceNode.mEndFrame;

        mEvaluationStages.SetEvaluationParameters(target, stage.mParameters);
        mEvaluationStages.SetSamplers(target, stage.mInputSamplers);
        mEvaluationStages.SetTime(&mEditingContext, mEditingContext.GetCurrentTime(), true);
        mEditingContext.SetTargetDirty(target, Dirty::All);
    }*/

            // URDummy undoRedoDummy;
    /*
    ImVec2 min(FLT_MAX, FLT_MAX);
    for (auto& clipboardNode : mNodesClipboard)
    {
        min.x = ImMin(clipboardNode.mPos.x, min.x);
        min.y = ImMin(clipboardNode.mPos.y, min.y);
    }
    for (auto& selnode : nodes)
        selnode.mbSelected = false;
    for (auto& clipboardNode : mNodesClipboard)
    {
        // URAdd<Node> undoRedoAddRug(int(nodes.size()), []() { return &nodes; }, [](int index) {}, [](int index)
        // {});
        nodes.push_back(clipboardNode);
        nodes.back().mPos += (io.MousePos - offset) / factor - min;
        nodes.back().mbSelected = true;
    }
    */
}
