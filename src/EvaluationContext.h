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
#pragma once
#include "Evaluation.h"

struct EvaluationContext
{
	EvaluationContext(Evaluation& evaluation, bool synchronousEvaluation, int defaultWidth, int defaultHeight);
	~EvaluationContext();

	void RunAll();
	void RunBackward(size_t nodeIndex);
	void RunSingle(size_t nodeIndex, EvaluationInfo& evaluationInfo);
	void RunDirty();

	unsigned int GetEvaluationTexture(size_t target);
	RenderTarget *GetRenderTarget(size_t target)
	{ 
		if (target >= mStageTarget.size())
			return NULL;
		return mStageTarget[target]; 
	}

	FFMPEGCodec::Encoder *GetEncoder(const std::string &filename, int width, int height);
	bool IsSynchronous() const { return mbSynchronousEvaluation; }
	void SetTargetDirty(size_t target, bool onlyChild = false);
	const EvaluationInfo& GetEvaluationInfo() const { return mEvaluationInfo; }

	bool StageIsProcessing(size_t target) const { if (target >= mbProcessing.size()) return false; return mbProcessing[target]; }
	void StageSetProcessing(size_t target, bool processing) { mbProcessing.resize(mEvaluation.GetStagesCount(), false); mbProcessing[target] = processing; }

	void AllocRenderTargetsForEditingPreview();
protected:
	Evaluation& mEvaluation;

	void PreRun();
	void EvaluateGLSL(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo);
	void EvaluateC(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo);
	void EvaluatePython(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo);

	void RunNodeList(const std::vector<size_t>& nodesToEvaluate);
	void RunNode(size_t nodeIndex);

	void RecurseBackward(size_t target, std::vector<size_t>& usedNodes);

	
	void AllocRenderTargetsForBaking(const std::vector<size_t>& nodesToEvaluate);

	std::vector<RenderTarget*> mStageTarget; // 1 per stage
	std::vector<RenderTarget*> mAllocatedTargets; // allocated RT, might be present multiple times in mStageTarget
	std::map<std::string, FFMPEGCodec::Encoder*> mWriteStreams;
	std::vector<bool> mbDirty;
	std::vector<bool> mbProcessing;
	EvaluationInfo mEvaluationInfo;

	int mDefaultWidth;
	int mDefaultHeight;
	bool mbSynchronousEvaluation;
};

extern EvaluationContext *gCurrentContext;
