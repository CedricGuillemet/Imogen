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

#include "NodeGraphControler.h"
#include "EvaluationStages.h"
#include "Library.h"
#include "EvaluationContext.h"
#include "Evaluators.h"
#include "UI.h"
#include "Utils.h"

NodeGraphControler::NodeGraphControler()
    : mbMouseDragging(false), mEditingContext(mEvaluationStages, false, 1024, 1024), mUndoRedoParamSetMouse(nullptr)
{
    mCategories = &MetaNode::mCategories;
}

void NodeGraphControler::Clear()
{
    mSelectedNodeIndex = -1;
    mBackgroundNode = -1;
    mEvaluationStages.Clear();
    mEvaluationStages.mStages.clear();
    mEditingContext.Clear();
}

void NodeGraphControler::SetParamBlock(size_t index, const std::vector<unsigned char>& parameters)
{
    auto& stage = mEvaluationStages.mStages[index];
    stage.mParameters = parameters;
    mEvaluationStages.SetEvaluationParameters(index, parameters);
    mEvaluationStages.SetEvaluationSampler(index, stage.mInputSamplers);
    mEditingContext.SetTargetDirty(index, Dirty::Parameter);
}

void NodeGraphControler::NodeIsAdded(int index)
{
    auto& stage = mEvaluationStages.mStages[index];
    mEvaluationStages.SetEvaluationParameters(index, stage.mParameters);
    mEvaluationStages.SetEvaluationSampler(index, stage.mInputSamplers);
    mEditingContext.SetTargetDirty(index, Dirty::Input);
};

void NodeGraphControler::AddSingleNode(size_t type)
{
    mEvaluationStages.AddSingleEvaluation(type);
    NodeIsAdded(int(mEvaluationStages.mStages.size()) - 1);
}

void NodeGraphControler::UserAddNode(size_t type)
{
    URAdd<EvaluationStage> undoRedoAddNode(int(mEvaluationStages.mStages.size()),
                                           [&]() { return &mEvaluationStages.mStages; },
                                           [](int) {},
                                           [&](int index) { NodeIsAdded(index); });

    mEditingContext.UserAddStage();
    AddSingleNode(type);
}

void NodeGraphControler::UserDeleteNode(size_t index)
{
    URDummy urdummy;
    mEvaluationStages.RemoveAnimation(index);
    mEvaluationStages.RemovePins(index);
    mEditingContext.UserDeleteStage(index);
    mEvaluationStages.UserDeleteEvaluation(index);
    if (mBackgroundNode == int(index))
    {
        mBackgroundNode = -1;
    }
    else if (mBackgroundNode > int(index))
    {
        mBackgroundNode--;
    }
}

void NodeGraphControler::HandlePin(uint32_t parameterPair)
{
    auto pinIter = std::find(
        mEvaluationStages.mPinnedParameters.begin(), mEvaluationStages.mPinnedParameters.end(), parameterPair);
    bool hasPin = pinIter != mEvaluationStages.mPinnedParameters.end();
    bool checked = hasPin;
    if (ImGui::Checkbox("", &checked))
    {
        if (hasPin)
        {
            URDel<uint32_t> undoRedoDelPin(int(&(*pinIter) - mEvaluationStages.mPinnedParameters.data()),
                                           [&]() { return &mEvaluationStages.mPinnedParameters; });
            mEvaluationStages.mPinnedParameters.erase(pinIter);
        }
        else
        {
            URAdd<uint32_t> undoRedoAddNode(int(mEvaluationStages.mPinnedParameters.size()),
                                            [&]() { return &mEvaluationStages.mPinnedParameters; });
            mEvaluationStages.mPinnedParameters.push_back(parameterPair);
        }
    }
}

bool NodeGraphControler::EditSingleParameter(unsigned int nodeIndex,
                                             unsigned int parameterIndex,
                                             void* paramBuffer,
                                             const MetaParameter& param)
{
    bool dirty = false;
    uint32_t parameterPair = (uint32_t(nodeIndex) << 16) + parameterIndex;
    ImGui::PushID(parameterPair * 4);
    HandlePin(parameterPair);
    ImGui::SameLine();
    switch (param.mType)
    {
        case Con_Float:
            dirty |= ImGui::InputFloat(param.mName.c_str(), (float*)paramBuffer);
            break;
        case Con_Float2:
            dirty |= ImGui::InputFloat2(param.mName.c_str(), (float*)paramBuffer);
            break;
        case Con_Float3:
            dirty |= ImGui::InputFloat3(param.mName.c_str(), (float*)paramBuffer);
            break;
        case Con_Float4:
            dirty |= ImGui::InputFloat4(param.mName.c_str(), (float*)paramBuffer);
            break;
        case Con_Color4:
            dirty |= ImGui::ColorPicker4(param.mName.c_str(), (float*)paramBuffer);
            break;
        case Con_Int:
            dirty |= ImGui::InputInt(param.mName.c_str(), (int*)paramBuffer);
            break;
        case Con_Int2:
            dirty |= ImGui::InputInt2(param.mName.c_str(), (int*)paramBuffer);
            break;
        case Con_Ramp:
        {
            RampEdit curveEditDelegate;
            curveEditDelegate.mPointCount = 0;
            for (int k = 0; k < 8; k++)
            {
                curveEditDelegate.mPts[k] = ImVec2(((float*)paramBuffer)[k * 2], ((float*)paramBuffer)[k * 2 + 1]);
                if (k && curveEditDelegate.mPts[k - 1].x > curveEditDelegate.mPts[k].x)
                    break;
                curveEditDelegate.mPointCount++;
            }
            float regionWidth = ImGui::GetWindowContentRegionWidth();
            if (ImCurveEdit::Edit(curveEditDelegate, ImVec2(regionWidth, regionWidth), 974))
            {
                for (size_t k = 0; k < curveEditDelegate.mPointCount; k++)
                {
                    ((float*)paramBuffer)[k * 2] = curveEditDelegate.mPts[k].x;
                    ((float*)paramBuffer)[k * 2 + 1] = curveEditDelegate.mPts[k].y;
                }
                ((float*)paramBuffer)[0] = 0.f;
                ((float*)paramBuffer)[(curveEditDelegate.mPointCount - 1) * 2] = 1.f;
                for (size_t k = curveEditDelegate.mPointCount; k < 8; k++)
                {
                    ((float*)paramBuffer)[k * 2] = -1.f;
                }
                dirty = true;
            }
        }
        break;
        case Con_Ramp4:
        {
            float regionWidth = ImGui::GetWindowContentRegionWidth();
            GradientEdit gradientDelegate;

            gradientDelegate.mPointCount = 0;

            for (int k = 0; k < 8; k++)
            {
                gradientDelegate.mPts[k] = ((ImVec4*)paramBuffer)[k];
                if (k && gradientDelegate.mPts[k - 1].w > gradientDelegate.mPts[k].w)
                    break;
                gradientDelegate.mPointCount++;
            }

            int colorIndex;
            dirty |= ImGradient::Edit(gradientDelegate, ImVec2(regionWidth, 22), colorIndex);
            if (colorIndex != -1)
            {
                dirty |= ImGui::ColorPicker3("", &gradientDelegate.mPts[colorIndex].x);
            }
            if (dirty)
            {
                for (size_t k = 0; k < gradientDelegate.mPointCount; k++)
                {
                    ((ImVec4*)paramBuffer)[k] = gradientDelegate.mPts[k];
                }
                ((ImVec4*)paramBuffer)[0].w = 0.f;
                ((ImVec4*)paramBuffer)[gradientDelegate.mPointCount - 1].w = 1.f;
                for (size_t k = gradientDelegate.mPointCount; k < 8; k++)
                {
                    ((ImVec4*)paramBuffer)[k].w = -1.f;
                }
            }
        }
        break;
        case Con_Angle:
            ((float*)paramBuffer)[0] = RadToDeg(((float*)paramBuffer)[0]);
            dirty |= ImGui::InputFloat(param.mName.c_str(), (float*)paramBuffer);
            ((float*)paramBuffer)[0] = DegToRad(((float*)paramBuffer)[0]);
            break;
        case Con_Angle2:
            ((float*)paramBuffer)[0] = RadToDeg(((float*)paramBuffer)[0]);
            ((float*)paramBuffer)[1] = RadToDeg(((float*)paramBuffer)[1]);
            dirty |= ImGui::InputFloat2(param.mName.c_str(), (float*)paramBuffer);
            ((float*)paramBuffer)[0] = DegToRad(((float*)paramBuffer)[0]);
            ((float*)paramBuffer)[1] = DegToRad(((float*)paramBuffer)[1]);
            break;
        case Con_Angle3:
            ((float*)paramBuffer)[0] = RadToDeg(((float*)paramBuffer)[0]);
            ((float*)paramBuffer)[1] = RadToDeg(((float*)paramBuffer)[1]);
            ((float*)paramBuffer)[2] = RadToDeg(((float*)paramBuffer)[2]);
            dirty |= ImGui::InputFloat3(param.mName.c_str(), (float*)paramBuffer);
            ((float*)paramBuffer)[0] = DegToRad(((float*)paramBuffer)[0]);
            ((float*)paramBuffer)[1] = DegToRad(((float*)paramBuffer)[1]);
            ((float*)paramBuffer)[2] = DegToRad(((float*)paramBuffer)[2]);
            break;
        case Con_Angle4:
            ((float*)paramBuffer)[0] = RadToDeg(((float*)paramBuffer)[0]);
            ((float*)paramBuffer)[1] = RadToDeg(((float*)paramBuffer)[1]);
            ((float*)paramBuffer)[2] = RadToDeg(((float*)paramBuffer)[2]);
            ((float*)paramBuffer)[3] = RadToDeg(((float*)paramBuffer)[3]);
            dirty |= ImGui::InputFloat4(param.mName.c_str(), (float*)paramBuffer);
            ((float*)paramBuffer)[0] = DegToRad(((float*)paramBuffer)[0]);
            ((float*)paramBuffer)[1] = DegToRad(((float*)paramBuffer)[1]);
            ((float*)paramBuffer)[2] = DegToRad(((float*)paramBuffer)[2]);
            ((float*)paramBuffer)[3] = DegToRad(((float*)paramBuffer)[3]);
            break;
        case Con_FilenameWrite:
        case Con_FilenameRead:
            ImGui::PushID(parameterPair * 4 + 1);
            dirty |= ImGui::InputText("", (char*)paramBuffer, 1024);
            ImGui::SameLine();
            if (ImGui::Button("..."))
            {
                #ifdef NFD_OpenDialog
                nfdchar_t* outPath = NULL;
                nfdresult_t result = (param.mType == Con_FilenameRead) ? NFD_OpenDialog(NULL, NULL, &outPath)
                                                                       : NFD_SaveDialog(NULL, NULL, &outPath);

                if (result == NFD_OKAY)
                {
                    strcpy((char*)paramBuffer, outPath);
                    free(outPath);
                    dirty = true;
                }
                #endif
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Text(param.mName.c_str());
            break;
        case Con_Enum:
        {
            std::string cbString = param.mEnumList;
            for (auto& c : cbString)
            {
                if (c == '|')
                    c = '\0';
            }
            dirty |= ImGui::Combo(param.mName.c_str(), (int*)paramBuffer, cbString.c_str());
        }
        break;
        case Con_ForceEvaluate:
            if (ImGui::Button(param.mName.c_str()))
            {
                EvaluationInfo evaluationInfo;
                evaluationInfo.forcedDirty = 1;
                evaluationInfo.uiPass = 0;
                mEditingContext.RunSingle(nodeIndex, evaluationInfo);
            }
            break;
        case Con_Bool:
        {
            bool checked = (*(int*)paramBuffer) != 0;
            if (ImGui::Checkbox(param.mName.c_str(), &checked))
            {
                *(int*)paramBuffer = checked ? 1 : 0;
                dirty = true;
            }
        }
        break;
        case Con_Camera:
            if (ImGui::Button("Reset"))
            {
                Camera* cam = (Camera*)paramBuffer;
                cam->mPosition = Vec4(0.f, 0.f, 0.f);
                cam->mDirection = Vec4(0.f, 0.f, 1.f);
                cam->mUp = Vec4(0.f, 1.f, 0.f);
            }
            break;
    }
    ImGui::PopID();
    return dirty;
}

void NodeGraphControler::UpdateDirtyParameter(int index)
{
    auto& stage = mEvaluationStages.mStages[index];
    mEvaluationStages.SetEvaluationParameters(index, stage.mParameters);
    mEditingContext.SetTargetDirty(index, Dirty::Parameter);
}

void NodeGraphControler::PinnedEdit()
{
    int dirtyNode = -1;
    for (const auto pin : mEvaluationStages.mPinnedParameters)
    {
        unsigned int nodeIndex = (pin >> 16) & 0xFFFF;
        unsigned int parameterIndex = pin & 0xFFFF;

        size_t nodeType = mEvaluationStages.mStages[nodeIndex].mType;
        const MetaNode& metaNode = gMetaNodes[nodeType];
        if (parameterIndex >= metaNode.mParams.size())
            continue;

        ImGui::PushID(171717 + pin);
        const MetaParameter& metaParam = metaNode.mParams[parameterIndex];
        unsigned char* paramBuffer = mEvaluationStages.mStages[nodeIndex].mParameters.data();
        paramBuffer += GetParameterOffset(uint32_t(nodeType), parameterIndex);
        if (EditSingleParameter(nodeIndex, parameterIndex, paramBuffer, metaParam))
            dirtyNode = nodeIndex;

        ImGui::PopID();
    }
    if (dirtyNode != -1)
    {
        UpdateDirtyParameter(dirtyNode);
    }
}

void NodeGraphControler::EditNodeParameters()
{
    size_t index = mSelectedNodeIndex;

    const MetaNode* metaNodes = gMetaNodes.data();
    bool dirty = false;
    bool forceEval = false;
    bool samplerDirty = false;
    auto& stage = mEvaluationStages.mStages[index];
    const MetaNode& currentMeta = metaNodes[stage.mType];

    if (ImGui::CollapsingHeader("Samplers", 0))
    {
        URChange<std::vector<InputSampler>> undoRedoSampler(int(index),
                                                            [&](int index) { return &stage.mInputSamplers; },
                                                            [&](int index) {
                                                                auto& node = mEvaluationStages.mStages[index];
                                                                mEvaluationStages.SetEvaluationSampler(
                                                                    index, stage.mInputSamplers);
                                                                mEditingContext.SetTargetDirty(index, Dirty::Sampler);
                                                            });

        for (size_t i = 0; i < stage.mInputSamplers.size(); i++)
        {
            InputSampler& inputSampler = stage.mInputSamplers[i];
            static const char* wrapModes = {"REPEAT\0EDGE\0BORDER\0MIRROR"};
            static const char* filterModes = {"LINEAR\0NEAREST"};
            ImGui::PushItemWidth(80);
            ImGui::PushID(int(99 + i));
            HandlePinIO(index, i, false);
			ImGui::SameLine();
            ImGui::Text("Sampler %d", i);
            samplerDirty |= ImGui::Combo("U", (int*)&inputSampler.mWrapU, wrapModes);
            ImGui::SameLine();
            samplerDirty |= ImGui::Combo("V", (int*)&inputSampler.mWrapV, wrapModes);
            ImGui::SameLine();
            ImGui::PushItemWidth(80);
            samplerDirty |= ImGui::Combo("Min", (int*)&inputSampler.mFilterMin, filterModes);
            ImGui::SameLine();
            samplerDirty |= ImGui::Combo("Mag", (int*)&inputSampler.mFilterMag, filterModes);
            ImGui::PopItemWidth();
            ImGui::PopID();
            ImGui::PopItemWidth();
        }
        if (samplerDirty)
        {
            mEvaluationStages.SetEvaluationSampler(index, stage.mInputSamplers);
            mEditingContext.SetTargetDirty(index, Dirty::Sampler);
        }
        else
        {
            undoRedoSampler.Discard();
        }
    }
    if (!ImGui::CollapsingHeader(currentMeta.mName.c_str(), 0, ImGuiTreeNodeFlags_DefaultOpen))
        return;

    URChange<std::vector<unsigned char>> undoRedoParameter(
        int(index),
        [&](int index) { return &mEvaluationStages.mStages[index].mParameters; },
        [&](int index) { UpdateDirtyParameter(index); });

    unsigned char* paramBuffer = stage.mParameters.data();
    int i = 0;
    for (const MetaParameter& param : currentMeta.mParams)
    {
        ImGui::PushID(667889 + i);

        dirty |= EditSingleParameter((unsigned int)(index), i, paramBuffer, param);

        ImGui::PopID();
        paramBuffer += GetParameterTypeSize(param.mType);
        i++;
    }

    if (dirty)
    {
        UpdateDirtyParameter(int(index));
    }
    else
    {
        undoRedoParameter.Discard();
    }
}

void NodeGraphControler::HandlePinIO(size_t nodeIndex, size_t slotIndex, bool forOutput)
{
    if (IsIOUsed(nodeIndex, slotIndex, forOutput))
	{
            return;
	}
        ImGui::PushID(nodeIndex * 256 + slotIndex * 2 + (forOutput ? 1 : 0));
        bool pinned = IsIOPinned(nodeIndex, slotIndex, forOutput);
    ImGui::Checkbox("", &pinned);
        mEvaluationStages.SetIOPin(nodeIndex, slotIndex, forOutput, pinned);
    ImGui::PopID();
}

void NodeGraphControler::NodeEdit()
{
    ImGuiIO& io = ImGui::GetIO();

    if (mSelectedNodeIndex == -1)
    {
        /*
        for (const auto pin : mEvaluationStages.mPinnedParameters)
        {
            unsigned int nodeIndex = (pin >> 16) & 0xFFFF;
            unsigned int parameterIndex = pin & 0xFFFF;
            if (parameterIndex != 0xDEAD)
                continue;

            ImGui::PushID(1717171 + nodeIndex);
            uint32_t parameterPair = (uint32_t(nodeIndex) << 16) + 0xDEAD;
            HandlePin(parameterPair);
            ImGui::SameLine();
            Imogen::RenderPreviewNode(nodeIndex, *this);
            ImGui::PopID();
        }
		*/
        auto& io = mEvaluationStages.mPinnedIO;
        for (size_t nodeIndex = 0; nodeIndex < io.size(); nodeIndex++)
			{
            if ((io[nodeIndex]&1) == 0)
                            {
                continue;
				}
        ImGui::PushID(1717171 + nodeIndex);
        uint32_t parameterPair = (uint32_t(nodeIndex) << 16) + 0xDEAD;
        HandlePinIO(nodeIndex, 0, true);
        ImGui::SameLine();
        Imogen::RenderPreviewNode(nodeIndex, *this);
        ImGui::PopID();
		}
        PinnedEdit();
    }
    else
    {
        if (ImGui::CollapsingHeader("Preview", 0, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID(1717171);
            ImGui::BeginGroup();
            HandlePinIO(mSelectedNodeIndex, 0, true);
            unsigned int maxiMini = gImageCache.GetTexture("Stock/MaxiMini.png");
            bool selectedNodeAsBackground = mBackgroundNode == mSelectedNodeIndex;
            float ofs = selectedNodeAsBackground ? 0.5f : 0.f;
            if (ImGui::ImageButton(
                    (ImTextureID)(uint64_t)maxiMini, ImVec2(12, 13), ImVec2(0.f + ofs, 1.f), ImVec2(0.5f + ofs, 0.f)))
            {
                mBackgroundNode = selectedNodeAsBackground ? -1 : mSelectedNodeIndex;
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            Imogen::RenderPreviewNode(mSelectedNodeIndex, *this);
            ImGui::PopID();
        }

        EditNodeParameters();
    }
}

void NodeGraphControler::SetTimeSlot(size_t index, int frameStart, int frameEnd)
{
    auto& stage = mEvaluationStages.mStages[index];
    stage.mStartFrame = frameStart;
    stage.mEndFrame = frameEnd;
}

void NodeGraphControler::SetTimeDuration(size_t index, int duration)
{
    auto& stage = mEvaluationStages.mStages[index];
    stage.mEndFrame = stage.mStartFrame + duration;
}

void NodeGraphControler::InvalidateParameters()
{
    for (size_t i = 0; i < mEvaluationStages.mStages.size(); i++)
    {
        auto& stage = mEvaluationStages.mStages[i];
        mEvaluationStages.SetEvaluationParameters(i, stage.mParameters);
    }
}

void NodeGraphControler::SetKeyboardMouse(float rx,
                                          float ry,
                                          float dx,
                                          float dy,
                                          bool lButDown,
                                          bool rButDown,
                                          float wheel,
                                          bool bCtrl,
                                          bool bAlt,
                                          bool bShift)
{
    if (mSelectedNodeIndex == -1)
        return;

    if (!lButDown)
        mbMouseDragging = false;

    if ((!lButDown && !rButDown) && mUndoRedoParamSetMouse)
    {
        delete mUndoRedoParamSetMouse;
        mUndoRedoParamSetMouse = nullptr;
    }
    if ((lButDown || rButDown) && !mUndoRedoParamSetMouse)
    {
        mUndoRedoParamSetMouse = new URChange<std::vector<unsigned char>>(
            mSelectedNodeIndex,
            [&](int index) { return &mEvaluationStages.mStages[index].mParameters; },
            [&](int index) { UpdateDirtyParameter(index); });
    }
    const MetaNode* metaNodes = gMetaNodes.data();
    size_t res = 0;
    const MetaNode& metaNode = metaNodes[mEvaluationStages.mStages[mSelectedNodeIndex].mType];

    unsigned char* paramBuffer = mEvaluationStages.mStages[mSelectedNodeIndex].mParameters.data();
    bool parametersUseMouse = false;

    // camera handling
    for (auto& param : metaNode.mParams)
    {
        float* paramFlt = (float*)paramBuffer;
        if (param.mType == Con_Camera)
        {
            Camera* cam = (Camera*)paramBuffer;
            cam->mPosition += cam->mDirection * wheel;
            Vec4 right = Cross(cam->mUp, cam->mDirection);
            right.y = 0.f; // keep head up
            right.Normalize();
            auto& io = ImGui::GetIO();
            if (io.KeyAlt)
            {
                if (io.MouseDown[2])
                {
                    cam->mPosition += (right * io.MouseDelta.x + cam->mUp * io.MouseDelta.y) * 0.01f;
                }
                if (io.MouseDown[1])
                {
                    cam->mPosition += (cam->mDirection * io.MouseDelta.y) * 0.01f;
                }
                if (io.MouseDown[0])
                {
                    Mat4x4 tr, rtUp, rtRight, trp;
                    tr.Translation(-(cam->mPosition));
                    rtRight.RotationAxis(right, io.MouseDelta.y * 0.01f);
                    rtUp.RotationAxis(cam->mUp, -io.MouseDelta.x * 0.01f);
                    trp.Translation((cam->mPosition));
                    Mat4x4 res = tr * rtRight * rtUp * trp;
                    cam->mPosition.TransformPoint(res);
                    cam->mDirection.TransformVector(res);
                    cam->mUp.Cross(cam->mDirection, right);
                    cam->mUp.Normalize();
                }
            }
            parametersUseMouse = true;
        }
        paramBuffer += GetParameterTypeSize(param.mType);
    }

    //
    paramBuffer = mEvaluationStages.mStages[mSelectedNodeIndex].mParameters.data();
    if (lButDown)
    {
        for (auto& param : metaNode.mParams)
        {
            float* paramFlt = (float*)paramBuffer;
            if (param.mType == Con_Camera)
            {
                Camera* cam = (Camera*)paramBuffer;
                if (cam->mDirection.LengthSq() < FLT_EPSILON)
                    cam->mDirection.Set(0.f, 0.f, 1.f);
                cam->mPosition += cam->mDirection * wheel;
            }
            if (param.mbQuadSelect && param.mType == Con_Float4)
            {
                if (!mbMouseDragging)
                {
                    paramFlt[2] = paramFlt[0] = rx;
                    paramFlt[3] = paramFlt[1] = 1.f - ry;
                    mbMouseDragging = true;
                }
                else
                {
                    paramFlt[2] = rx;
                    paramFlt[3] = 1.f - ry;
                }
                continue;
            }

            if (param.mRangeMinX != 0.f || param.mRangeMaxX != 0.f)
            {
                if (param.mbRelative)
                {
                    paramFlt[0] += (param.mRangeMaxX - param.mRangeMinX) * dx;
                    if (param.mbLoop)
                        paramFlt[0] = fmodf(paramFlt[0], fabsf(param.mRangeMaxX - param.mRangeMinX)) +
                                      min(param.mRangeMinX, param.mRangeMaxX);
                }
                else
                {
                    paramFlt[0] = ImLerp(param.mRangeMinX, param.mRangeMaxX, rx);
                }
            }
            if (param.mRangeMinY != 0.f || param.mRangeMaxY != 0.f)
            {
                if (param.mbRelative)
                {
                    paramFlt[1] += (param.mRangeMaxY - param.mRangeMinY) * dy;
                    if (param.mbLoop)
                        paramFlt[1] = fmodf(paramFlt[1], fabsf(param.mRangeMaxY - param.mRangeMinY)) +
                                      min(param.mRangeMinY, param.mRangeMaxY);
                }
                else
                {
                    paramFlt[1] = ImLerp(param.mRangeMinY, param.mRangeMaxY, ry);
                }
            }
            paramBuffer += GetParameterTypeSize(param.mType);
            parametersUseMouse = true;
        }
    }
    if (metaNode.mbHasUI || parametersUseMouse)
    {
        mEvaluationStages.SetKeyboardMouse(mSelectedNodeIndex, rx, ry, lButDown, rButDown, bCtrl, bAlt, bShift);
        mEvaluationStages.SetEvaluationParameters(mSelectedNodeIndex,
                                                  mEvaluationStages.mStages[mSelectedNodeIndex].mParameters);
        mEditingContext.SetTargetDirty(mSelectedNodeIndex, Dirty::Mouse);
    }
}

bool NodeGraphControler::NodeIs2D(size_t nodeIndex) const
{
    auto target = mEditingContext.GetRenderTarget(nodeIndex);
    if (target)
        return target->mImage->mNumFaces == 1;
    return false;
}

bool NodeGraphControler::NodeIsCompute(size_t nodeIndex) const
{
    /*auto buffer = mEditingContext.GetComputeBuffer(nodeIndex);
    if (buffer)
        return true;
    return false;
    */
    return (gEvaluators.GetMask(mEvaluationStages.mStages[nodeIndex].mType) & EvaluationGLSLCompute) != 0;
}

bool NodeGraphControler::NodeIsCubemap(size_t nodeIndex) const
{
    auto target = mEditingContext.GetRenderTarget(nodeIndex);
    if (target)
        return target->mImage->mNumFaces == 6;
    return false;
}

ImVec2 NodeGraphControler::GetEvaluationSize(size_t nodeIndex) const
{
    int imageWidth(1), imageHeight(1);
    EvaluationAPI::GetEvaluationSize(&mEditingContext, int(nodeIndex), &imageWidth, &imageHeight);
    return ImVec2(float(imageWidth), float(imageHeight));
}

void NodeGraphControler::CopyNodes(const std::vector<size_t> nodes)
{
    mStagesClipboard.clear();
    for (auto nodeIndex : nodes)
        mStagesClipboard.push_back(mEvaluationStages.mStages[nodeIndex]);
}

void NodeGraphControler::CutNodes(const std::vector<size_t> nodes)
{
    mStagesClipboard.clear();
}

void NodeGraphControler::PasteNodes()
{
    for (auto& sourceNode : mStagesClipboard)
    {
        URAdd<EvaluationStage> undoRedoAddNode(int(mEvaluationStages.mStages.size()),
                                               [&]() { return &mEvaluationStages.mStages; },
                                               [](int) {},
                                               [&](int index) { NodeIsAdded(index); });

        mEditingContext.UserAddStage();
        size_t target = mEvaluationStages.mStages.size();
        AddSingleNode(sourceNode.mType);

        auto& stage = mEvaluationStages.mStages.back();
        stage.mParameters = sourceNode.mParameters;
        stage.mInputSamplers = sourceNode.mInputSamplers;
        stage.mStartFrame = sourceNode.mStartFrame;
        stage.mEndFrame = sourceNode.mEndFrame;

        mEvaluationStages.SetEvaluationParameters(target, stage.mParameters);
        mEvaluationStages.SetEvaluationSampler(target, stage.mInputSamplers);
        mEvaluationStages.SetTime(&mEditingContext, mEditingContext.GetCurrentTime(), true);
        mEditingContext.SetTargetDirty(target, Dirty::All);
    }
}

// animation
AnimTrack* NodeGraphControler::GetAnimTrack(uint32_t nodeIndex, uint32_t parameterIndex)
{
    for (auto& animTrack : mEvaluationStages.mAnimTrack)
    {
        if (animTrack.mNodeIndex == nodeIndex && animTrack.mParamIndex == parameterIndex)
            return &animTrack;
    }
    return NULL;
}

void NodeGraphControler::MakeKey(int frame, uint32_t nodeIndex, uint32_t parameterIndex)
{
    if (nodeIndex == -1)
    {
        return;
    }
    URDummy urDummy;

    AnimTrack* animTrack = GetAnimTrack(nodeIndex, parameterIndex);
    if (!animTrack)
    {
        URAdd<AnimTrack> urAdd(int(mEvaluationStages.mAnimTrack.size()), [&] { return &mEvaluationStages.mAnimTrack; });
        uint32_t parameterType = gMetaNodes[mEvaluationStages.mStages[nodeIndex].mType].mParams[parameterIndex].mType;
        AnimTrack newTrack;
        newTrack.mNodeIndex = nodeIndex;
        newTrack.mParamIndex = parameterIndex;
        newTrack.mValueType = parameterType;
        newTrack.mAnimation = AllocateAnimation(parameterType);
        mEvaluationStages.mAnimTrack.push_back(newTrack);
        animTrack = &mEvaluationStages.mAnimTrack.back();
    }
    URChange<AnimTrack> urChange(int(animTrack - mEvaluationStages.mAnimTrack.data()),
                                 [&](int index) { return &mEvaluationStages.mAnimTrack[index]; });
    EvaluationStage& stage = mEvaluationStages.mStages[nodeIndex];
    size_t parameterOffset = GetParameterOffset(uint32_t(stage.mType), parameterIndex);
    animTrack->mAnimation->SetValue(frame, &stage.mParameters[parameterOffset]);
}

void NodeGraphControler::GetKeyedParameters(int frame, uint32_t nodeIndex, std::vector<bool>& keyed)
{
}

void NodeGraphControler::DrawNodeImage(ImDrawList* drawList,
                                       const ImRect& rc,
                                       const ImVec2 marge,
                                       const size_t nodeIndex)
{
    if (NodeIsProcesing(nodeIndex) == 1)
    {
        AddUICustomDraw(drawList, rc, DrawUICallbacks::DrawUIProgress, nodeIndex, &mEditingContext);
    }
    else if (NodeIsCubemap(nodeIndex))
    {
        AddUICustomDraw(drawList, rc, DrawUICallbacks::DrawUICubemap, nodeIndex, &mEditingContext);
    }
    else if (NodeIsCompute(nodeIndex))
    {
    }
    else
    {
        drawList->AddImage((ImTextureID)(int64_t)(GetNodeTexture(size_t(nodeIndex))),
                           rc.Min + marge,
                           rc.Max - marge,
                           ImVec2(0, 1),
                           ImVec2(1, 0));
    }
}

bool NodeGraphControler::RenderBackground()
{
    if (mBackgroundNode != -1)
    {
        Imogen::RenderPreviewNode(mBackgroundNode, *this, true);
        return true;
    }
    return false;
}

void NodeGraphControler::SetParameter(int nodeIndex,
                                      const std::string& parameterName,
                                      const std::string& parameterValue)
{
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
    ParseStringToParameter(parameterValue, parameterType, &mEvaluationStages.mStages[nodeIndex].mParameters[paramOffset]);
    mEditingContext.SetTargetDirty(nodeIndex, Dirty::Parameter);
}