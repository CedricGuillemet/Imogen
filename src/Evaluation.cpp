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

Evaluation::Evaluation() : mProgressShader(0), mDisplayCubemapShader(0), mNodeErrorShader(0)
{
	
}

void Evaluation::Init()
{
	APIInit();
}

void Evaluation::Finish()
{

}

void Evaluation::AddSingleEvaluation(size_t nodeType)
{
	EvaluationStage evaluation;
#ifdef _DEBUG
	evaluation.mNodeTypename		= gMetaNodes[nodeType].mName;;
#endif
	evaluation.mDecoder				= NULL;
	evaluation.mUseCountByOthers	= 0;
	evaluation.mNodeType			= nodeType;
	evaluation.mParametersBuffer	= 0;
	evaluation.mBlendingSrc			= ONE;
	evaluation.mBlendingDst			= ZERO;
	evaluation.mLocalTime			= 0;
	evaluation.gEvaluationMask		= gEvaluators.GetMask(nodeType);
	mStages.push_back(evaluation);
}

void Evaluation::StageIsAdded(int index)
{
	for (size_t i = 0;i< gEvaluation.mStages.size();i++)
	{
		if (i == index)
			continue;
		auto& evaluation = gEvaluation.mStages[i];
		for (auto& inp : evaluation.mInput.mInputs)
		{
			if (inp >= index)
				inp++;
		}
	}
}

void Evaluation::StageIsDeleted(int index)
{
	EvaluationStage& ev = gEvaluation.mStages[index];
	ev.Clear();

	// shift all connections
	for (auto& evaluation : gEvaluation.mStages)
	{
		for (auto& inp : evaluation.mInput.mInputs)
		{
			if (inp >= index)
				inp--;
		}
	}
}

void Evaluation::UserAddEvaluation(size_t nodeType)
{
	URAdd<EvaluationStage> undoRedoAddStage(int(mStages.size()), []() {return &gEvaluation.mStages; },
		StageIsDeleted, StageIsAdded);

	AddSingleEvaluation(nodeType);
}

void Evaluation::UserDeleteEvaluation(size_t target)
{
	URDel<EvaluationStage> undoRedoDelStage(int(target), []() {return &gEvaluation.mStages; },
		StageIsDeleted, StageIsAdded);

	mStages.erase(mStages.begin() + target);
}

void Evaluation::SetEvaluationParameters(size_t target, const std::vector<unsigned char> &parameters)
{
	EvaluationStage& stage = mStages[target];
	stage.mParameters = parameters;

	if (stage.gEvaluationMask&EvaluationGLSL)
		BindGLSLParameters(stage);
	if (stage.mDecoder)
		stage.mDecoder = NULL;
}

void Evaluation::SetEvaluationSampler(size_t target, const std::vector<InputSampler>& inputSamplers)
{
	mStages[target].mInputSamplers = inputSamplers;
	gCurrentContext->SetTargetDirty(target);
}

void Evaluation::AddEvaluationInput(size_t target, int slot, int source)
{
	if (mStages[target].mInput.mInputs[slot] == source)
		return;
	mStages[target].mInput.mInputs[slot] = source;
	mStages[source].mUseCountByOthers++;
	gCurrentContext->SetTargetDirty(target);
}

void Evaluation::DelEvaluationInput(size_t target, int slot)
{
	mStages[mStages[target].mInput.mInputs[slot]].mUseCountByOthers--;
	mStages[target].mInput.mInputs[slot] = -1;
	gCurrentContext->SetTargetDirty(target);
}

void Evaluation::SetEvaluationOrder(const std::vector<size_t> nodeOrderList)
{
	gEvaluationOrderList = nodeOrderList;
}

void Evaluation::Clear()
{
	for (auto& ev : mStages)
		ev.Clear();

	mStages.clear();
	gEvaluationOrderList.clear();
}

void Evaluation::SetMouse(int target, float rx, float ry, bool lButDown, bool rButDown)
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

size_t Evaluation::GetEvaluationImageDuration(size_t target)
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

void Evaluation::SetStageLocalTime(size_t target, int localTime, bool updateDecoder)
{
	auto& stage = mStages[target];
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
	for (auto& evaluation : mStages)
	{
		if (evaluation.mDecoder && evaluation.mDecoder->GetFilename() == filename)
			return evaluation.mDecoder.get();
	}
	auto decoder = new FFMPEGCodec::Decoder;
	decoder->Open(filename);
	return decoder;
}
