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

EvaluationStages::EvaluationStages() : mFrameMin(0), mFrameMax(1), mInputNodeIndex(-1)
{
}

void EvaluationStages::AddSingleEvaluation(size_t nodeType)
{
    EvaluationStage evaluation;
    //#ifdef _DEBUG needed for fur
    evaluation.mTypename = gMetaNodes[nodeType].mName;
    //#endif
    evaluation.mDecoder = NULL;
    evaluation.mUseCountByOthers = 0;
    evaluation.mType = nodeType;
    evaluation.mBlendingSrc = ONE;
    evaluation.mBlendingDst = ZERO;
    evaluation.mLocalTime = 0;
    evaluation.gEvaluationMask = gEvaluators.GetMask(nodeType);
    evaluation.mbDepthBuffer = false;
    evaluation.mbClearBuffer = false;
    evaluation.mVertexSpace = 0;
    static std::shared_ptr<Scene> defaultScene;
    if (!defaultScene)
    {
        defaultScene = std::make_shared<Scene>();
        defaultScene->mMeshes.resize(1);
        auto& mesh = defaultScene->mMeshes.back();
        mesh.mPrimitives.resize(1);
        auto& prim = mesh.mPrimitives.back();
        static const float fsVts[] = {0.f, 0.f, 2.f, 0.f, 0.f, 2.f};
        prim.AddBuffer(fsVts, Scene::Mesh::Format::UV, 2 * sizeof(float), 3);
        // add node and transform
        defaultScene->mWorldTransforms.resize(1);
        defaultScene->mWorldTransforms[0].Identity();
        defaultScene->mMeshIndex.resize(1, 0);
    }
    evaluation.mScene = nullptr;
    evaluation.mGScene = defaultScene;
    evaluation.renderer = nullptr;
    evaluation.mRuntimeUniqueId = GetRuntimeId();
    const size_t inputCount = gMetaNodes[nodeType].mInputs.size();
    evaluation.mStartFrame = mFrameMin;
    evaluation.mEndFrame = mFrameMax;

    mParameters.push_back(Parameters());
    InitDefaultParameters(evaluation, mParameters.back());
    mStages.push_back(evaluation);
    mPinnedIO.push_back(0);
    mPinnedParameters.push_back(0);
    mInputSamplers.push_back(Samplers(inputCount));
    mMultiplexInputs.push_back(MultiplexInput());
}

void EvaluationStages::StageIsAdded(size_t nodeIndex)
{
    for (size_t i = 0; i < mStages.size(); i++)
    {
        if (i == nodeIndex)
            continue;
        auto& evaluation = mStages[i];
        for (auto& inp : evaluation.mInput.mInputs)
        {
            if (inp >= int(nodeIndex))
            {
                inp++;
            }
        }
    }
}

void EvaluationStages::StageIsDeleted(size_t nodeIndex)
{
    EvaluationStage& ev = mStages[nodeIndex];

    // shift all connections
    for (auto& evaluation : mStages)
    {
        for (auto& inp : evaluation.mInput.mInputs)
        {
            if (inp >= nodeIndex)
                inp--;
        }
    }
}
/*
void EvaluationStages::UserAddEvaluation(size_t nodeType)
{
    URAdd<EvaluationStage> undoRedoAddStage(int(mStages.size()),
                                            [&]() { return &mStages; },
                                            [&](int index) { StageIsDeleted(index); },
                                            [&](int index) { StageIsAdded(index); });

    AddSingleEvaluation(nodeType);
}

void EvaluationStages::UserDeleteEvaluation(size_t target)
{
    URDel<EvaluationStage> undoRedoDelStage(int(target),
                                            [&]() { return &mStages; },
                                            [&](int index) { StageIsDeleted(index); },
                                            [&](int index) { StageIsAdded(index); });

    StageIsDeleted(int(target));
    mStages.erase(mStages.begin() + target);
}
*/
void EvaluationStages::SetEvaluationParameters(size_t nodeIndex, const Parameters& parameters)
{
    EvaluationStage& stage = mStages[nodeIndex];
    mParameters[nodeIndex] = parameters;

    if (stage.mDecoder)
        stage.mDecoder = NULL;
}

void EvaluationStages::SetSamplers(size_t nodeIndex, const std::vector<InputSampler>& inputSamplers)
{
    mInputSamplers[nodeIndex] = inputSamplers;
}

void EvaluationStages::AddEvaluationInput(size_t target, int slot, int source)
{
    if (mStages.size() <= target || mStages[target].mInput.mInputs[slot] == source)
        return;
    mStages[target].mInput.mInputs[slot] = source;
    mStages[source].mUseCountByOthers++;
}

void EvaluationStages::DelEvaluationInput(size_t target, int slot)
{
    mStages[mStages[target].mInput.mInputs[slot]].mUseCountByOthers--;
    mStages[target].mInput.mInputs[slot] = -1;
}

void EvaluationStages::SetEvaluationOrder(const std::vector<size_t>& nodeOrderList)
{
    mEvaluationOrderList = nodeOrderList;
}

void EvaluationStages::Clear()
{
    mEvaluationOrderList.clear();

    mStages.clear();
    mAnimTrack.clear();
    mPinnedIO.clear();
    mPinnedParameters.clear();
    mInputSamplers.clear();
    mParameters.clear();
}

void EvaluationStages::SetKeyboardMouse(size_t nodeIndex, const UIInput& input)
{
    /*for (auto& ev : mStages)
    {
        ev.mRx = -9999.f;
        ev.mRy = -9999.f;
        ev.mLButDown = false;
        ev.mRButDown = false;
        ev.mbCtrl = false;
        ev.mbAlt = false;
        ev.mbShift = false;
    }
    auto& ev = mStages[nodeIndex];
    ev.mRx = rx;
    ev.mRy = 1.f - ry; // inverted for UI
    ev.mLButDown = lButDown;
    ev.mRButDown = rButDown;
    ev.mbCtrl = bCtrl;
    ev.mbAlt = bAlt;
    ev.mbShift = bShift;
*/
    mInputs = input;
    mInputNodeIndex = nodeIndex;
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

void EvaluationStages::SetStageLocalTime(EvaluationContext* evaluationContext,
                                         size_t target,
                                         int localTime,
                                         bool updateDecoder)
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

Camera* EvaluationStages::GetCameraParameter(size_t nodeIndex)
{
    if (nodeIndex >= mStages.size())
        return NULL;
    EvaluationStage& stage = mStages[nodeIndex];
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[stage.mType];
    const size_t paramsSize = ComputeNodeParametersSize(stage.mType);
    mParameters[nodeIndex].resize(paramsSize);
    unsigned char* paramBuffer = mParameters[nodeIndex].data();
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (param.mType == Con_Camera)
        {
            Camera* cam = (Camera*)paramBuffer;
            return cam;
        }
        paramBuffer += GetParameterTypeSize(param.mType);
    }

    return NULL;
}
// TODO : create parameter struct with templated accessors
int EvaluationStages::GetIntParameter(size_t nodeIndex, const char* parameterName, int defaultValue)
{
    if (nodeIndex >= mStages.size())
        return NULL;
    EvaluationStage& stage = mStages[nodeIndex];
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[stage.mType];
    const size_t paramsSize = ComputeNodeParametersSize(stage.mType);
    mParameters[nodeIndex].resize(paramsSize);
    unsigned char* paramBuffer = mParameters[nodeIndex].data();
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (param.mType == Con_Int)
        {
            if (!strcmp(param.mName.c_str(), parameterName))
            {
                int* value = (int*)paramBuffer;
                return *value;
            }
        }
        paramBuffer += GetParameterTypeSize(param.mType);
    }
    return defaultValue;
}

void EvaluationStages::InitDefaultParameters(const EvaluationStage& stage, Parameters& parameters)
{
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[stage.mType];
    const size_t paramsSize = ComputeNodeParametersSize(stage.mType);
    parameters.resize(paramsSize);
    unsigned char* paramBuffer = parameters.data();
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

void EvaluationStages::ApplyAnimationForNode(EvaluationContext* context, size_t nodeIndex, int frame)
{
    bool animatedNodes = false;
    EvaluationStage& stage = mStages[nodeIndex];
    Parameters parameters = mParameters[nodeIndex];
    for (auto& animTrack : mAnimTrack)
    {
        if (animTrack.mNodeIndex == nodeIndex)
        {
            size_t parameterOffset = GetParameterOffset(uint32_t(stage.mType), animTrack.mParamIndex);
            animTrack.mAnimation->GetValue(frame, &parameters[parameterOffset]);

            animatedNodes = true;
        }
    }
    if (animatedNodes)
    {
        SetEvaluationParameters(nodeIndex, parameters);
        context->SetTargetDirty(nodeIndex, Dirty::Parameter);
    }
}

void EvaluationStages::ApplyAnimation(EvaluationContext* context, int frame)
{
    std::vector<bool> animatedNodes;
    animatedNodes.resize(mStages.size(), false);
    for (auto& animTrack : mAnimTrack)
    {
        EvaluationStage& stage = mStages[animTrack.mNodeIndex];

        animatedNodes[animTrack.mNodeIndex] = true;
        size_t parameterOffset = GetParameterOffset(uint32_t(stage.mType), animTrack.mParamIndex);
        animTrack.mAnimation->GetValue(frame, &mParameters[animTrack.mNodeIndex][parameterOffset]);
    }
    for (size_t i = 0; i < animatedNodes.size(); i++)
    {
        if (!animatedNodes[i])
            continue;
        SetEvaluationParameters(i, mParameters[i]);
        context->SetTargetDirty(i, Dirty::Parameter);
    }
}

void EvaluationStages::RemovePins(size_t nodeIndex)
{
    mPinnedParameters.erase(mPinnedParameters.begin() + nodeIndex);
    mPinnedIO.erase(mPinnedIO.begin() + nodeIndex);
}

bool EvaluationStages::IsParameterPinned(size_t nodeIndex, size_t parameterIndex) const
{
    if (nodeIndex >= mPinnedParameters.size())
    {
        return false;
    }
    uint32_t mask = 1 << parameterIndex;
    return mPinnedParameters[nodeIndex] & mask;
}

void EvaluationStages::SetParameterPin(size_t nodeIndex, size_t parameterIndex, bool pinned)
{
    mPinnedParameters.resize(mStages.size(), false);
    uint32_t mask = 1 << parameterIndex;
    mPinnedParameters[nodeIndex] &= ~mask;
    mPinnedParameters[nodeIndex] += pinned ? mask : 0;
}

float EvaluationStages::GetParameterComponentValue(size_t nodeIndex, int parameterIndex, int componentIndex)
{
    EvaluationStage& stage = mStages[nodeIndex];
    size_t paramOffset = GetParameterOffset(uint32_t(stage.mType), parameterIndex);
    const unsigned char* ptr = &mParameters[nodeIndex].data()[paramOffset];
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

void EvaluationStages::SetTime(EvaluationContext* evaluationContext, int time, bool updateDecoder)
{
    for (size_t i = 0; i < mStages.size(); i++)
    {
        const auto& stage = mStages[i];
        SetStageLocalTime(evaluationContext,
                          i,
                          ImClamp(time - stage.mStartFrame, 0, stage.mEndFrame - stage.mStartFrame),
                          updateDecoder);
        // bool enabled = time >= node.mStartFrame && time <= node.mEndFrame;
        evaluationContext->SetTargetDirty(i, Dirty::Time);
    }
}

bool EvaluationStages::IsIOPinned(size_t nodeIndex, size_t io, bool forOutput) const
{
    if (nodeIndex >= mPinnedIO.size())
    {
        return false;
    }
    uint32_t mask = 0;
    if (forOutput)
    {
        mask = (1 << io) & 0xFF;
    }
    else
    {
        mask = (1 << (8 + io));
    }
    return mPinnedIO[nodeIndex] & mask;
}

void EvaluationStages::SetIOPin(size_t nodeIndex, size_t io, bool forOutput, bool pinned)
{
    mPinnedIO.resize(mStages.size(), 0);
	uint32_t mask = 0;
    if (forOutput)
    {
        mask = (1 << io) & 0xFF;
    }
    else
    {
        mask = (1 << (8 + io));
    }
    mPinnedIO[nodeIndex] &= ~mask;
    mPinnedIO[nodeIndex] += pinned ? mask : 0;
}


size_t EvaluationStages::PickBestNode(const std::vector<EvaluationStages::NodeOrder>& orders) const
{
    for (auto& order : orders)
    {
        if (order.mNodePriority == 0)
            return order.mNodeIndex;
    }
    // issue!
    assert(0);
    return -1;
}

void EvaluationStages::RecurseSetPriority(std::vector<EvaluationStages::NodeOrder>& orders,
    size_t currentIndex,
    size_t currentPriority,
    size_t& undeterminedNodeCount) const
{
    if (!orders[currentIndex].mNodePriority)
        undeterminedNodeCount--;

    orders[currentIndex].mNodePriority = std::max(orders[currentIndex].mNodePriority, currentPriority + 1);
    for (auto input : mStages[currentIndex].mInput.mInputs)
    {
        if (input == -1)
        {
            continue;
        }

        RecurseSetPriority(orders, input, currentPriority + 1, undeterminedNodeCount);
    }
}

std::vector<EvaluationStages::NodeOrder> EvaluationStages::ComputeEvaluationOrders()
{
    size_t nodeCount = mStages.size();

    std::vector<NodeOrder> orders(nodeCount);
    for (size_t i = 0; i < nodeCount; i++)
    {
        orders[i].mNodeIndex = i;
        orders[i].mNodePriority = 0;
    }
    size_t undeterminedNodeCount = nodeCount;
    while (undeterminedNodeCount)
    {
        size_t currentIndex = PickBestNode(orders);
        RecurseSetPriority(orders, currentIndex, orders[currentIndex].mNodePriority, undeterminedNodeCount);
    };
    //
    return orders;
}

void EvaluationStages::ComputeEvaluationOrder()
{
    mEvaluationOrderList.clear();

    auto orders = ComputeEvaluationOrders();
    std::sort(orders.begin(), orders.end());
    mEvaluationOrderList.resize(orders.size());
    for (size_t i = 0; i < orders.size(); i++)
    {
        mEvaluationOrderList[i] = orders[i].mNodeIndex;
    }
}

bool NodeTypeHasMultiplexer(size_t nodeType)
{
    for(const auto& parameter : gMetaNodes[nodeType].mParams)
    {
        if (parameter.mType == Con_Multiplexer)
        {
            return true;
        }
    }
    return false;
}

void EvaluationStages::GetMultiplexedInputs(size_t nodeIndex, std::vector<size_t>& list) const
{
    for (auto& input: mStages[nodeIndex].mInput.mInputs)
    {
        if (input == -1)
        {
            continue;
        }
        if (!NodeTypeHasMultiplexer(mStages[input].mType))
        {
            list.push_back(input);
        }
        else
        {
            GetMultiplexedInputs(input, list);
        }
    }
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
                glVertexAttribPointer(SemUV0 + 1, 4, GL_FLOAT, GL_FALSE, 16, 0);
                glEnableVertexAttribArray(SemUV0 + 1);
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
        glDrawElements(GL_TRIANGLES,
                       mIndexBuffer.count,
                       (mIndexBuffer.stride == 4) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
                       (void*)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDeleteVertexArrays(1, &vao);
}
void Scene::Mesh::Primitive::AddBuffer(const void* data, unsigned int format, unsigned int stride, unsigned int count)
{
    unsigned int va;
    glGenBuffers(1, &va);
    glBindBuffer(GL_ARRAY_BUFFER, va);
    glBufferData(GL_ARRAY_BUFFER, stride * count, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    mBuffers.push_back({va, format, stride, count});
}

void Scene::Mesh::Primitive::AddIndexBuffer(const void* data, unsigned int stride, unsigned int count)
{
    unsigned int ia;
    glGenBuffers(1, &ia);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ia);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, stride * count, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    mIndexBuffer = {ia, stride, count};
}
void Scene::Mesh::Draw() const
{
    for (auto& prim : mPrimitives)
    {
        prim.Draw();
    }
}
void Scene::Draw(EvaluationContext* context, EvaluationInfo& evaluationInfo) const
{
    for (unsigned int i = 0; i < mMeshIndex.size(); i++)
    {
        int index = mMeshIndex[i];
        if (index == -1)
            continue;

        glBindBuffer(GL_UNIFORM_BUFFER, context->mEvaluationStateGLSLBuffer);
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

