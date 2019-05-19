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
#include <algorithm>

extern UndoRedoHandler gUndoRedoHandler;

GraphModel::GraphModel() : mbTransaction(false), mSelectedNodeIndex(-1), mUndoRedo(nullptr)
{
}

GraphModel::~GraphModel()
{
    assert(!mbTransaction);
}

void GraphModel::Clear()
{
    assert(!mbTransaction);

    mNodes.clear();
    mLinks.clear();
    mRugs.clear();
    gUndoRedoHandler.Clear();
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
    if (mUndoRedo)
    {
        // undo/redo but nothing has been updated
        assert(mUndoRedo->HasSubUndoRedo());
    }
    delete mUndoRedo;
    mUndoRedo = nullptr;
}

void GraphModel::Undo()
{
    assert(!mbTransaction);
    gUndoRedoHandler.Undo();
}

void GraphModel::Redo()
{
    assert(!mbTransaction);
    gUndoRedoHandler.Redo();
}

void GraphModel::MoveSelectedNodes(const ImVec2 delta)
{
    assert(mbTransaction);
    for (size_t i = 0 ; i < mNodes.size(); i++)
    {
        auto& node = mNodes[i];
        if (!node.mbSelected)
        {
            continue;
        }
        auto ur = mUndoRedo
                ? std::make_unique<URChange<Node>>(int(i), [&](int index) { return &mNodes[index]; }, [](int) {})
                : nullptr;
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

    size_t nodeIndex = mNodes.size();

    // size_t index = nodes.size();
    mNodes.push_back(Node(type, position));
    mEvaluationStages.AddSingleEvaluation(type);

    /*mEvaluationStages.SetIOPin(inputNodeIndex, inputSlotIndex, true, false);
    mEvaluationStages.SetIOPin(outputNodeIndex, outputSlotIndex, false, false);
*/
    return nodeIndex;
}

void GraphModel::DelNode(size_t nodeIndex)
{
    assert(mbTransaction);
}

void GraphModel::DeleteLinkHelper(int index)
{
    const Link& link = mLinks[index];
    mEvaluationStages.DelEvaluationInput(link.mOutputNodeIndex, link.mOutputSlotIndex);
}

void GraphModel::AddLinkHelper(int index)
{
    const Link& link = mLinks[index];
    mEvaluationStages.AddEvaluationInput(link.mOutputNodeIndex, link.mOutputSlotIndex, link.mInputNodeIndex);
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

    auto delLink = [&](int index) { DeleteLinkHelper(index); };
    auto addLink = [&](int index) { AddLinkHelper(index); };

    size_t linkIndex = mLinks.size();

    auto ur = mUndoRedo ? std::make_unique<URAdd<Link>>(int(linkIndex), [&]() { return &mLinks; }, delLink, addLink)
                  : nullptr;
    Link link;
    link.mInputNodeIndex = inputNodeIndex;
    link.mInputSlotIndex = inputSlotIndex;
    link.mOutputNodeIndex = outputNodeIndex;
    link.mOutputSlotIndex = outputSlotIndex;
    mLinks.push_back(link);
    AddLinkHelper(linkIndex);
}

void GraphModel::DelLink(size_t linkIndex)
{
    assert(mbTransaction);

    auto delLink = [&](int index) { DeleteLinkHelper(index); };
    auto addLink = [&](int index) { AddLinkHelper(index); };

    auto ur = mUndoRedo ? std::make_unique<URDel<Link>>(int(linkIndex), [&]() { return &mLinks; }, addLink, delLink)
                  : nullptr;
    
    DeleteLinkHelper(linkIndex);
    mLinks.erase(mLinks.begin() + linkIndex);
}

void GraphModel::AddRug(const Rug& rug)
{
    assert(mbTransaction);

    auto ur = mUndoRedo
                  ? std::make_unique<URAdd<Rug>>(int(mRugs.size()), [&]() { return &mRugs; }, [](int) {}, [](int) {})
                  : nullptr;

    mRugs.push_back(rug);
}

void GraphModel::DelRug(size_t rugIndex)
{
    assert(mbTransaction);

    auto ur = mUndoRedo ? std::make_unique<URDel<Rug>>(int(rugIndex), [&]() { return &mRugs; }, [](int) {}, [](int) {})
                        : nullptr;

    mRugs.erase(mRugs.begin() + rugIndex);
}

void GraphModel::SetRug(size_t rugIndex, const Rug& rug)
{
    assert(mbTransaction);

    auto ur = mUndoRedo
                  ? std::make_unique<URChange<Rug>>(int(rugIndex), [&](int index) { return &mRugs[index]; }, [](int) {})
                  : nullptr;

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
                                            if (mLinks[id].mInputNodeIndex > index)
                                                mLinks[id].mInputNodeIndex--;
                                            if (mLinks[id].mOutputNodeIndex > index)
                                                mLinks[id].mOutputNodeIndex--;
                                        }
                                        // NodeGraphUpdateEvaluationOrder(controler); todo
                                        mSelectedNodeIndex = -1;
                                    },
                                    [this](int index) {
                                        // recompute link indices
                                        for (int id = 0; id < mLinks.size(); id++)
                                        {
                                            if (mLinks[id].mInputNodeIndex >= index)
                                                mLinks[id].mInputNodeIndex++;
                                            if (mLinks[id].mOutputNodeIndex >= index)
                                                mLinks[id].mOutputNodeIndex++;
                                        }

                                        // NodeGraphUpdateEvaluationOrder(controler); todo
                                        mSelectedNodeIndex = -1;
                                    });

        for (int id = 0; id < mLinks.size(); id++)
        {
            if (mLinks[id].mInputNodeIndex == selection || mLinks[id].mOutputNodeIndex == selection)
                DelLink(id);
        }
        // auto iter = links.begin();
        for (size_t i = 0; i < mLinks.size();)
        {
            auto& link = mLinks[i];
            if (link.mInputNodeIndex == selection || link.mOutputNodeIndex == selection)
            {
                URDel<Link> undoRedoDelNodeLink(
                    int(i),
                    [this]() { return &mLinks; },
                    [this](int index) {
                        DelLink(index);
                    },
                    [this](int index) {
                        Link& link = mLinks[index];
                        AddLink(link.mInputNodeIndex, link.mInputSlotIndex, link.mOutputNodeIndex, link.mOutputSlotIndex);
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
            if (mLinks[id].mInputNodeIndex > selection)
                mLinks[id].mInputNodeIndex--;
            if (mLinks[id].mOutputNodeIndex > selection)
                mLinks[id].mOutputNodeIndex--;
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
        if ((link.mInputNodeIndex == nodeIndex && link.mInputSlotIndex == slotIndex && forOutput) ||
            (link.mOutputNodeIndex == nodeIndex && link.mOutputSlotIndex == slotIndex && !forOutput))
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

    auto ur = mUndoRedo ? std::make_unique<URChange<std::vector<InputSampler>>>(
                              int(nodeIndex),
                              [&](int index) { return &mEvaluationStages.mInputSamplers[index]; },
                              [&](int index) {
                                  // auto& stage = mEvaluationStages.mStages[index];
                                  // mEvaluationStages.SetSamplers(index, stage.mInputSamplers);
                                  // mEditingContext.SetTargetDirty(index, Dirty::Sampler);
                              })
                        : nullptr;
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

void GraphModel::SetParameter(size_t nodeIndex, const std::string& parameterName, const std::string& parameterValue)
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
        parameterValue, parameterType, &mEvaluationStages.mParameters[nodeIndex][paramOffset]);
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
        // URAdd<AnimTrack> urAdd(int(mEvaluationStages.mAnimTrack.size()), [&] { return
        // &mEvaluationStages.mAnimTrack;
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
    animTrack->mAnimation->SetValue(frame, &mEvaluationStages.mParameters[parameterOffset]);
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

void GraphModel::SetParameters(size_t nodeIndex, const std::vector<unsigned char>& parameters)
{
    assert(mbTransaction);

    auto ur = mUndoRedo ? std::make_unique<URChange<std::vector<unsigned char>>>(
                              int(nodeIndex),
                              [&](int index) { return &mEvaluationStages.mParameters[index]; },
                              [&](int index) {
                                  // auto& stage = mEvaluationStages.mStages[index];
                                  // mEvaluationStages.SetSamplers(index, stage.mInputSamplers);
                                  // mEditingContext.SetTargetDirty(index, Dirty::Sampler);
                              })
                        : nullptr;

    mEvaluationStages.mParameters[nodeIndex] = parameters;
    //mEditingContext.SetTargetDirty(index, Dirty::Parameter);
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
    mEvaluationStages.SetKeyboardMouse(nodeIndex, rx, ry, lButDown, rButDown, bCtrl, bAlt, bShift);
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

const std::vector<unsigned char>& GraphModel::GetParameters(size_t nodeIndex) const
{
    return mEvaluationStages.mParameters[nodeIndex];
}