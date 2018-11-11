#pragma once
#include "Evaluation.h"

struct EvaluationContext
{
	EvaluationContext(Evaluation& evaluation, bool synchronousEvaluation, int defaultWidth, int defaultHeight);
	~EvaluationContext();

	void RunAll();
	void RunForward(size_t nodeIndex);
	void RunBackward(size_t nodeIndex);


	unsigned int GetEvaluationTexture(size_t target);
	RenderTarget *GetRenderTarget(size_t target) { return mStageTarget[target]; }

	FFMPEGCodec::Encoder *GetEncoder(const std::string &filename, int width, int height);
	bool IsSynchronous() const { return mbSynchronousEvaluation; }

protected:
	Evaluation& mEvaluation;

	void EvaluateGLSL(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo);
	void EvaluateC(const EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo);
	void RunNodeList(const std::vector<size_t>& nodesToEvaluate);

	void RecurseBackward(size_t target, std::vector<size_t>& usedNodes);
	void RecurseForward(size_t base, size_t parent, std::vector<size_t>& usedNodes);

	std::vector<RenderTarget*> mStageTarget; // 1 per stage
	std::vector<RenderTarget*> mAllocatedTargets; // allocated RT, might be present multiple times in mStageTarget
	std::map<std::string, FFMPEGCodec::Encoder*> mWriteStreams;

	int mDefaultWidth;
	int mDefaultHeight;
	bool mbSynchronousEvaluation;
};

extern EvaluationContext *gCurrentContext;