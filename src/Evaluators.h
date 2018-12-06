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

#include <vector>
#include <map>
#include <string>
#include "Imogen.h"
#include "pybind11/embed.h"

struct Evaluator
{
	Evaluator() : mGLSLProgram(0), mCFunction(0), mMem(0) {}
	unsigned int mGLSLProgram;
	int(*mCFunction)(void *parameters, void *evaluationInfo);
	void *mMem;
	pybind11::module mPyModule;

	void RunPython() const;

#ifdef _DEBUG
	std::string mName;
#endif
};

struct Evaluators
{
	Evaluators() : gEvaluationStateGLSLBuffer(0) {}
	void SetEvaluators(const std::vector<EvaluatorFile>& evaluatorfilenames);
	std::string GetEvaluator(const std::string& filename);
	int GetMask(size_t nodeType);
	void ClearEvaluators();

	const Evaluator& GetEvaluator(size_t nodeType) const { return mEvaluatorPerNodeType[nodeType]; }

	unsigned int gEvaluationStateGLSLBuffer;
	pybind11::module mImogenModule;
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
		pybind11::module mPyModule;
		
	};

	std::map<std::string, EvaluatorScript> mEvaluatorScripts;
	std::vector<Evaluator> mEvaluatorPerNodeType;
	
};

extern Evaluators gEvaluators;
