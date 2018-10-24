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
#include <vector>
#include <algorithm>
#include <map>


std::string Evaluation::GetEvaluator(const std::string& filename)
{
	return mEvaluatorScripts[filename].mText;
}

Evaluation::Evaluation() : mDirtyCount(0), mEvaluationMode(-1), mEvaluationStateGLSLBuffer(0), mProgressShader(0), mDisplayCubemapShader(0)
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
	evaluation.mTarget = NULL;
	evaluation.mUseCountByOthers = 0;
	evaluation.mbDirty = true;
	evaluation.mbForceEval = false;
	evaluation.mbProcessing = false;
	evaluation.mbFreeSizing = true;
	evaluation.mNodeType = nodeType;
	evaluation.mParametersBuffer = 0;
	evaluation.mEvaluationMask = 0;
	evaluation.mBlendingSrc = ONE;
	evaluation.mBlendingDst = ZERO;
#ifdef _DEBUG
	evaluation.mNodeTypename = nodeName;
#endif

	bool valid(false);
	auto iter = mEvaluatorScripts.find(nodeName+".glsl");
	if (iter != mEvaluatorScripts.end())
	{
		evaluation.mEvaluationMask |= EvaluationGLSL;
		iter->second.mNodeType = int(nodeType);
		evaluation.mTarget = new RenderTarget;
		mAllocatedRenderTargets.push_back(evaluation.mTarget);
		mEvaluatorPerNodeType[nodeType].mGLSLProgram = iter->second.mProgram;
		valid = true;
	}
	iter = mEvaluatorScripts.find(nodeName + ".c");
	if (iter != mEvaluatorScripts.end())
	{
		evaluation.mEvaluationMask |= EvaluationC;
		iter->second.mNodeType = int(nodeType);
		mEvaluatorPerNodeType[nodeType].mCFunction = iter->second.mCFunction;
		mEvaluatorPerNodeType[nodeType].mMem = iter->second.mMem;
		valid = true;
	}

	if (valid)
	{
		mDirtyCount++;
		mEvaluationStages.push_back(evaluation);
		return mEvaluationStages.size() - 1;
	}
	Log("Could not find node name \"%s\" \n", nodeName.c_str());
	return -1;
}

void Evaluation::DelEvaluationTarget(size_t target)
{
	SetTargetDirty(target);
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
}

unsigned int Evaluation::GetEvaluationTexture(size_t target)
{
	if (!mEvaluationStages[target].mTarget)
		return 0;
	return mEvaluationStages[target].mTarget->mGLTexID;
}

void Evaluation::SetEvaluationParameters(size_t target, void *parameters, size_t parametersSize)
{
	EvaluationStage& stage = mEvaluationStages[target];
	stage.mParameters = parameters;
	stage.mParametersSize = parametersSize;

	if (stage.mEvaluationMask&EvaluationGLSL)
		BindGLSLParameters(stage);

	SetTargetDirty(target);
}

void Evaluation::PerformEvaluationForNode(size_t index, int width, int height, bool force, EvaluationInfo& evaluationInfo)
{
	EvaluationStage& evaluation = mEvaluationStages[index];
	
	if (force)
	{
		evaluation.mbForceEval = true;
		SetTargetDirty(index);
	}
	// check processing 
	for (auto& inp : evaluation.mInput.mInputs)
	{
		if (inp >= 0)
		{
			if (mEvaluationStages[inp].mbProcessing)
			{
				evaluation.mbProcessing = true;
				return;
			}
		}
	}

	evaluation.mbProcessing = false;

	// good to go
	if (evaluation.mEvaluationMask&EvaluationC)
		EvaluateC(evaluation, index, evaluationInfo);
	if (evaluation.mEvaluationMask&EvaluationGLSL)
		EvaluateGLSL(evaluation, evaluationInfo);
}

void Evaluation::SetEvaluationMemoryMode(int evaluationMode)
{
	if (evaluationMode == mEvaluationMode)
		return;

	mEvaluationMode = evaluationMode;
	// free previously allocated RT

	for (auto* rt : mAllocatedRenderTargets)
	{
		rt->Destroy();
		delete rt;
	}
	mAllocatedRenderTargets.clear();

	if (evaluationMode == 0) // edit mode
	{
		for (size_t i = 0; i < mEvaluationStages.size(); i++)
		{
			EvaluationStage& evaluation = mEvaluationStages[i];
			if (evaluation.mEvaluationMask&EvaluationGLSL)
			{
				evaluation.mTarget = new RenderTarget;
				mAllocatedRenderTargets.push_back(evaluation.mTarget);
			}
			if (evaluation.mEvaluationMask&EvaluationC)
			{
				evaluation.mTarget = 0;
			}
		}
	}
	else // baking mode
	{
		std::vector<RenderTarget*> freeRenderTargets;
		std::vector<int> useCount(mEvaluationStages.size(), 0);
		for (size_t i = 0; i < mEvaluationStages.size(); i++)
		{
			useCount[i] = mEvaluationStages[i].mUseCountByOthers;
		}

		for (size_t i = 0; i < mEvaluationOrderList.size(); i++)
		{
			size_t index = mEvaluationOrderList[i];

			EvaluationStage& evaluation = mEvaluationStages[index];
			if (!evaluation.mUseCountByOthers)
				continue;

			if (freeRenderTargets.empty())
			{
				evaluation.mTarget = new RenderTarget();
				mAllocatedRenderTargets.push_back(evaluation.mTarget);
			}
			else
			{
				evaluation.mTarget = freeRenderTargets.back();
				freeRenderTargets.pop_back();
			}

			const Input& input = evaluation.mInput;
			for (size_t inputIndex = 0; inputIndex < 8; inputIndex++)
			{
				int targetIndex = input.mInputs[inputIndex];
				if (targetIndex == -1)
					continue;
				
				useCount[targetIndex]--;
				if (!useCount[targetIndex])
				{
					freeRenderTargets.push_back(mEvaluationStages[targetIndex].mTarget);
				}
			}
		}
		
	}
	//Log("Using %d allocated buffers.\n", mAllocatedRenderTargets.size());
}

void Evaluation::RunEvaluation(int width, int height, bool forceEvaluation)
{
	if (mEvaluationOrderList.empty())
		return;
	if (!mDirtyCount && !forceEvaluation)
		return;

	EvaluationInfo evaluationInfo;
	evaluationInfo.forcedDirty = forceEvaluation ? 1 : 0;
	evaluationInfo.uiPass = 0;
	for (size_t i = 0; i < mEvaluationOrderList.size(); i++)
	{
		size_t index = mEvaluationOrderList[i];

		EvaluationStage& evaluation = mEvaluationStages[index];
		if (!evaluation.mbDirty && !forceEvaluation)
			continue;

		if (evaluation.mTarget && !evaluation.mTarget->mGLTexID)
		{
			evaluation.mTarget->InitBuffer(width, height);
		}

		PerformEvaluationForNode(index, width, height, false, evaluationInfo);
	}

	for (auto& evaluation : mEvaluationStages)
	{
		if (evaluation.mbDirty)
		{
			evaluation.mbDirty = false;
			evaluation.mbForceEval = false;
			mDirtyCount--;
		}
	}

	FinishEvaluation();
}

void Evaluation::SetEvaluationSampler(size_t target, const std::vector<InputSampler>& inputSamplers)
{
	mEvaluationStages[target].mInputSamplers = inputSamplers;
	SetTargetDirty(target);
}

void Evaluation::AddEvaluationInput(size_t target, int slot, int source)
{
	mEvaluationStages[target].mInput.mInputs[slot] = source;
	mEvaluationStages[source].mUseCountByOthers++;
	SetTargetDirty(target);
}

void Evaluation::DelEvaluationInput(size_t target, int slot)
{
	mEvaluationStages[mEvaluationStages[target].mInput.mInputs[slot]].mUseCountByOthers--;
	mEvaluationStages[target].mInput.mInputs[slot] = -1;
	SetTargetDirty(target);
}

void Evaluation::SetEvaluationOrder(const std::vector<size_t> nodeOrderList)
{
	mEvaluationOrderList = nodeOrderList;
}

void Evaluation::SetTargetDirty(size_t target, bool onlyChild)
{
	if (!mEvaluationStages[target].mbDirty)
	{
		if (!onlyChild)
			mDirtyCount++;
		mEvaluationStages[target].mbDirty = true;
	}
	for (size_t i = 0; i < mEvaluationOrderList.size(); i++)
	{
		if (mEvaluationOrderList[i] != target)
			continue;
		
		for (i++; i < mEvaluationOrderList.size(); i++)
		{
			EvaluationStage& currentEvaluation = mEvaluationStages[mEvaluationOrderList[i]];
			if (currentEvaluation.mbDirty)
				continue;

			for (auto inp : currentEvaluation.mInput.mInputs)
			{
				if (inp >= 0 && mEvaluationStages[inp].mbDirty)
				{
					mDirtyCount++;
					currentEvaluation.mbDirty = true;
					break; // at least 1 dirty
				}
			}
		}
	}
	if (onlyChild)
		mEvaluationStages[target].mbDirty = false;
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