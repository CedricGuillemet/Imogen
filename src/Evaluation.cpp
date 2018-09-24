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

Evaluation::Evaluation() : mDirtyCount(0)
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
	auto iter = mEvaluatorScripts.find(nodeName+".glsl");
	if (iter != mEvaluatorScripts.end())
	{
		EvaluationStage evaluation;
		evaluation.mTarget.initBuffer(256, 256, false);
		evaluation.mbDirty = true;
		evaluation.mbForceEval = false;
		evaluation.mNodeType = nodeType;
		evaluation.mParametersBuffer = 0;
		evaluation.mEvaluationType = 0;
		iter->second.mNodeType = int(nodeType);
		mEvaluatorPerNodeType[nodeType].mGLSLProgram = iter->second.mProgram;
		mDirtyCount++;
		mEvaluations.push_back(evaluation);
		return mEvaluations.size() - 1;
	}
	iter = mEvaluatorScripts.find(nodeName + ".c");
	if (iter != mEvaluatorScripts.end())
	{
		EvaluationStage evaluation;
		//evaluation.mTarget.initBuffer(256, 256, false);
		evaluation.mbDirty = true;
		evaluation.mbForceEval = false;
		evaluation.mNodeType = nodeType;
		evaluation.mParametersBuffer = 0;
		evaluation.mEvaluationType = 1;
		iter->second.mNodeType = int(nodeType);
		mEvaluatorPerNodeType[nodeType].mCFunction = iter->second.mCFunction;
		mEvaluatorPerNodeType[nodeType].mMem = iter->second.mMem;
		mDirtyCount++;
		mEvaluations.push_back(evaluation);
		return mEvaluations.size() - 1;
	}
	Log("Could not find node name \"%s\" \n", nodeName.c_str());
	return -1;
}

void Evaluation::DelEvaluationTarget(size_t target)
{
	SetTargetDirty(target);
	EvaluationStage& ev = mEvaluations[target];
	ev.Clear();
	mEvaluations.erase(mEvaluations.begin() + target);

	// shift all connections
	for (auto& evaluation : mEvaluations)
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
	return mEvaluations[target].mTarget.mGLTexID;
}

void Evaluation::SetEvaluationParameters(size_t target, void *parameters, size_t parametersSize)
{
	EvaluationStage& stage = mEvaluations[target];
	stage.mParameters = parameters;
	stage.mParametersSize = parametersSize;

	if (stage.mEvaluationType == EVALUATOR_GLSL)
		BindGLSLParameters(stage);

	SetTargetDirty(target);
}

void Evaluation::ForceEvaluation(size_t target)
{
	EvaluationStage& stage = mEvaluations[target];
	stage.mbForceEval = true;
	SetTargetDirty(target);
}

void Evaluation::RunEvaluation()
{
	if (mEvaluationOrderList.empty())
		return;
	if (!mDirtyCount)
		return;

	
	for (size_t i = 0; i < mEvaluationOrderList.size(); i++)
	{
		size_t index = mEvaluationOrderList[i];
		const EvaluationStage& evaluation = mEvaluations[index];
		if (!evaluation.mbDirty)
			continue;

		switch (evaluation.mEvaluationType)
		{
		case 0: // GLSL
			EvaluateGLSL(evaluation);
			break;
		case 1: // C
			EvaluateC(evaluation, index);
			break;
		}
	}

	for (auto& evaluation : mEvaluations)
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
	mEvaluations[target].mInputSamplers = inputSamplers;
	SetTargetDirty(target);
}

void Evaluation::AddEvaluationInput(size_t target, int slot, int source)
{
	mEvaluations[target].mInput.mInputs[slot] = source;
	SetTargetDirty(target);
}

void Evaluation::DelEvaluationInput(size_t target, int slot)
{
	mEvaluations[target].mInput.mInputs[slot] = -1;
	SetTargetDirty(target);
}

void Evaluation::SetEvaluationOrder(const std::vector<size_t> nodeOrderList)
{
	mEvaluationOrderList = nodeOrderList;
}

void Evaluation::SetTargetDirty(size_t target)
{
	if (!mEvaluations[target].mbDirty)
	{
		mDirtyCount++;
		mEvaluations[target].mbDirty = true;
	}
	for (size_t i = 0; i < mEvaluationOrderList.size(); i++)
	{
		if (mEvaluationOrderList[i] != target)
			continue;
		
		for (i++; i < mEvaluationOrderList.size(); i++)
		{
			EvaluationStage& currentEvaluation = mEvaluations[mEvaluationOrderList[i]];
			if (currentEvaluation.mbDirty)
				continue;

			for (auto inp : currentEvaluation.mInput.mInputs)
			{
				if (inp >= 0 && mEvaluations[inp].mbDirty)
				{
					mDirtyCount++;
					currentEvaluation.mbDirty = true;
					break; // at least 1 dirty
				}
			}
		}
	}
}

struct TransientTarget
{
	RenderTarget mTarget;
	int mUseCount;
};
std::vector<TransientTarget*> mFreeTargets;
int mTransientTextureMaxCount;
static TransientTarget* GetTransientTarget(int width, int height, int useCount)
{
	if (mFreeTargets.empty())
	{
		TransientTarget *res = new TransientTarget;
		res->mTarget.initBuffer(width, height, false);
		res->mUseCount = useCount;
		mTransientTextureMaxCount++;
		return res;
	}
	TransientTarget *res = mFreeTargets.back();
	res->mUseCount = useCount;
	mFreeTargets.pop_back();
	return res;
}

static void LoseTransientTarget(TransientTarget *transientTarget)
{
	transientTarget->mUseCount--;
	if (transientTarget->mUseCount <= 0)
		mFreeTargets.push_back(transientTarget);
}

void Evaluation::Clear()
{
	for (auto& ev : mEvaluations)
		ev.Clear();

	mEvaluations.clear();
	mEvaluationOrderList.clear();
}