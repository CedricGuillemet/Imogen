#include <GL/gl3w.h>    // Initialize with gl3wInit()
#include "EvaluationContext.h"
#include "Evaluators.h"

EvaluationContext *gCurrentContext = NULL;

static const unsigned int wrap[] = { GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, GL_MIRRORED_REPEAT };
static const unsigned int filter[] = { GL_LINEAR, GL_NEAREST };
static const char* samplerName[] = { "Sampler0", "Sampler1", "Sampler2", "Sampler3", "Sampler4", "Sampler5", "Sampler6", "Sampler7", "CubeSampler0" };
static const unsigned int GLBlends[] = { GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR,GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA, GL_SRC_ALPHA_SATURATE };

static const float rotMatrices[6][16] = {
	// toward +x
	{ 0,0,-1,0,
	0,1,0,0,
	1,0,0,0,
	0,0,0,1
	},

	// -x
	{ 0,0,1,0,
	0,1,0,0,
	-1,0,0,0,
	0,0,0,1 },

	//+y
	{ 1,0,0,0,
	0,0,1,0,
	0,-1,0,0,
	0,0,0,1 },

	// -y
	{ 1,0,0,0,
	0,0,-1,0,
	0,1,0,0,
	0,0,0,1 },

	// +z
	{ 1,0,0,0,
	0,1,0,0,
	0,0,1,0,
	0,0,0,1 },

	//-z
	{ -1,0,0,0,
	0,1,0,0,
	0,0,-1,0,
	0,0,0,1 }
};


EvaluationContext::EvaluationContext(Evaluation& evaluation, bool synchronousEvaluation) : mEvaluation(evaluation), mbSynchronousEvaluation(synchronousEvaluation)
{

}

EvaluationContext::~EvaluationContext()
{
	for (auto& stream : mWriteStreams)
	{
		stream.second->Finish();
		delete stream.second;
	}
	mWriteStreams.clear();
}

static void SetMouseInfos(EvaluationInfo &evaluationInfo, const EvaluationStage &evaluationStage)
{
	evaluationInfo.mouse[0] = evaluationStage.mRx;
	evaluationInfo.mouse[1] = evaluationStage.mRy;
	evaluationInfo.mouse[2] = evaluationStage.mLButDown ? 1.f : 0.f;
	evaluationInfo.mouse[3] = evaluationStage.mRButDown ? 1.f : 0.f;
}

unsigned int EvaluationContext::GetEvaluationTexture(size_t target)
{
	if (!mStageTarget[target])
		return 0;
	return mStageTarget[target]->mGLTexID;
}

void EvaluationContext::EvaluateGLSL(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo)
{
	const Input& input = evaluationStage.mInput;

	RenderTarget* tgt = mStageTarget[index];
	if (!evaluationInfo.uiPass)
	{
		if (tgt->mImage.mNumFaces == 6)
			tgt->BindAsCubeTarget();
		else
			tgt->BindAsTarget();
	}
	const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mNodeType);
	unsigned int program = evaluator.mGLSLProgram;
	const int blendOps[] = { evaluationStage.mBlendingSrc, evaluationStage.mBlendingDst };
	unsigned int blend[] = { GL_ONE, GL_ZERO };


	for (int i = 0; i < 2; i++)
	{
		if (blendOps[i] < BLEND_LAST)
			blend[i] = GLBlends[blendOps[i]];
	}

	evaluationInfo.targetIndex = 0;
	memcpy(evaluationInfo.inputIndices, input.mInputs, sizeof(evaluationInfo.inputIndices));
	evaluationInfo.forcedDirty = evaluationStage.mbForceEval ? 1 : 0;
	SetMouseInfos(evaluationInfo, evaluationStage);
	//evaluationInfo.uiPass = 1;

	glEnable(GL_BLEND);
	glBlendFunc(blend[0], blend[1]);

	glUseProgram(program);

	size_t faceCount = evaluationInfo.uiPass ? 1 : tgt->mImage.mNumFaces;
	for (size_t face = 0; face < faceCount; face++)
	{
		if (tgt->mImage.mNumFaces == 6)
			tgt->BindCubeFace(face);

		memcpy(evaluationInfo.viewRot, rotMatrices[face], sizeof(float) * 16);
		glBindBuffer(GL_UNIFORM_BUFFER, gEvaluators.mEvaluationStateGLSLBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(EvaluationInfo), &evaluationInfo, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);


		glBindBufferBase(GL_UNIFORM_BUFFER, 1, evaluationStage.mParametersBuffer);
		glBindBufferBase(GL_UNIFORM_BUFFER, 2, gEvaluators.mEvaluationStateGLSLBuffer);

		int samplerIndex = 0;
		for (size_t inputIndex = 0; inputIndex < sizeof(samplerName) / sizeof(const char*); inputIndex++)
		{
			unsigned int parameter = glGetUniformLocation(program, samplerName[inputIndex]);
			if (parameter == 0xFFFFFFFF)
				continue;
			glUniform1i(parameter, samplerIndex);
			glActiveTexture(GL_TEXTURE0 + samplerIndex);

			int targetIndex = input.mInputs[samplerIndex];
			if (targetIndex < 0)
			{
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			else
			{
				auto* tgt = mStageTarget[targetIndex];
				if (tgt)
				{
					const InputSampler& inputSampler = evaluationStage.mInputSamplers[samplerIndex];
					if (tgt->mImage.mNumFaces == 1)
					{
						glBindTexture(GL_TEXTURE_2D, tgt->mGLTexID);
						TexParam(filter[inputSampler.mFilterMin], filter[inputSampler.mFilterMag], wrap[inputSampler.mWrapU], wrap[inputSampler.mWrapV], GL_TEXTURE_2D);
					}
					else
					{
						glBindTexture(GL_TEXTURE_CUBE_MAP, tgt->mGLTexID);
						TexParam(filter[inputSampler.mFilterMin], filter[inputSampler.mFilterMag], wrap[inputSampler.mWrapU], wrap[inputSampler.mWrapV], GL_TEXTURE_CUBE_MAP);
					}
				}
			}
			samplerIndex++;
		}
		//
		gFSQuad.Render();
	}
	glDisable(GL_BLEND);
}

void EvaluationContext::EvaluateC(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo)
{
	const Input& input = evaluationStage.mInput;

	//EvaluationInfo evaluationInfo;
	evaluationInfo.targetIndex = int(index);
	memcpy(evaluationInfo.inputIndices, input.mInputs, sizeof(evaluationInfo.inputIndices));
	SetMouseInfos(evaluationInfo, evaluationStage);
	//evaluationInfo.forcedDirty = evaluation.mbForceEval ? 1 : 0;
	//evaluationInfo.uiPass = false;
	try // todo: find a better solution than a try catch
	{
		const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mNodeType);
		evaluator.mCFunction(evaluationStage.mParameters, &evaluationInfo);
	}
	catch (...)
	{

	}
}

void EvaluationContext::RunNodeList(const std::vector<size_t>& nodesToEvaluate)
{
	EvaluationInfo evaluationInfo;
	// run C nodes
	for (size_t index : nodesToEvaluate)
	{
		auto& currentEvaluation = mEvaluation.GetEvaluationStage(index);
		if (!currentEvaluation.mEvaluationMask&EvaluationC)
			continue;
		EvaluateC(currentEvaluation, index, evaluationInfo);
	}
	// run glsl
	for (size_t index : nodesToEvaluate)
	{
		auto& currentEvaluation = mEvaluation.GetEvaluationStage(index);
		if (!(currentEvaluation.mEvaluationMask&EvaluationGLSL))
			continue;
		EvaluateGLSL(currentEvaluation, index, evaluationInfo);
	}
}
void EvaluationContext::RecurseBackward(size_t target, std::vector<size_t>& usedNodes)
{
	const EvaluationStage& evaluation = mEvaluation.GetEvaluationStage(target);
	const Input& input = evaluation.mInput;

	for (size_t inputIndex = 0; inputIndex < 8; inputIndex++)
	{
		int targetIndex = input.mInputs[inputIndex];
		if (targetIndex == -1)
			continue;
		RecurseBackward(targetIndex, usedNodes);
	}

	if (std::find(usedNodes.begin(), usedNodes.end(), target) == usedNodes.end())
		usedNodes.push_back(target);
}

void EvaluationContext::RecurseForward(size_t base, size_t parent, std::vector<size_t>& usedNodes)
{
	auto evaluationOrderList = mEvaluation.GetForwardEvaluationOrder();
	for (; base < evaluationOrderList.size(); base++)
	{
		size_t currentNodeIndex = evaluationOrderList[base];
		const EvaluationStage& evaluation = mEvaluation.GetEvaluationStage(currentNodeIndex);
		const Input& input = evaluation.mInput;

		for (size_t inputIndex = 0; inputIndex < 8; inputIndex++)
		{
			int targetIndex = input.mInputs[inputIndex];
			if (targetIndex == parent)
				RecurseForward(base, currentNodeIndex, usedNodes);
		}
	}
	if (std::find(usedNodes.begin(), usedNodes.end(), parent) == usedNodes.end())
		usedNodes.push_back(parent);
}


void EvaluationContext::RunForward(size_t nodeIndex)
{
	std::vector<size_t> nodesToEvaluate;
	auto evaluationOrderList = mEvaluation.GetForwardEvaluationOrder();
	for (size_t index = 0;index< evaluationOrderList.size();index++)
	{
		if (evaluationOrderList[index] == nodeIndex)
		{
			RecurseForward(index, nodeIndex, nodesToEvaluate);
		}
	}
	RunNodeList(nodesToEvaluate);
}

void EvaluationContext::RunAll()
{
	// get list of nodes to run
	auto evaluationOrderList = mEvaluation.GetForwardEvaluationOrder();
	RunNodeList(evaluationOrderList);
}

void EvaluationContext::RunBackward(size_t nodeIndex)
{
	std::vector<size_t> nodesToEvaluate;
	RecurseBackward(nodeIndex, nodesToEvaluate);
	RunNodeList(nodesToEvaluate);
}

FFMPEGCodec::Encoder *EvaluationContext::GetEncoder(const std::string &filename, int width, int height)
{
	FFMPEGCodec::Encoder *encoder;
	auto iter = mWriteStreams.find(filename);
	if (iter != mWriteStreams.end())
	{
		encoder = iter->second;
	}
	else
	{
		encoder = new FFMPEGCodec::Encoder;
		mWriteStreams[filename] = encoder;
		encoder->Init(width, height, 25, 400000);
	}
	return encoder;
}