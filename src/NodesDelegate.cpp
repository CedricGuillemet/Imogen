// https://github.com/CedricGuillemet/Imogen
//
// The MIT License(MIT)
// 
// Copyright(c) 2018 Cedric Guillemet
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

#include "Nodes.h"
#include "Evaluation.h"
#include "ImCurveEdit.h"
#include "ImGradient.h"
#include "Library.h"
#include "nfd.h"
#include "EvaluationContext.h"
#include "NodesDelegate.h"
#include "Evaluators.h"

TileNodeEditGraphDelegate gNodeDelegate;


struct RampEdit : public ImCurveEdit::Delegate
{
    RampEdit()
    {
        mMin = ImVec2(0.f, 0.f);
        mMax = ImVec2(1.f, 1.f);
    }
    size_t GetCurveCount()
    {
        return 1;
    }

    size_t GetPointCount(size_t curveIndex)
    {
        return mPointCount;
    }

    uint32_t GetCurveColor(size_t curveIndex)
    {
        return 0xFFAAAAAA;
    }
    ImVec2* GetPoints(size_t curveIndex)
    {
        return mPts;
    }
    virtual ImVec2& GetMin() { return mMin; }
    virtual ImVec2& GetMax() { return mMax; }

    virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
    {
        mPts[pointIndex] = value;
        SortValues(curveIndex);
        for (size_t i = 0; i < GetPointCount(curveIndex); i++)
        {
            if (mPts[i].x == value.x)
                return int(i);
        }
        return pointIndex;
    }
    virtual void AddPoint(size_t curveIndex, ImVec2 value)
    {
        if (mPointCount >= 8)
            return;
        mPts[mPointCount++] = value;
        SortValues(curveIndex);
    }

    virtual void DelPoint(size_t curveIndex, size_t pointIndex)
    {
        mPts[pointIndex].x = FLT_MAX;
        SortValues(curveIndex);
        mPointCount--;
        mPts[mPointCount].x = -1.f; // end marker
    }
    ImVec2 mPts[8];
    size_t mPointCount;

private:
    void SortValues(size_t curveIndex)
    {
        auto b = std::begin(mPts);
        auto e = std::begin(mPts) + GetPointCount(curveIndex);
        std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });
    }
    ImVec2 mMin, mMax;
};


struct GradientEdit : public ImGradient::Delegate
{
    GradientEdit()
    {
        mPointCount = 0;
    }

    size_t GetPointCount()
    {
        return mPointCount;
    }

    ImVec4* GetPoints()
    {
        return mPts;
    }

    virtual int EditPoint(int pointIndex, ImVec4 value)
    {
        mPts[pointIndex] = value;
        SortValues();
        for (size_t i = 0; i < GetPointCount(); i++)
        {
            if (mPts[i].w == value.w)
                return int(i);
        }
        return pointIndex;
    }
    virtual void AddPoint(ImVec4 value)
    {
        if (mPointCount >= 8)
            return;
        mPts[mPointCount++] = value;
        SortValues();
    }
    virtual ImVec4 GetPoint(float t)
    {
        if (GetPointCount() == 0)
            return ImVec4(1.f, 1.f, 1.f, 1.f);
        if (GetPointCount() == 1 || t <= mPts[0].w)
            return mPts[0];

        for (size_t i = 0; i < GetPointCount() - 1; i++)
        {
            if (mPts[i].w <= t && mPts[i + 1].w >= t)
            {
                float r = (t - mPts[i].w) / (mPts[i + 1].w - mPts[i].w);
                return ImLerp(mPts[i], mPts[i + 1], r);
            }
        }
        return mPts[GetPointCount() - 1];
    }
    ImVec4 mPts[8];
    size_t mPointCount;

private:
    void SortValues()
    {
        auto b = std::begin(mPts);
        auto e = std::begin(mPts) + GetPointCount();
        std::sort(b, e, [](ImVec4 a, ImVec4 b) { return a.w < b.w; });

    }
};

TileNodeEditGraphDelegate::TileNodeEditGraphDelegate() : mbMouseDragging(false), mEditingContext(gEvaluation, false, 1024, 1024), mFrameMin(0), mFrameMax(1)
{
    mCategoriesCount = 10;
    static const char *categories[] = {
        "Transform",
        "Generator",
        "Material",
        "Blend",
        "Filter",
        "Noise",
        "File",
        "Paint",
        "Cubemap",
        "Fur"};
    mCategories = categories;
    gCurrentContext = &mEditingContext;
}

void TileNodeEditGraphDelegate::Clear()
{
    mSelectedNodeIndex = -1;
    mNodes.clear();
    mAnimTrack.clear();
    mEditingContext.Clear();
}

void TileNodeEditGraphDelegate::SetParamBlock(size_t index, const std::vector<unsigned char>& parameters)
{
    ImogenNode & node = mNodes[index];
    node.mParameters = parameters;
    gEvaluation.SetEvaluationParameters(index, parameters);
    gEvaluation.SetEvaluationSampler(index, node.mInputSamplers);
}

void NodeIsDeleted(int index)
{
};

void NodeIsAdded(int index)
{
    auto& node = gNodeDelegate.mNodes[index];
    gEvaluation.SetEvaluationParameters(index, node.mParameters);
    gEvaluation.SetEvaluationSampler(index, node.mInputSamplers);
};

void TileNodeEditGraphDelegate::AddSingleNode(size_t type)
{
    const size_t inputCount = gMetaNodes[type].mInputs.size();

    ImogenNode node;
    node.mRuntimeUniqueId = GetRuntimeId();
    node.mType = type;
    node.mStartFrame = mFrameMin;
    node.mEndFrame = mFrameMax;
#ifdef _DEBUG
    node.mNodeTypename = gMetaNodes[type].mName;
#endif
    InitDefault(node);
    node.mInputSamplers.resize(inputCount);

    mNodes.push_back(node);

    NodeIsAdded(int(mNodes.size()) - 1);
}

void TileNodeEditGraphDelegate::UserAddNode(size_t type)
{
    URAdd<ImogenNode> undoRedoAddNode(int(mNodes.size()), []() {return &gNodeDelegate.mNodes; },
        NodeIsDeleted, NodeIsAdded);

    mEditingContext.UserAddStage();
    gEvaluation.UserAddEvaluation(type);
    AddSingleNode(type);
}

void TileNodeEditGraphDelegate::UserDeleteNode(size_t index)
{
    {
        URDel<ImogenNode> undoRedoDelNode(int(index), []() {return &gNodeDelegate.mNodes; },
            NodeIsDeleted, NodeIsAdded);

        NodeIsDeleted(int(index));
        mNodes.erase(mNodes.begin() + index);
    }
    RemoveAnimation(index);
    RemovePins(index);
    mEditingContext.UserDeleteStage(index);
    gEvaluation.UserDeleteEvaluation(index);
}
    
const float PI = 3.14159f;
float RadToDeg(float a) { return a * 180.f / PI; }
float DegToRad(float a) { return a / 180.f * PI; }

void TileNodeEditGraphDelegate::InitDefault(ImogenNode& node)
{
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[node.mType];
    const size_t paramsSize = ComputeNodeParametersSize(node.mType);
    node.mParameters.resize(paramsSize);
    unsigned char *paramBuffer = node.mParameters.data();
    memset(paramBuffer, 0, paramsSize);
    int i = 0;
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (!_stricmp(param.mName.c_str(), "scale"))
        {
            float* pf = (float*)paramBuffer;
            switch (param.mType)
            {
            case Con_Float:
                pf[0] = 1.f;
                break;
            case Con_Float2:
                pf[1] = pf[0] = 1.f;
                break;
            case Con_Float3:
                pf[2] = pf[1] = pf[0] = 1.f;
                break;
            case Con_Float4:
                pf[3] = pf[2] = pf[1] = pf[0] = 1.f;
                break;
            }
        }
        switch (param.mType)
        {
            case Con_Ramp:
                ((ImVec2*)paramBuffer)[0] = ImVec2(0, 0);
                ((ImVec2*)paramBuffer)[1] = ImVec2(1, 1);
                break;
            case Con_Ramp4:
                ((ImVec4*)paramBuffer)[0] = ImVec4(0, 0, 0, 0);
                ((ImVec4*)paramBuffer)[1] = ImVec4(1, 1, 1, 1);
                break;
            case Con_Camera:
            {
                Camera *cam = (Camera*)paramBuffer;
                cam->mDirection = Vec4(0.f, 0.f, 1.f, 0.f);
                cam->mUp = Vec4(0.f, 1.f, 0.f, 0.f);
            }
            break;
        }
        paramBuffer += GetParameterTypeSize(param.mType);
    }
}
bool TileNodeEditGraphDelegate::EditSingleParameter(unsigned int nodeIndex, unsigned int parameterIndex, void *paramBuffer, const MetaParameter& param)
{
    bool dirty = false;
    uint32_t parameterPair = (uint32_t(nodeIndex) << 16) + parameterIndex;
    auto pinIter = std::find(mPinnedParameters.begin(), mPinnedParameters.end(), parameterPair);
    bool hasPin = pinIter != mPinnedParameters.end();
    bool checked = hasPin;
    if (ImGui::Checkbox("", &checked))
    {
        if (hasPin)
            mPinnedParameters.erase(pinIter);
        else
            mPinnedParameters.push_back(parameterPair);
        std::sort(mPinnedParameters.begin(), mPinnedParameters.end());
    }
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
        dirty |= ImGui::InputText("", (char*)paramBuffer, 1024);
        ImGui::SameLine();
        if (ImGui::Button("..."))
        {
            nfdchar_t *outPath = NULL;
            nfdresult_t result = (param.mType == Con_FilenameRead) ? NFD_OpenDialog(NULL, NULL, &outPath) : NFD_SaveDialog(NULL, NULL, &outPath);

            if (result == NFD_OKAY)
            {
                strcpy((char*)paramBuffer, outPath);
                free(outPath);
                dirty = true;
            }
        }
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
            Camera *cam = (Camera*)paramBuffer;
            cam->mPosition = Vec4(0.f, 0.f, 0.f);
            cam->mDirection = Vec4(0.f, 0.f, 1.f);
            cam->mUp = Vec4(0.f, 1.f, 0.f);
        }
        break;
    }
    return dirty;
}

void UpdateDirtyParameter(int index)
{
    auto& node = gNodeDelegate.mNodes[index];
    gEvaluation.SetEvaluationParameters(index, node.mParameters);
    gCurrentContext->SetTargetDirty(index);
}

void TileNodeEditGraphDelegate::PinnedEdit()
{
    int dirtyNode = -1;
    for (const auto pin : mPinnedParameters)
    {
        unsigned int nodeIndex = (pin >> 16) & 0xFFFF;
        unsigned int parameterIndex = pin & 0xFFFF;

        size_t nodeType = mNodes[nodeIndex].mType;
        const MetaNode& metaNode = gMetaNodes[nodeType];
        if (parameterIndex >= metaNode.mParams.size())
            continue;
        
        ImGui::PushID(171717 + pin);
        const MetaParameter& metaParam = metaNode.mParams[parameterIndex];
        unsigned char *paramBuffer = mNodes[nodeIndex].mParameters.data();
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

void TileNodeEditGraphDelegate::EditNode()
{
    size_t index = mSelectedNodeIndex;

    const MetaNode* metaNodes = gMetaNodes.data();
    bool dirty = false;
    bool forceEval = false;
    bool samplerDirty = false;
    ImogenNode& node = mNodes[index];
    const MetaNode& currentMeta = metaNodes[node.mType];
        
    if (ImGui::CollapsingHeader("Samplers", 0))
    {
        URChange<std::vector<InputSampler> > undoRedoSampler(int(index)
            , [](int index) { return &gNodeDelegate.mNodes[index].mInputSamplers; }
            , [](int index) { auto& node = gNodeDelegate.mNodes[index]; gEvaluation.SetEvaluationSampler(index, node.mInputSamplers);});
            
        for (size_t i = 0; i < node.mInputSamplers.size();i++)
        {
            InputSampler& inputSampler = node.mInputSamplers[i];
            static const char *wrapModes = { "REPEAT\0CLAMP_TO_EDGE\0CLAMP_TO_BORDER\0MIRRORED_REPEAT" };
            static const char *filterModes = { "LINEAR\0NEAREST" };
            ImGui::PushItemWidth(150);
            ImGui::Text("Sampler %d", i);
            samplerDirty |= ImGui::Combo("Wrap U", (int*)&inputSampler.mWrapU, wrapModes);
            samplerDirty |= ImGui::Combo("Wrap V", (int*)&inputSampler.mWrapV, wrapModes);
            samplerDirty |= ImGui::Combo("Filter Min", (int*)&inputSampler.mFilterMin, filterModes);
            samplerDirty |= ImGui::Combo("Filter Mag", (int*)&inputSampler.mFilterMag, filterModes);
            ImGui::PopItemWidth();
        }
        if (samplerDirty)
        {
            gEvaluation.SetEvaluationSampler(index, node.mInputSamplers);
        }
        else
        {
            undoRedoSampler.Discard();
        }

    }
    if (!ImGui::CollapsingHeader(currentMeta.mName.c_str(), 0, ImGuiTreeNodeFlags_DefaultOpen))
        return;

    URChange<std::vector<unsigned char> > undoRedoParameter(int(index)
        , [](int index) { return &gNodeDelegate.mNodes[index].mParameters; }
        , UpdateDirtyParameter);

    unsigned char *paramBuffer = node.mParameters.data();
    int i = 0;
    for(const MetaParameter& param : currentMeta.mParams)
    {
        ImGui::PushID(667889 + i);
        
        dirty |= EditSingleParameter(unsigned int(index), i, paramBuffer, param);

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

void TileNodeEditGraphDelegate::SetTimeSlot(size_t index, int frameStart, int frameEnd)
{
    ImogenNode & node = mNodes[index];
    node.mStartFrame = frameStart;
    node.mEndFrame = frameEnd;
}

void TileNodeEditGraphDelegate::SetTimeDuration(size_t index, int duration)
{
    ImogenNode & node = mNodes[index];
    node.mEndFrame = node.mStartFrame + duration;
}

void TileNodeEditGraphDelegate::SetTime(int time, bool updateDecoder)
{
    gEvaluationTime = time;
    for (size_t i = 0; i < mNodes.size(); i++)
    {
        const ImogenNode& node = mNodes[i];
        gEvaluation.SetStageLocalTime(i, ImClamp(time - node.mStartFrame, 0, node.mEndFrame - node.mStartFrame), updateDecoder);
        //bool enabled = time >= node.mStartFrame && time <= node.mEndFrame;
    }
}

void TileNodeEditGraphDelegate::DoForce()
{
    int currentTime = gEvaluationTime;
    for (size_t i = 0; i < mNodes.size(); i++)
    {
        const ImogenNode& node = mNodes[i];
        const MetaNode& currentMeta = gMetaNodes[node.mType];
        bool forceEval = false;
        for(auto& param : currentMeta.mParams)
        {
            if (!param.mName.c_str())
                break;
            if (param.mType == Con_ForceEvaluate)
            {
                forceEval = true;
                break;
            }
        }
        if (forceEval)
        {
            EvaluationContext writeContext(gEvaluation, true, 1024, 1024);
            gCurrentContext = &writeContext;
            for (int frame = node.mStartFrame; frame <= node.mEndFrame; frame++)
            {
                SetTime(frame, false);
                ApplyAnimation(frame);
                EvaluationInfo evaluationInfo;
                evaluationInfo.forcedDirty = 1;
                evaluationInfo.uiPass = 0;
                writeContext.RunSingle(i, evaluationInfo);
            }
            gCurrentContext = &mEditingContext;
        }
    }
    SetTime(currentTime, true);
    InvalidateParameters();
}

void TileNodeEditGraphDelegate::InvalidateParameters()
{
    for (size_t i= 0;i<mNodes.size();i++)
    {
        auto& node = mNodes[i];
        gEvaluation.SetEvaluationParameters(i, node.mParameters);
    }
}

template<typename T> static inline T nmin(T lhs, T rhs) { return lhs >= rhs ? rhs : lhs; }

void TileNodeEditGraphDelegate::SetMouse(float rx, float ry, float dx, float dy, bool lButDown, bool rButDown, float wheel)
{
    if (mSelectedNodeIndex == -1)
        return;

    if (!lButDown)
        mbMouseDragging = false;

    const MetaNode* metaNodes = gMetaNodes.data();
    size_t res = 0;
    const MetaNode& metaNode = metaNodes[mNodes[mSelectedNodeIndex].mType];

    unsigned char *paramBuffer = mNodes[mSelectedNodeIndex].mParameters.data();
    bool parametersUseMouse = false;

    // camera handling
    for (auto& param : metaNode.mParams)
    {
        float *paramFlt = (float*)paramBuffer;
        if (param.mType == Con_Camera)
        {
            Camera *cam = (Camera*)paramBuffer;
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
                    cam->mPosition += (cam->mDirection * io.MouseDelta.y)*0.01f;
                }
                if (io.MouseDown[0])
                {
                    Mat4x4 tr, rtUp, rtRight, trp;
                    tr.Translation(-(cam->mPosition ));
                    rtRight.RotationAxis(right, io.MouseDelta.y * 0.01f);
                    rtUp.RotationAxis(cam->mUp, -io.MouseDelta.x * 0.01f);
                    trp.Translation((cam->mPosition ));
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
    
    paramBuffer = mNodes[mSelectedNodeIndex].mParameters.data();
    if (lButDown)
    {
        for(auto& param : metaNode.mParams)
        {
            float *paramFlt = (float*)paramBuffer;
            if (param.mType == Con_Camera)
            {
                Camera *cam = (Camera*)paramBuffer;
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
                    paramFlt[0] += ImLerp(param.mRangeMinX, param.mRangeMaxX, dx);
                    paramFlt[0] = fmodf(paramFlt[0], fabsf(param.mRangeMaxX - param.mRangeMinX)) + nmin(param.mRangeMinX, param.mRangeMaxX);
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
                    paramFlt[1] += ImLerp(param.mRangeMinY, param.mRangeMaxY, dy);
                    paramFlt[1] = fmodf(paramFlt[1], fabsf(param.mRangeMaxY - param.mRangeMinY)) + nmin(param.mRangeMinY, param.mRangeMaxY);
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
        gEvaluation.SetMouse(mSelectedNodeIndex, rx, ry, lButDown, rButDown);
        gEvaluation.SetEvaluationParameters(mSelectedNodeIndex, mNodes[mSelectedNodeIndex].mParameters);
        mEditingContext.SetTargetDirty(mSelectedNodeIndex);
    }
}

size_t TileNodeEditGraphDelegate::ComputeNodeParametersSize(size_t nodeTypeIndex)
{
    size_t res = 0;
    for(auto& param : gMetaNodes[nodeTypeIndex].mParams)
    {
        res += GetParameterTypeSize(param.mType);
    }
    return res;
}

bool TileNodeEditGraphDelegate::NodeIs2D(size_t nodeIndex)
{
    auto target = mEditingContext.GetRenderTarget(nodeIndex);
    if (target)
        return target->mImage.mNumFaces == 1;
    return false;
}

bool TileNodeEditGraphDelegate::NodeIsCompute(size_t nodeIndex)
{
    /*auto buffer = mEditingContext.GetComputeBuffer(nodeIndex);
    if (buffer)
        return true;
    return false;
    */
    return (gEvaluators.GetMask(mNodes[nodeIndex].mType) & EvaluationGLSLCompute) != 0;
}

bool TileNodeEditGraphDelegate::NodeIsCubemap(size_t nodeIndex)
{
    auto target = mEditingContext.GetRenderTarget(nodeIndex);
    if (target)
        return target->mImage.mNumFaces == 6;
    return false;
}

ImVec2 TileNodeEditGraphDelegate::GetEvaluationSize(size_t nodeIndex)
{
    int imageWidth(1), imageHeight(1);
    gEvaluation.GetEvaluationSize(int(nodeIndex), &imageWidth, &imageHeight);
    return ImVec2(float(imageWidth), float(imageHeight));
}

void TileNodeEditGraphDelegate::CopyNodes(const std::vector<size_t> nodes)
{
    mNodesClipboard.clear();
    for (auto nodeIndex : nodes)
        mNodesClipboard.push_back(mNodes[nodeIndex]);
}

void TileNodeEditGraphDelegate::CutNodes(const std::vector<size_t> nodes)
{
    mNodesClipboard.clear();
}

void TileNodeEditGraphDelegate::PasteNodes()
{
    for (auto& sourceNode : mNodesClipboard)
    {
        URAdd<ImogenNode> undoRedoAddNode(int(mNodes.size()), []() {return &gNodeDelegate.mNodes; },
            NodeIsDeleted, NodeIsAdded);

        mEditingContext.UserAddStage();
        gEvaluation.UserAddEvaluation(sourceNode.mType);
        size_t target = mNodes.size();
        AddSingleNode(sourceNode.mType);
        auto& node = mNodes.back();
        node.mParameters = sourceNode.mParameters;
        node.mInputSamplers = sourceNode.mInputSamplers;
        node.mStartFrame = sourceNode.mStartFrame;
        node.mEndFrame = sourceNode.mEndFrame;
        
        gEvaluation.SetEvaluationParameters(target, node.mParameters);
        gEvaluation.SetEvaluationSampler(target, node.mInputSamplers);
        SetTime(gEvaluationTime, true);
        mEditingContext.SetTargetDirty(target);
    }
}

// animation
AnimTrack* TileNodeEditGraphDelegate::GetAnimTrack(uint32_t nodeIndex, uint32_t parameterIndex)
{
    for (auto& animTrack : mAnimTrack)
    {
        if (animTrack.mNodeIndex == nodeIndex && animTrack.mParamIndex == parameterIndex)
            return &animTrack;
    }
    return NULL;
}

void TileNodeEditGraphDelegate::MakeKey(int frame, uint32_t nodeIndex, uint32_t parameterIndex)
{
    URDummy urDummy;

    AnimTrack* animTrack = GetAnimTrack(nodeIndex, parameterIndex);
    if (!animTrack)
    {
        URAdd<AnimTrack> urAdd(int(mAnimTrack.size()), [] { return &gNodeDelegate.mAnimTrack; });
        uint32_t parameterType = gMetaNodes[mNodes[nodeIndex].mType].mParams[parameterIndex].mType;
        AnimTrack newTrack;
        newTrack.mNodeIndex = nodeIndex;
        newTrack.mParamIndex = parameterIndex;
        newTrack.mValueType = parameterType;
        newTrack.mAnimation = AllocateAnimation(parameterType);
        mAnimTrack.push_back(newTrack);
        animTrack = &mAnimTrack.back();
    }
    URChange<AnimTrack> urChange(int(animTrack - mAnimTrack.data()), [](int index) {return &gNodeDelegate.mAnimTrack[index]; });
    auto& node = mNodes[nodeIndex];
    size_t parameterOffset = GetParameterOffset(uint32_t(node.mType), parameterIndex);
    animTrack->mAnimation->SetValue(frame, &node.mParameters[parameterOffset]);
}

void TileNodeEditGraphDelegate::GetKeyedParameters(int frame, uint32_t nodeIndex, std::vector<bool>& keyed)
{

}

void TileNodeEditGraphDelegate::ApplyAnimationForNode(size_t nodeIndex, int frame)
{
    bool animatedNodes = false;
    auto& node = mNodes[nodeIndex];
    for (auto& animTrack : mAnimTrack)
    {
        if (animTrack.mNodeIndex == nodeIndex)
        {
            size_t parameterOffset = GetParameterOffset(uint32_t(node.mType), animTrack.mParamIndex);
            animTrack.mAnimation->GetValue(frame, &node.mParameters[parameterOffset]);

            animatedNodes = true;
        }
    }
    if (animatedNodes)
    {
        gEvaluation.SetEvaluationParameters(nodeIndex, node.mParameters);
        gCurrentContext->SetTargetDirty(nodeIndex);
    }
}

void TileNodeEditGraphDelegate::ApplyAnimation(int frame)
{
    std::vector<bool> animatedNodes;
    animatedNodes.resize(mNodes.size(), false);
    for (auto& animTrack : mAnimTrack)
    {
        auto& node = mNodes[animTrack.mNodeIndex];
        animatedNodes[animTrack.mNodeIndex] = true;
        size_t parameterOffset = GetParameterOffset(uint32_t(node.mType), animTrack.mParamIndex);
        animTrack.mAnimation->GetValue(frame, &node.mParameters[parameterOffset]);
    }
    for (size_t i = 0; i < animatedNodes.size(); i++)
    {
        if (!animatedNodes[i])
            continue;
        gEvaluation.SetEvaluationParameters(i, mNodes[i].mParameters);
        gCurrentContext->SetTargetDirty(i);
    }
}

void TileNodeEditGraphDelegate::RemoveAnimation(size_t nodeIndex)
{
    if (mAnimTrack.empty())
        return;
    std::vector<int> tracks;
    for (int i = 0; i < int(mAnimTrack.size()); i++)
    {
        const AnimTrack& animTrack = mAnimTrack[i];
        if (animTrack.mNodeIndex == nodeIndex)
            tracks.push_back(i);
    }
    if (tracks.empty())
        return;

    for (int i = 0; i < int(tracks.size()); i++)
    {
        int index = tracks[i] - i;
        URDel<AnimTrack> urDel(index, [] { return &gNodeDelegate.mAnimTrack; });
        mAnimTrack.erase(mAnimTrack.begin() + index);
    }
}

void TileNodeEditGraphDelegate::RemovePins(size_t nodeIndex)
{
    auto iter = mPinnedParameters.begin();
    for (; iter != mPinnedParameters.end();)
    {
        uint32_t pin = *iter;
        if ( ((pin >> 16) & 0xFFFF) == nodeIndex)
            iter = mPinnedParameters.erase(iter);
        else
            ++iter;
    }
}

Camera *TileNodeEditGraphDelegate::GetCameraParameter(size_t index)
{
    if (index >= mNodes.size()) 
        return NULL;
    ImogenNode& node = mNodes[index];
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[node.mType];
    const size_t paramsSize = ComputeNodeParametersSize(node.mType);
    node.mParameters.resize(paramsSize);
    unsigned char *paramBuffer = node.mParameters.data();
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (param.mType == Con_Camera)
        {
            Camera *cam = (Camera*)paramBuffer;
            return cam;
        }
        paramBuffer += GetParameterTypeSize(param.mType);
    }

    return NULL;
}

float TileNodeEditGraphDelegate::GetParameterComponentValue(size_t index, int parameterIndex, int componentIndex)
{
    auto& node = mNodes[index];
    size_t paramOffset = GetParameterOffset(uint32_t(node.mType), parameterIndex);
    unsigned char *ptr = &node.mParameters.data()[paramOffset];
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[node.mType];
    switch (currentMeta.mParams[parameterIndex].mType)
    {
    case Con_Angle:
    case Con_Float:
        return ((float*)ptr)[componentIndex];
    case Con_Angle2:
    case Con_Float2:
        return ((float*)ptr)[componentIndex];
    case Con_Angle3:
    case Con_Float3:
        return ((float*)ptr)[componentIndex];
    case Con_Angle4:
    case Con_Color4:
    case Con_Float4:
        return ((float*)ptr)[componentIndex];
    case Con_Ramp:
        return 0;
    case Con_Ramp4:
        return 0;
    case Con_Enum:
    case Con_Int:
        return float(((int*)ptr)[componentIndex]);
    case Con_Int2:
        return float(((int*)ptr)[componentIndex]);
    case Con_FilenameRead:
    case Con_FilenameWrite:
        return 0;
    case Con_ForceEvaluate:
        return 0;
    case Con_Bool:
        return float(((bool*)ptr)[componentIndex]);
    case Con_Camera:
        return float((*(Camera*)ptr)[componentIndex]);
    }
    return 0.f;
}

void TileNodeEditGraphDelegate::SetAnimTrack(const std::vector<AnimTrack>& animTrack) 
{ 
    mAnimTrack = animTrack; 
}
