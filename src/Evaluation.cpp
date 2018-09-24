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
/*
void Evaluation::Bake(const char *szFilename, size_t target, int width, int height)
{
	if (mEvaluationOrderList.empty())
		return;

	if (target < 0 || target >= (int)mEvaluations.size())
		return;

	mTransientTextureMaxCount = 0;
	std::vector<int> evaluationUseCount(mEvaluationOrderList.size(), 0); // use count of texture by others
	std::vector<TransientTarget*> evaluationTransientTargets(mEvaluationOrderList.size(), NULL);
	for (auto& evaluation : mEvaluations)
	{
		for (int targetIndex : evaluation.mInput.mInputs)
			if (targetIndex != -1)
				evaluationUseCount[targetIndex]++;
	}
	if (evaluationUseCount[target] < 1)
		evaluationUseCount[target] = 1;

	// todo : revert pass. dec use count for parent nodes whose children have 0 use count

	for (size_t i = 0; i < mEvaluationOrderList.size(); i++)
	{
		size_t nodeIndex = mEvaluationOrderList[i];
		if (!evaluationUseCount[nodeIndex])
			continue;
		const EvaluationStage& evaluation = mEvaluations[nodeIndex];

		const Input& input = evaluation.mInput;

		TransientTarget* transientTarget = GetTransientTarget(width, height, evaluationUseCount[nodeIndex]);
		evaluationTransientTargets[nodeIndex] = transientTarget;
		transientTarget->mTarget.bindAsTarget();
		unsigned int program = mEvaluatorPerNodeType[evaluation.mNodeType].mGLSLProgram;
		glUseProgram(program);
		int samplerIndex = 0;
		for (size_t inputIndex = 0; inputIndex < 8; inputIndex++)
		{
			unsigned int parameter = glGetUniformLocation(program, samplerName[inputIndex]);
			if (parameter == 0xFFFFFFFF)
				continue;
			glUniform1i(parameter, samplerIndex);
			glActiveTexture(GL_TEXTURE0 + samplerIndex);
			samplerIndex++;
			int targetIndex = input.mInputs[inputIndex];
			if (targetIndex < 0)
			{
				glBindTexture(GL_TEXTURE_2D, 0);
				continue;
			}

			glBindTexture(GL_TEXTURE_2D, evaluationTransientTargets[targetIndex]->mTarget.mGLTexID);
			LoseTransientTarget(evaluationTransientTargets[targetIndex]);
			const InputSampler& inputSampler = evaluation.mInputSamplers[inputIndex];
			TexParam(filter[inputSampler.mFilterMin], filter[inputSampler.mFilterMag], wrap[inputSampler.mWrapU], wrap[inputSampler.mWrapV], GL_TEXTURE_2D);
		}
		mFSQuad.Render();
		if (nodeIndex == target)
			break;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);

	// save
	unsigned char *imgBits = new unsigned char[width * height * 4];
	glBindTexture(GL_TEXTURE_2D, evaluationTransientTargets[target]->mTarget.mGLTexID);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgBits);
	stbi_write_png(szFilename, width, height, 4, imgBits, width * 4);
	delete[] imgBits;
	
	// free transient textures
	for (auto transientTarget : mFreeTargets)
	{
		assert(transientTarget->mUseCount <= 0);
		transientTarget->mTarget.destroy();
	}

	Log("Texture %s saved. Using %d textures.\n", szFilename, mTransientTextureMaxCount);
}
*/
void Evaluation::Clear()
{
	for (auto& ev : mEvaluations)
		ev.Clear();

	mEvaluations.clear();
	mEvaluationOrderList.clear();
}