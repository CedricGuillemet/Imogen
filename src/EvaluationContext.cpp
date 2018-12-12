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

#include <GL/gl3w.h>    // Initialize with gl3wInit()
#include <memory>
#include "EvaluationContext.h"
#include "Evaluators.h"
#include "NodesDelegate.h"

EvaluationContext *gCurrentContext = NULL;

static const unsigned int wrap[] = { GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, GL_MIRRORED_REPEAT };
static const unsigned int filter[] = { GL_LINEAR, GL_NEAREST };
static const char* sampler2DName[] = { "Sampler0", "Sampler1", "Sampler2", "Sampler3", "Sampler4", "Sampler5", "Sampler6", "Sampler7" };
static const char* samplerCubeName[] = { "CubeSampler0", "CubeSampler1", "CubeSampler2", "CubeSampler3", "CubeSampler4", "CubeSampler5", "CubeSampler6", "CubeSampler7" };

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


EvaluationContext::EvaluationContext(Evaluation& evaluation, bool synchronousEvaluation, int defaultWidth, int defaultHeight) 
	: gEvaluation(evaluation)
	, mbSynchronousEvaluation(synchronousEvaluation)
	, mDefaultWidth(defaultWidth)
	, mDefaultHeight(defaultHeight)
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
	if (target >= mStageTarget.size())
		return 0;
	if (!mStageTarget[target])
		return 0;
	return mStageTarget[target]->mGLTexID;
}

#define SemUV 0
#define SemInstanceP0 1
#define SemInstanceN0 2
#define SemInstanceP1 3
#define SemInstanceP2 4

unsigned int mInstancesParametersBufferName[2];
unsigned int cubesVertexArrayName[2];
int instancesBufferReadIndex = 0;
unsigned int feedbackVertexArray[2];

unsigned int UploadIndices(const unsigned short *indices, unsigned int indexCount)
{
	unsigned int indexArray;
	glGenBuffers(1, &indexArray);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexArray);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned short), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	return indexArray;
}


void UploadVertices(const void *vertices, unsigned int vertexArraySize)
{
	unsigned int vertexSize = 2 * sizeof(float);// Mesh::GetVertexSizeFromFormat(vertexFormat);

	glGenVertexArrays(2, cubesVertexArrayName);
	glGenVertexArrays(2, feedbackVertexArray);

	// cube vertex array
	unsigned int vertexArray;
	glGenBuffers(1, &vertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, vertexArray);
	glBufferData(GL_ARRAY_BUFFER, vertexArraySize, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	glGenBuffers(2, mInstancesParametersBufferName);
		struct Instance
		{
			float p0x, p0y, p0z, pad0;
			float n0x, n0y, n0z, pad1;
			float p1x, p1y, p1z, pad2;
			float p2x, p2y, p2z, pad3;
		};
	
		Instance instancesPos[32*32];
		memset(instancesPos, 0.f, sizeof(instancesPos));
		for (int y = 0; y < 32; y++)
		{
			for (int x = 0; x < 32; x++)
			{
				instancesPos[y * 32 + x].p0x = x - 15;
				instancesPos[y * 32 + x].p0z = y - 15;
			}
		}
		
		// 2 buffers for transform feedback
		for (unsigned int i = 0; i < 2; i++)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mInstancesParametersBufferName[i]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(instancesPos), instancesPos, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

	// 2 vertex arrays
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindVertexArray(cubesVertexArrayName[i]);

		// blade vertices
		glBindBuffer(GL_ARRAY_BUFFER, vertexArray);
		glVertexAttribPointer(SemUV, 2, GL_FLOAT, GL_FALSE, vertexSize, 0);
		glEnableVertexAttribArray(SemUV);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// blade instances
		// root pos0, normal, pos1, pos2
		glBindBuffer(GL_ARRAY_BUFFER, mInstancesParametersBufferName[i]);
		glVertexAttribPointer(SemInstanceP0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 16, (void*)(16 * 0));
		glVertexAttribPointer(SemInstanceN0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 16, (void*)(16 * 1));
		glVertexAttribPointer(SemInstanceP1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 16, (void*)(16 * 2));
		glVertexAttribPointer(SemInstanceP2, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 16, (void*)(16 * 3));

		glEnableVertexAttribArray(SemInstanceP0);
		glEnableVertexAttribArray(SemInstanceN0);
		glEnableVertexAttribArray(SemInstanceP1);
		glEnableVertexAttribArray(SemInstanceP2);

		glBindBuffer(GL_ARRAY_BUFFER, 0);


		// back to normal
		glBindVertexArray(0);

		// vertex array for transform feedback
		glBindVertexArray(feedbackVertexArray[i]);
		glBindBuffer(GL_ARRAY_BUFFER, mInstancesParametersBufferName[i]);
		glVertexAttribPointer(0/*SemInstanceP0*/, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 16, (void*)(16 * 0));
		glVertexAttribPointer(1/*SemInstanceN0*/, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 16, (void*)(16 * 1));
		glVertexAttribPointer(2/*SemInstanceP1*/, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 16, (void*)(16 * 2));
		glVertexAttribPointer(3/*SemInstanceP2*/, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 16, (void*)(16 * 3));

		glEnableVertexAttribArray(0/*SemInstanceP0*/);
		glEnableVertexAttribArray(1/*SemInstanceN0*/);
		glEnableVertexAttribArray(2/*SemInstanceP1*/);
		glEnableVertexAttribArray(3/*SemInstanceP2*/);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);
	}

}
static const int tess = 10;
static unsigned int bladeIA = -1;
void drawBlades()
{
	//glUseProgram(bladeProgram);
	glBindVertexArray(cubesVertexArrayName[instancesBufferReadIndex]);

	// instances
	glVertexAttribDivisor(SemInstanceP0, 1);
	glVertexAttribDivisor(SemInstanceN0, 1);
	glVertexAttribDivisor(SemInstanceP1, 1);
	glVertexAttribDivisor(SemInstanceP2, 1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bladeIA);

	glDrawElementsInstanced(GL_TRIANGLE_STRIP, tess*2, GL_UNSIGNED_SHORT, (void*)0, 1/*32 * 32*/);

	glVertexAttribDivisor(SemInstanceP0, 0);
	glVertexAttribDivisor(SemInstanceN0, 0);
	glVertexAttribDivisor(SemInstanceP1, 0);
	glVertexAttribDivisor(SemInstanceP2, 0);


	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


void EvaluationContext::EvaluateGLSLCompute(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo)
{
	if (bladeIA == -1)
	{
		float bladeVertices[4 * tess];
		unsigned short bladeIndices[2 * tess];

		for (int i = 0; i < tess; i++)
		{
			bladeVertices[i * 4] = -1.f;
			bladeVertices[i * 4 + 1] = bladeVertices[i * 4 + 3] = float(i) / float(tess - 1);
			bladeVertices[i * 4 + 2] = 1.f;
		}
		for (int i = 0; i < tess * 2; i++)
		{
			bladeIndices[i] = i;
		}

		bladeIA = UploadIndices(bladeIndices, sizeof(bladeIndices) / sizeof(unsigned short));
		UploadVertices(bladeVertices, sizeof(bladeVertices));
	}

	const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mNodeType);
	const unsigned int program = evaluator.mGLSLProgram;

	size_t firstParticle = 0;
	size_t particleCount = 32 * 32;
	glUseProgram(program);
	glEnable(GL_RASTERIZER_DISCARD);
	glBindVertexArray(feedbackVertexArray[instancesBufferReadIndex]);
	glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, mInstancesParametersBufferName[(instancesBufferReadIndex + 1) & 1], firstParticle * 16 * sizeof(float), particleCount * 16 * sizeof(float));

	glBeginTransformFeedback(GL_POINTS);
	glDrawArrays(GL_POINTS, firstParticle, particleCount);
	glEndTransformFeedback();

	glDisable(GL_RASTERIZER_DISCARD);
	glBindVertexArray(0);
	glUseProgram(0);
	++instancesBufferReadIndex &= 1;
}

void EvaluationContext::EvaluateGLSL(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo)
{
	const Input& input = evaluationStage.mInput;

	auto tgt = mStageTarget[index];
	if (!evaluationInfo.uiPass)
	{
		if (tgt->mImage.mNumFaces == 6)
			tgt->BindAsCubeTarget();
		else
			tgt->BindAsTarget();
	}
	/*if (evaluationStage.mNodeTypename == "TerrainPreview")
	{
		DebugBreak();
	}
	*/
	const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mNodeType);
	const unsigned int program = evaluator.mGLSLProgram;
	const int blendOps[] = { evaluationStage.mBlendingSrc, evaluationStage.mBlendingDst };
	unsigned int blend[] = { GL_ONE, GL_ZERO };

	if (!program)
	{
		glUseProgram(gEvaluation.mNodeErrorShader);
		gFSQuad.Render();
		return;
	}
	for (int i = 0; i < 2; i++)
	{
		if (blendOps[i] < BLEND_LAST)
			blend[i] = GLBlends[blendOps[i]];
	}

	glEnable(GL_BLEND);
	glBlendFunc(blend[0], blend[1]);

	glUseProgram(program);

	Camera *camera = gNodeDelegate.GetCameraParameter(index);
	if (camera)
	{
		camera->ComputeViewProjectionMatrix(evaluationInfo.viewProjection, evaluationInfo.viewInverse);
	}

	size_t faceCount = evaluationInfo.uiPass ? 1 : tgt->mImage.mNumFaces;
	for (size_t face = 0; face < faceCount; face++)
	{
		if (tgt->mImage.mNumFaces == 6)
			tgt->BindCubeFace(face);

		memcpy(evaluationInfo.viewRot, rotMatrices[face], sizeof(float) * 16);
		memcpy(evaluationInfo.inputIndices, input.mInputs, sizeof(input.mInputs));
			
		glBindBuffer(GL_UNIFORM_BUFFER, gEvaluators.gEvaluationStateGLSLBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(EvaluationInfo), &evaluationInfo, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);


		glBindBufferBase(GL_UNIFORM_BUFFER, 1, evaluationStage.mParametersBuffer);
		glBindBufferBase(GL_UNIFORM_BUFFER, 2, gEvaluators.gEvaluationStateGLSLBuffer);

		for (int inputIndex = 0; inputIndex < 8; inputIndex++)
		{
			glActiveTexture(GL_TEXTURE0 + inputIndex);
			int targetIndex = input.mInputs[inputIndex];
			if (targetIndex < 0)
			{
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			else
			{
				unsigned int parameter = glGetUniformLocation(program, sampler2DName[inputIndex]);
				if (parameter == 0xFFFFFFFF)
					parameter = glGetUniformLocation(program, samplerCubeName[inputIndex]);
				if (parameter == 0xFFFFFFFF)
				{
					glBindTexture(GL_TEXTURE_2D, 0);
					continue;
				}
				glUniform1i(parameter, inputIndex);

				auto tgt = mStageTarget[targetIndex];
				if (tgt)
				{
					const InputSampler& inputSampler = evaluationStage.mInputSamplers[inputIndex];
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
		}
		//
		if (evaluationStage.mNodeTypename == "FurDisplay")
		{
			glClear(GL_COLOR_BUFFER_BIT);
			drawBlades();
		}
		else
		{
			gFSQuad.Render();
		}
	}
	glDisable(GL_BLEND);
}

void EvaluationContext::EvaluateC(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo)
{
	try // todo: find a better solution than a try catch
	{
		const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mNodeType);
		if (evaluator.mCFunction)
			evaluator.mCFunction((unsigned char*)evaluationStage.mParameters.data(), &evaluationInfo);
	}
	catch (...)
	{

	}
}

void EvaluationContext::EvaluatePython(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo)
{
	try // todo: find a better solution than a try catch
	{
		const Evaluator& evaluator = gEvaluators.GetEvaluator(evaluationStage.mNodeType);
		evaluator.RunPython();
	}
	catch (...)
	{

	}
}


void EvaluationContext::AllocRenderTargetsForEditingPreview()
{
	// alloc targets
	mStageTarget.resize(gEvaluation.GetStagesCount(), NULL);
	for (size_t i = 0; i < gEvaluation.GetStagesCount(); i++)
	{
		if (!mStageTarget[i])
		{
			mStageTarget[i] = std::make_shared<RenderTarget>();
		}
	}
}

void EvaluationContext::AllocRenderTargetsForBaking(const std::vector<size_t>& nodesToEvaluate)
{
	if (!mStageTarget.empty())
		return;

	//auto evaluationOrderList = gEvaluation.GetForwardEvaluationOrder();
	size_t stageCount = gEvaluation.GetStagesCount();
	mStageTarget.resize(stageCount, NULL);
	std::vector<std::shared_ptr<RenderTarget> > freeRenderTargets;
	std::vector<int> useCount(stageCount, 0);
	for (size_t i = 0; i < stageCount; i++)
	{
		useCount[i] = gEvaluation.GetEvaluationStage(i).mUseCountByOthers;
	}

	for (auto index : nodesToEvaluate)
	{
		const EvaluationStage& evaluation = gEvaluation.GetEvaluationStage(index);
		if (!evaluation.mUseCountByOthers)
			continue;

		if (freeRenderTargets.empty())
		{
			mStageTarget[index] = std::make_shared<RenderTarget>();
		}
		else
		{
			mStageTarget[index] = freeRenderTargets.back();
			freeRenderTargets.pop_back();
		}

		const Input& input = evaluation.mInput;
		for (auto targetIndex : input.mInputs)
		{
			if (targetIndex == -1)
				continue;

			useCount[targetIndex]--;
			if (!useCount[targetIndex])
			{
				freeRenderTargets.push_back(mStageTarget[targetIndex]);
			}
		}
	}
}
void EvaluationContext::PreRun()
{
	mbDirty.resize(gEvaluation.GetStagesCount(), false);
	mbProcessing.resize(gEvaluation.GetStagesCount(), false);
}

void EvaluationContext::RunNode(size_t nodeIndex)
{
	auto& currentStage = gEvaluation.GetEvaluationStage(nodeIndex);
	const Input& input = currentStage.mInput;

	// check processing 
	for (auto& inp : input.mInputs)
	{
		if (inp < 0)
			continue;
		if (mbProcessing[inp])
		{
			mbProcessing[nodeIndex] = true;
			return;
		}
	}

	mbProcessing[nodeIndex] = false;

	mEvaluationInfo.targetIndex = int(nodeIndex);
	memcpy(mEvaluationInfo.inputIndices, input.mInputs, sizeof(mEvaluationInfo.inputIndices));
	SetMouseInfos(mEvaluationInfo, currentStage);

	if (currentStage.gEvaluationMask&EvaluationC)
		EvaluateC(currentStage, nodeIndex, mEvaluationInfo);

	if (currentStage.gEvaluationMask&EvaluationPython)
		EvaluatePython(currentStage, nodeIndex, mEvaluationInfo);

	if (currentStage.gEvaluationMask&EvaluationGLSLCompute)
	{
		EvaluateGLSLCompute(currentStage, nodeIndex, mEvaluationInfo);
	}

	if (currentStage.gEvaluationMask&EvaluationGLSL)
	{
		if (!mStageTarget[nodeIndex]->mGLTexID)
			mStageTarget[nodeIndex]->InitBuffer(mDefaultWidth, mDefaultHeight);

		EvaluateGLSL(currentStage, nodeIndex, mEvaluationInfo);
	}
	mbDirty[nodeIndex] = false;
}

void EvaluationContext::RunNodeList(const std::vector<size_t>& nodesToEvaluate)
{
	// run C nodes
	for (size_t nodeIndex : nodesToEvaluate)
	{
		RunNode(nodeIndex);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
}

void EvaluationContext::RunSingle(size_t nodeIndex, EvaluationInfo& evaluationInfo)
{
	PreRun();

	mEvaluationInfo = evaluationInfo;

	RunNode(nodeIndex);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
}

void EvaluationContext::RecurseBackward(size_t target, std::vector<size_t>& usedNodes)
{
	const EvaluationStage& evaluation = gEvaluation.GetEvaluationStage(target);
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

void EvaluationContext::RunDirty()
{
	PreRun();
	memset(&mEvaluationInfo, 0, sizeof(EvaluationInfo));
	auto evaluationOrderList = gEvaluation.GetForwardEvaluationOrder();
	std::vector<size_t> nodesToEvaluate;
	for (size_t index = 0; index < evaluationOrderList.size(); index++)
	{
		size_t currentNodeIndex = evaluationOrderList[index];
		if (currentNodeIndex < mbDirty.size() && mbDirty[currentNodeIndex]) // TODOUNDO
			nodesToEvaluate.push_back(currentNodeIndex);
	}
	AllocRenderTargetsForEditingPreview();
	RunNodeList(nodesToEvaluate);
}

void EvaluationContext::RunAll()
{
	PreRun();
	// get list of nodes to run
	memset(&mEvaluationInfo, 0, sizeof(EvaluationInfo));
	auto evaluationOrderList = gEvaluation.GetForwardEvaluationOrder();
	AllocRenderTargetsForEditingPreview();
	RunNodeList(evaluationOrderList);
}

void EvaluationContext::RunBackward(size_t nodeIndex)
{
	PreRun();
	memset(&mEvaluationInfo, 0, sizeof(EvaluationInfo));
	mEvaluationInfo.forcedDirty = true;
	std::vector<size_t> nodesToEvaluate;
	RecurseBackward(nodeIndex, nodesToEvaluate);
	AllocRenderTargetsForBaking(nodesToEvaluate);
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
		encoder->Init(filename, align(width, 4), align(height, 4), 25, 400000);
	}
	return encoder;
}

void EvaluationContext::SetTargetDirty(size_t target, bool onlyChild)
{
	mbDirty.resize(gEvaluation.GetStagesCount(), false);
	auto evaluationOrderList = gEvaluation.GetForwardEvaluationOrder();
	mbDirty[target] = true;
	for (size_t i = 0; i < evaluationOrderList.size(); i++)
	{
		size_t currentNodeIndex = evaluationOrderList[i];
		if (currentNodeIndex != target)
			continue;

		for (i++; i < evaluationOrderList.size(); i++)
		{
			currentNodeIndex = evaluationOrderList[i];
			if (currentNodeIndex >= mbDirty.size() || mbDirty[currentNodeIndex]) // TODOUNDO
				continue;

			auto& currentEvaluation = gEvaluation.GetEvaluationStage(currentNodeIndex);
			for (auto inp : currentEvaluation.mInput.mInputs)
			{
				if (inp >= 0 && mbDirty[inp])
				{
					mbDirty[currentNodeIndex] = true;
					break;
				}
			}
		}
	}
	if (onlyChild)
		mbDirty[target] = false;
}

void EvaluationContext::UserAddStage()
{
	URAdd<std::shared_ptr<RenderTarget>> undoRedoAddRenderTarget(int(mStageTarget.size()), []() {return &gCurrentContext->mStageTarget; });
	URAdd<bool> undoRedoAddDirty(int(mbDirty.size()), []() {return &gCurrentContext->mbDirty; });
	URAdd<bool> undoRedoAddProcessing(int(mbProcessing.size()), []() {return &gCurrentContext->mbProcessing; });

	mStageTarget.push_back(std::make_shared<RenderTarget>());
	mbDirty.push_back(true);
	mbProcessing.push_back(false);
}

void EvaluationContext::UserDeleteStage(size_t index)
{
	URDel<std::shared_ptr<RenderTarget>> undoRedoDelRenderTarget(int(index), []() {return &gCurrentContext->mStageTarget; });
	URDel<bool> undoRedoDelDirty(int(index), []() {return &gCurrentContext->mbDirty; });
	URDel<bool> undoRedoDelProcessing(int(index), []() {return &gCurrentContext->mbProcessing; });

	mStageTarget.erase(mStageTarget.begin() + index);
	mbDirty.erase(mbDirty.begin() + index);
	mbProcessing.erase(mbProcessing.begin() + index);
}