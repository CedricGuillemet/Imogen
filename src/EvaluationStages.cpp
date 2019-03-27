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

#include "EvaluationStages.h"
#include "EvaluationContext.h"
#include "Evaluators.h"
#include <vector>
#include <algorithm>
#include <map>
#include <GL/gl3w.h>

EvaluationStages::EvaluationStages() : mFrameMin(0), mFrameMax(1)
{
    
}

void EvaluationStages::AddSingleEvaluation(size_t nodeType)
{
    EvaluationStage evaluation;
#ifdef _DEBUG
    evaluation.mTypename              = gMetaNodes[nodeType].mName;;
#endif
    evaluation.mDecoder               = NULL;
    evaluation.mUseCountByOthers      = 0;
    evaluation.mType                  = nodeType;
    evaluation.mParametersBuffer      = 0;
    evaluation.mBlendingSrc           = ONE;
    evaluation.mBlendingDst           = ZERO;
    evaluation.mLocalTime             = 0;
    evaluation.gEvaluationMask        = gEvaluators.GetMask(nodeType);
    evaluation.mbDepthBuffer          = false;
    evaluation.mbClearBuffer          = false;
    evaluation.mVertexSpace           = 0;
    static std::shared_ptr<Scene> defaultScene;
    if (!defaultScene)
    {
        defaultScene = std::make_shared<Scene>();
        defaultScene->mMeshes.resize(1);
        auto& mesh = defaultScene->mMeshes.back();
        mesh.mPrimitives.resize(1);
        auto& prim = mesh.mPrimitives.back();
        static const float fsVts[] = { 0.f,0.f, 2.f,0.f, 0.f,2.f };
        prim.AddBuffer(fsVts, Scene::Mesh::Format::UV, 2*sizeof(float), 3);
        // add node and transform
        defaultScene->mWorldTransforms.resize(1);
        defaultScene->mWorldTransforms[0].Identity();
        defaultScene->mMeshIndex.resize(1, 0);
    }
    evaluation.mGScene                = defaultScene;
    evaluation.renderer               = nullptr;
    evaluation.mRuntimeUniqueId       = GetRuntimeId();
    const size_t inputCount = gMetaNodes[nodeType].mInputs.size();
    evaluation.mInputSamplers.resize(inputCount);
    evaluation.mStartFrame            = mFrameMin;
    evaluation.mEndFrame              = mFrameMax;

    InitDefaultParameters(evaluation);
    mStages.push_back(evaluation);
}

void EvaluationStages::StageIsAdded(int index)
{
    for (size_t i = 0;i< mStages.size();i++)
    {
        if (i == index)
            continue;
        auto& evaluation = mStages[i];
        for (auto& inp : evaluation.mInput.mInputs)
        {
            if (inp >= index)
                inp++;
        }
    }
}

void EvaluationStages::StageIsDeleted(int index)
{
    EvaluationStage& ev = mStages[index];
    ev.Clear();

    // shift all connections
    for (auto& evaluation : mStages)
    {
        for (auto& inp : evaluation.mInput.mInputs)
        {
            if (inp >= index)
                inp--;
        }
    }
}

void EvaluationStages::UserAddEvaluation(size_t nodeType)
{
    URAdd<EvaluationStage> undoRedoAddStage(int(mStages.size()), [&]() {return &mStages; },
        [&](int index) {StageIsDeleted(index); }, [&](int index) {StageIsAdded(index); });

    AddSingleEvaluation(nodeType);
}

void EvaluationStages::UserDeleteEvaluation(size_t target)
{
    URDel<EvaluationStage> undoRedoDelStage(int(target), [&]() {return &mStages; },
        [&](int index) {StageIsDeleted(index); }, [&](int index) {StageIsAdded(index); });

    StageIsDeleted(int(target));
    mStages.erase(mStages.begin() + target);
}

void EvaluationStages::SetEvaluationParameters(size_t target, const std::vector<unsigned char> &parameters)
{
    EvaluationStage& stage = mStages[target];
    stage.mParameters = parameters;

    if (stage.gEvaluationMask&EvaluationGLSL)
        BindGLSLParameters(stage);
    if (stage.mDecoder)
        stage.mDecoder = NULL;
}

void EvaluationStages::SetEvaluationSampler(size_t target, const std::vector<InputSampler>& inputSamplers)
{
    mStages[target].mInputSamplers = inputSamplers;
}

void EvaluationStages::AddEvaluationInput(size_t target, int slot, int source)
{
    if (mStages[target].mInput.mInputs[slot] == source)
        return;
    mStages[target].mInput.mInputs[slot] = source;
    mStages[source].mUseCountByOthers++;
}

void EvaluationStages::DelEvaluationInput(size_t target, int slot)
{
    mStages[mStages[target].mInput.mInputs[slot]].mUseCountByOthers--;
    mStages[target].mInput.mInputs[slot] = -1;
}

void EvaluationStages::SetEvaluationOrder(const std::vector<size_t> nodeOrderList)
{
    mEvaluationOrderList = nodeOrderList;
}

void EvaluationStages::Clear()
{
    for (auto& ev : mStages)
        ev.Clear();

    mStages.clear();
    mEvaluationOrderList.clear();
    mAnimTrack.clear();
}

void EvaluationStages::SetMouse(int target, float rx, float ry, bool lButDown, bool rButDown)
{
    for (auto& ev : mStages)
    {
        ev.mRx = -9999.f;
        ev.mRy = -9999.f;
        ev.mLButDown = false;
        ev.mRButDown = false;
    }
    auto& ev = mStages[target];
    ev.mRx = rx;
    ev.mRy = 1.f - ry; // inverted for UI
    ev.mLButDown = lButDown;
    ev.mRButDown = rButDown;
}

size_t EvaluationStages::GetEvaluationImageDuration(size_t target)
{
    auto& stage = mStages[target];
    if (!stage.mDecoder)
        return 1;
    if (stage.mDecoder->mFrameCount > 2000)
    {
        int a = 1;
    }
    return stage.mDecoder->mFrameCount;
}

void EvaluationStages::SetStageLocalTime(EvaluationContext *evaluationContext, size_t target, int localTime, bool updateDecoder)
{
    auto& stage = mStages[target];
    int newLocalTime = ImMin(localTime, int(GetEvaluationImageDuration(target)));
    if (stage.mDecoder && updateDecoder && stage.mLocalTime != newLocalTime)
    {
        stage.mLocalTime = newLocalTime;
        Image image = stage.DecodeImage();
        EvaluationAPI::SetEvaluationImage(evaluationContext, int(target), &image);
        Image::Free(&image);
    }
    else
    {
        stage.mLocalTime = newLocalTime;
    }
}

FFMPEGCodec::Decoder* EvaluationStages::FindDecoder(const std::string& filename)
{
    for (auto& evaluation : mStages)
    {
        if (evaluation.mDecoder && evaluation.mDecoder->GetFilename() == filename)
            return evaluation.mDecoder.get();
    }
    auto decoder = new FFMPEGCodec::Decoder;
    decoder->Open(filename);
    return decoder;
}

Camera *EvaluationStages::GetCameraParameter(size_t index)
{
    if (index >= mStages.size())
        return NULL;
    EvaluationStage& stage = mStages[index];
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[stage.mType];
    const size_t paramsSize = ComputeNodeParametersSize(stage.mType);
    stage.mParameters.resize(paramsSize);
    unsigned char *paramBuffer = stage.mParameters.data();
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
// TODO : create parameter struct with templated accessors
int EvaluationStages::GetIntParameter(size_t index, const char *parameterName, int defaultValue)
{
    if (index >= mStages.size())
        return NULL;
    EvaluationStage& stage = mStages[index];
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[stage.mType];
    const size_t paramsSize = ComputeNodeParametersSize(stage.mType);
    stage.mParameters.resize(paramsSize);
    unsigned char *paramBuffer = stage.mParameters.data();
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (param.mType == Con_Int)
        {
            if (!strcmp(param.mName.c_str(), parameterName))
            {
                int *value = (int*)paramBuffer;
                return *value;
            }
        }
        paramBuffer += GetParameterTypeSize(param.mType);
    }
    return defaultValue;
}

void EvaluationStages::InitDefaultParameters(EvaluationStage& stage)
{
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[stage.mType];
    const size_t paramsSize = ComputeNodeParametersSize(stage.mType);
    stage.mParameters.resize(paramsSize);
    unsigned char *paramBuffer = stage.mParameters.data();
    memset(paramBuffer, 0, paramsSize);
    int i = 0;
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (!param.mDefaultValue.empty())
        {
            memcpy(paramBuffer, param.mDefaultValue.data(), param.mDefaultValue.size());
        }

        paramBuffer += GetParameterTypeSize(param.mType);
    }
}

Image EvaluationStage::DecodeImage()
{
    return Image::DecodeImage(mDecoder.get(), mLocalTime);
}

void EvaluationStages::BindGLSLParameters(EvaluationStage& stage)
{
    if (!stage.mParametersBuffer)
    {
        glGenBuffers(1, &stage.mParametersBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, stage.mParametersBuffer);

        glBufferData(GL_UNIFORM_BUFFER, stage.mParameters.size(), stage.mParameters.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, stage.mParametersBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
    else
    {
        glBindBuffer(GL_UNIFORM_BUFFER, stage.mParametersBuffer);
        glBufferData(GL_UNIFORM_BUFFER, stage.mParameters.size(), stage.mParameters.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

void EvaluationStages::ApplyAnimationForNode(EvaluationContext *context, size_t nodeIndex, int frame)
{
    bool animatedNodes = false;
    EvaluationStage& stage = mStages[nodeIndex];
    for (auto& animTrack : mAnimTrack)
    {
        if (animTrack.mNodeIndex == nodeIndex)
        {
            size_t parameterOffset = GetParameterOffset(uint32_t(stage.mType), animTrack.mParamIndex);
            animTrack.mAnimation->GetValue(frame, &stage.mParameters[parameterOffset]);

            animatedNodes = true;
        }
    }
    if (animatedNodes)
    {
        SetEvaluationParameters(nodeIndex, stage.mParameters);
        context->SetTargetDirty(nodeIndex);
    }
}

void EvaluationStages::ApplyAnimation(EvaluationContext *context, int frame)
{
    std::vector<bool> animatedNodes;
    animatedNodes.resize(mStages.size(), false);
    for (auto& animTrack : mAnimTrack)
    {
        EvaluationStage& stage = mStages[animTrack.mNodeIndex];

        animatedNodes[animTrack.mNodeIndex] = true;
        size_t parameterOffset = GetParameterOffset(uint32_t(stage.mType), animTrack.mParamIndex);
        animTrack.mAnimation->GetValue(frame, &stage.mParameters[parameterOffset]);
    }
    for (size_t i = 0; i < animatedNodes.size(); i++)
    {
        if (!animatedNodes[i])
            continue;
        SetEvaluationParameters(i, mStages[i].mParameters);
        context->SetTargetDirty(i);
    }
}

void EvaluationStages::RemoveAnimation(size_t nodeIndex)
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
        URDel<AnimTrack> urDel(index, [&] { return &mAnimTrack; });
        mAnimTrack.erase(mAnimTrack.begin() + index);
    }
}

void EvaluationStages::RemovePins(size_t nodeIndex)
{
    auto iter = mPinnedParameters.begin();
    for (; iter != mPinnedParameters.end();)
    {
        uint32_t pin = *iter;
        if (((pin >> 16) & 0xFFFF) == nodeIndex)
        {
            URDel<uint32_t> undoRedoDelPin(int(&(*iter) - mPinnedParameters.data()), [&]() {return &mPinnedParameters; });
            iter = mPinnedParameters.erase(iter);
        }
        else
            ++iter;
    }
}

float EvaluationStages::GetParameterComponentValue(size_t index, int parameterIndex, int componentIndex)
{
    EvaluationStage& stage = mStages[index];
    size_t paramOffset = GetParameterOffset(uint32_t(stage.mType), parameterIndex);
    unsigned char *ptr = &stage.mParameters.data()[paramOffset];
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[stage.mType];
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

void EvaluationStages::SetAnimTrack(const std::vector<AnimTrack>& animTrack)
{
    mAnimTrack = animTrack;
}

void EvaluationStages::SetTime(EvaluationContext *evaluationContext, int time, bool updateDecoder)
{
    for (size_t i = 0; i < mStages.size(); i++)
    {
        const auto& stage = mStages[i];
        SetStageLocalTime(evaluationContext, i, ImClamp(time - stage.mStartFrame, 0, stage.mEndFrame - stage.mStartFrame), updateDecoder);
        //bool enabled = time >= node.mStartFrame && time <= node.mEndFrame;
        evaluationContext->SetTargetDirty(i);
    }
}

void EvaluationStage::Clear()
{
    if (gEvaluationMask&EvaluationGLSL)
        glDeleteBuffers(1, &mParametersBuffer);
    mParametersBuffer = 0;
}

///////////////////////////////////////////////////////////////////////////////////////

void Scene::Mesh::Primitive::Draw() const
{
    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    for (auto& buffer : mBuffers)
    {
        glBindBuffer(GL_ARRAY_BUFFER, buffer.id);
        switch (buffer.format)
        {
        case Format::UV:
            glVertexAttribPointer(SemUV0, 2, GL_FLOAT, GL_FALSE, 8, 0);
            glEnableVertexAttribArray(SemUV0);
            break;
        case Format::COL:
            glVertexAttribPointer(SemUV0+1, 4, GL_FLOAT, GL_FALSE, 16, 0);
            glEnableVertexAttribArray(SemUV0+1);
            break;
        case Format::POS:
            glVertexAttribPointer(SemUV0 + 2, 3, GL_FLOAT, GL_FALSE, 12, 0);
            glEnableVertexAttribArray(SemUV0 + 2);
            break;
        case Format::NORM:
            glVertexAttribPointer(SemUV0 + 3, 3, GL_FLOAT, GL_FALSE, 12, 0);
            glEnableVertexAttribArray(SemUV0 + 3);
            break;
        }
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    

    glBindVertexArray(vao);

    if (!mIndexBuffer.id)
    {
        glDrawArrays(GL_TRIANGLES, 0, mBuffers[0].count);
    }
    else
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer.id);
        glDrawElements(GL_TRIANGLES, mIndexBuffer.count, (mIndexBuffer.stride==4)?GL_UNSIGNED_INT:GL_UNSIGNED_SHORT, (void*)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDeleteVertexArrays(1, &vao);
}
void Scene::Mesh::Primitive::AddBuffer(const void *data, unsigned int format, unsigned int stride, unsigned int count)
{
    unsigned int va;
    glGenBuffers(1, &va);
    glBindBuffer(GL_ARRAY_BUFFER, va);
    glBufferData(GL_ARRAY_BUFFER, stride * count, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    mBuffers.push_back({ va, format, stride, count });
}

void Scene::Mesh::Primitive::AddIndexBuffer(const void *data, unsigned int stride, unsigned int count)
{
    unsigned int ia;
    glGenBuffers(1, &ia);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ia);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, stride * count, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    mIndexBuffer = { ia, stride, count };
}
void Scene::Mesh::Draw() const
{
    for (auto& prim : mPrimitives)
    {
        prim.Draw();
    }
}
void Scene::Draw(EvaluationInfo& evaluationInfo) const
{
    for (unsigned int i = 0; i < mMeshIndex.size(); i++)
    {
        int index = mMeshIndex[i];
        if (index == -1)
            continue;

        glBindBuffer(GL_UNIFORM_BUFFER, gEvaluators.gEvaluationStateGLSLBuffer);
        memcpy(evaluationInfo.model, mWorldTransforms[i], sizeof(Mat4x4));
        FPU_MatrixF_x_MatrixF(evaluationInfo.model, evaluationInfo.viewProjection, evaluationInfo.modelViewProjection);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(EvaluationInfo), &evaluationInfo, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        mMeshes[index].Draw();
    }
}

Scene::~Scene()
{
    // todo : clear ia/va
}