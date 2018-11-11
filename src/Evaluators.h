#pragma once

#include <vector>
#include <map>
#include <string>
#include "Imogen.h"

struct Evaluator
{
	Evaluator() : mGLSLProgram(0), mCFunction(0), mMem(0) {}
	unsigned int mGLSLProgram;
	int(*mCFunction)(void *parameters, void *evaluationInfo);
	void *mMem;
};

struct Evaluators
{
	Evaluators() : mEvaluationStateGLSLBuffer(0) {}
	void SetEvaluators(const std::vector<EvaluatorFile>& evaluatorfilenames);
	std::string GetEvaluator(const std::string& filename);
	int GetMask(size_t nodeType, const std::string& nodeName);
	void ClearEvaluators();

	const Evaluator& GetEvaluator(size_t nodeType) const { return mEvaluatorPerNodeType[nodeType]; }


	unsigned int mEvaluationStateGLSLBuffer;

protected:

	struct EvaluatorScript
	{
		EvaluatorScript() : mProgram(0), mCFunction(0), mMem(0), mNodeType(-1) {}
		EvaluatorScript(const std::string & text) : mText(text), mProgram(0), mCFunction(0), mMem(0), mNodeType(-1) {}
		std::string mText;
		unsigned int mProgram;
		int(*mCFunction)(void *parameters, void *evaluationInfo);
		void *mMem;
		int mNodeType;
	};

	std::map<std::string, EvaluatorScript> mEvaluatorScripts;
	std::vector<Evaluator> mEvaluatorPerNodeType;

	
};

extern Evaluators gEvaluators;