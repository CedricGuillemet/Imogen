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

#include "Evaluation.h"
#include "EvaluationContext.h"
#include "Evaluators.h"
#include <vector>
#include <algorithm>
#include <map>

Evaluation::Evaluation() : mProgressShader(0), mDisplayCubemapShader(0)
{
	
}

void Evaluation::Init()
{
	APIInit();
}

void Evaluation::Finish()
{

}

size_t Evaluation::AddEvaluation(size_t nodeType, const std::string& nodeName)
{
	EvaluationStage evaluation;
#ifdef _DEBUG
	evaluation.mNodeTypename = nodeName;
#endif
	evaluation.mDecoder				= NULL;
	evaluation.mUseCountByOthers	= 0;
	evaluation.mNodeType			= nodeType;
	evaluation.mParametersBuffer	= 0;
	evaluation.mBlendingSrc			= ONE;
	evaluation.mBlendingDst			= ZERO;
	evaluation.mLocalTime			= 0;
	evaluation.mEvaluationMask		= gEvaluators.GetMask(nodeType, nodeName);

	if (evaluation.mEvaluationMask)
	{
		//mDirtyCount++;
		mEvaluationStages.push_back(evaluation);
		return mEvaluationStages.size() - 1;
	}
	Log("Could not find node name \"%s\" \n", nodeName.c_str());
	return -1;
}

void Evaluation::DelEvaluationTarget(size_t target)
{
	EvaluationStage& ev = mEvaluationStages[target];
	ev.Clear();
	mEvaluationStages.erase(mEvaluationStages.begin() + target);

	// shift all connections
	for (auto& evaluation : mEvaluationStages)
	{
		for (auto& inp : evaluation.mInput.mInputs)
		{
			if (inp >= int(target))
				inp--;
		}
	}
	gCurrentContext->RunAll();
}

void Evaluation::SetEvaluationParameters(size_t target, void *parameters, size_t parametersSize)
{
	EvaluationStage& stage = mEvaluationStages[target];
	stage.mParameters = parameters;
	stage.mParametersSize = parametersSize;

	if (stage.mEvaluationMask&EvaluationGLSL)
		BindGLSLParameters(stage);
	if (stage.mDecoder)
		stage.mDecoder = NULL;
}

void Evaluation::SetEvaluationSampler(size_t target, const std::vector<InputSampler>& inputSamplers)
{
	mEvaluationStages[target].mInputSamplers = inputSamplers;
	gCurrentContext->SetTargetDirty(target);
}

void Evaluation::AddEvaluationInput(size_t target, int slot, int source)
{
	mEvaluationStages[target].mInput.mInputs[slot] = source;
	mEvaluationStages[source].mUseCountByOthers++;
	gCurrentContext->SetTargetDirty(target);
}

void Evaluation::DelEvaluationInput(size_t target, int slot)
{
	mEvaluationStages[mEvaluationStages[target].mInput.mInputs[slot]].mUseCountByOthers--;
	mEvaluationStages[target].mInput.mInputs[slot] = -1;
	gCurrentContext->SetTargetDirty(target);
}

void Evaluation::SetEvaluationOrder(const std::vector<size_t> nodeOrderList)
{
	mEvaluationOrderList = nodeOrderList;
}

void Evaluation::Clear()
{
	for (auto& ev : mEvaluationStages)
		ev.Clear();

	mEvaluationStages.clear();
	mEvaluationOrderList.clear();
}

void Evaluation::SetMouse(int target, float rx, float ry, bool lButDown, bool rButDown)
{
	for (auto& ev : mEvaluationStages)
	{
		ev.mRx = -9999.f;
		ev.mRy = -9999.f;
		ev.mLButDown = false;
		ev.mRButDown = false;
	}
	auto& ev = mEvaluationStages[target];
	ev.mRx = rx;
	ev.mRy = 1.f - ry; // inverted for UI
	ev.mLButDown = lButDown;
	ev.mRButDown = rButDown;
}

size_t Evaluation::GetEvaluationImageDuration(size_t target)
{
	auto& stage = mEvaluationStages[target];
	if (!stage.mDecoder)
		return 1;
	if (stage.mDecoder->mFrameCount > 2000)
	{
		int a = 1;
	}
	return stage.mDecoder->mFrameCount;
}

void Evaluation::SetStageLocalTime(size_t target, int localTime, bool updateDecoder)
{
	auto& stage = mEvaluationStages[target];
	int newLocalTime = ImMin(localTime, int(GetEvaluationImageDuration(target)));
	if (stage.mDecoder && updateDecoder && stage.mLocalTime != newLocalTime)
	{
		stage.mLocalTime = newLocalTime;
		Image_t image = stage.DecodeImage();
		SetEvaluationImage(int(target), &image);
		FreeImage(&image);
	}
	else
	{
		stage.mLocalTime = newLocalTime;
	}
}

int Evaluation::Evaluate(int target, int width, int height, Image *image)
{
	EvaluationContext *previousContext = gCurrentContext;
	EvaluationContext context(gEvaluation, true, width, height);
	gCurrentContext = &context;
	context.RunBackward(target);
	GetEvaluationImage(target, image);
	gCurrentContext = previousContext;
	return EVAL_OK;
}

FFMPEGCodec::Decoder* Evaluation::FindDecoder(const std::string& filename)
{
	for (auto& evaluation : mEvaluationStages)
	{
		if (evaluation.mDecoder && evaluation.mDecoder->GetFilename() == filename)
			return evaluation.mDecoder.get();
	}
	auto decoder = new FFMPEGCodec::Decoder;
	decoder->Open(filename);
	return decoder;
}
