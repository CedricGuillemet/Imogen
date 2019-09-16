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

#include <string>
#include <vector>
#include <map>
#include "Library.h"
#include "Imogen.h"
#include <string.h>
#include <stdio.h>
#include <memory>
#include "Utils.h"
#include "Bitmap.h"
#include "Scene.h"
#include "ImogenConfig.h"


struct ImDrawList;
struct ImDrawCmd;
struct EvaluationContext;
struct EvaluationInfo;

struct Dirty
{
    enum Type
    {
        Input = 1 << 0,
        Parameter = 1 << 1,
        Mouse = 1 << 2,
        Camera = 1 << 3,
        Time = 1 << 4,
        Sampler = 1 << 5,
        AddedNode = 1 << 6,
        DeletedNode = 1 << 7,
        StartEndTime = 1 << 8,
        RugChanged = 1 << 9,
        VisualGraph = 1 << 10, // node selection, node position
    };
};

struct DirtyList
{
    NodeIndex mNodeIndex;
	SlotIndex mSlotIndex;
    Dirty::Type mFlags;
};

struct Input
{
    Input()
    {
		for (auto i = 0; i < 8 ; i++)
		{
			mInputs[i] = { InvalidNodeIndex };
			mOverrideInputs[i] = { InvalidNodeIndex };
		}
    }
    NodeIndex mInputs[8];
	NodeIndex mOverrideInputs[8];
};

struct EvaluationStage
{
    //#ifdef _DEBUG needed for fur rendering
    std::string mTypename;
    //#endif
#if USE_FFMPEG    
    std::shared_ptr<FFMPEGCodec::Decoder> mDecoder;
#endif
    uint16_t mType;
	RuntimeId mRuntimeUniqueId;

    int mStartFrame, mEndFrame;
    
    // Camera
    Mat4x4 mParameterViewMatrix = Mat4x4::GetIdentity();

    // scene render
    void* mScene; // for path tracer
    std::shared_ptr<Scene> mGScene;
    void* renderer;
    Image DecodeImage();

    int mLocalTime;
};

struct MultiplexArray
{
	std::vector<NodeIndex> mMultiplexPerInputs[8];
};

// simple API
struct EvaluationStages
{
    EvaluationStages();
    void BuildEvaluationFromMaterial(Material& material);

    void AddEvaluation(NodeIndex nodeIndex, size_t nodeType);
    void DelEvaluation(NodeIndex nodeIndex);

    void Clear();
    size_t GetEvaluationImageDuration(NodeIndex target);

    void SetStageLocalTime(EvaluationContext* evaluationContext, NodeIndex nodeIndex, int localTime, bool updateDecoder);
    void SetSamplers(NodeIndex nodeIndex, InputSamplers samplers) { mInputSamplers[nodeIndex] = samplers; }

    Mat4x4* GetParameterViewMatrix(NodeIndex nodeIndex) { return &mStages[nodeIndex].mParameterViewMatrix; }
    const Parameters& GetParameters(NodeIndex nodeIndex) const { return mParameters[nodeIndex]; }
    void SetParameters(NodeIndex nodeIndex, const Parameters& parameters);
    uint16_t GetNodeType(NodeIndex nodeIndex) const { return mStages[nodeIndex].mType; }
    size_t GetStagesCount() const { return mStages.size(); }
	void SetMaterialUniqueId(RuntimeId runtimeId);

    // animation
    void ApplyAnimationForNode(EvaluationContext* context, NodeIndex nodeIndex, int frame);
    void ApplyAnimation(EvaluationContext* context, int frame);

    void SetTime(EvaluationContext* evaluationContext, int time, bool updateDecoder);

    // ffmpeg encoders
#if USE_FFMPEG
    FFMPEGCodec::Decoder* FindDecoder(const std::string& filename);
#endif
    // Data
    const std::vector<size_t>& GetForwardEvaluationOrder() const { return mOrderList; }
    std::vector<EvaluationStage> mStages;
    std::vector<Input> mInputs; // merged with multiplexed
    std::vector<Input> mDirectInputs; // without multiplexed
	std::vector<MultiplexArray> mMultiplex; // multiplex list per node

    std::vector<InputSamplers> mInputSamplers;
    std::vector<Parameters> mParameters;

    std::vector<AnimTrack> mAnimTrack;

	RuntimeId mMaterialUniqueId;

    void ComputeEvaluationOrder();
    void SetStartEndFrame(NodeIndex nodeIndex, int startFrame, int endFrame) { mStages[nodeIndex].mStartFrame = startFrame; mStages[nodeIndex].mEndFrame = endFrame; }
protected:

    std::vector<size_t> mOrderList;

    // evaluation order
    struct NodeOrder
    {
        size_t mNodeIndex;
        size_t mNodePriority;
        bool operator<(const NodeOrder& other) const
        {
            return other.mNodePriority < mNodePriority; // reverse order compared to priority value: lower last
        }
    };
    
    std::vector<NodeOrder> ComputeEvaluationOrders();
    void RecurseSetPriority(std::vector<NodeOrder>& orders, size_t currentIndex, size_t currentPriority, size_t& undeterminedNodeCount) const;
    size_t PickBestNode(const std::vector<NodeOrder>& orders) const;
};
