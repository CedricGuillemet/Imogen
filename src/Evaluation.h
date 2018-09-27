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
#include <string>
#include <vector>
#include <map>
#include "Library.h"
#include "libtcc/libtcc.h"
#include "Imogen.h"

extern int Log(const char *szFormat, ...);

typedef unsigned int TextureID;
enum EvaluationStatus
{
	EVAL_OK,
	EVAL_ERR,
};

typedef struct Image_t
{
	int width, height;
	int components;
	void *bits;
} Image;

class RenderTarget
{

public:
	RenderTarget() : mGLTexID(0)
	{
		fbo = 0;
		depthbuffer = 0;
		mWidth = mHeight = 0;
	}

	void initBuffer(int width, int height, bool hasZBuffer);
	void bindAsTarget() const;

	void clear();

	TextureID txDepth;
	unsigned int mGLTexID;
	int mWidth, mHeight;
	TextureID fbo;
	TextureID depthbuffer;

	void destroy();

	void checkFBO();
};


// simple API
struct Evaluation
{
	Evaluation();

	void Init();
	void Finish();

	void SetEvaluators(const std::vector<EvaluatorFile>& evaluatorfilenames);
	std::string GetEvaluator(const std::string& filename);

	size_t AddEvaluation(size_t nodeType, const std::string& nodeName);

	void DelEvaluationTarget(size_t target);
	unsigned int GetEvaluationTexture(size_t target);
	void SetEvaluationParameters(size_t target, void *parameters, size_t parametersSize);
	void ForceEvaluation(size_t target);
	void SetEvaluationSampler(size_t target, const std::vector<InputSampler>& inputSamplers);
	void AddEvaluationInput(size_t target, int slot, int source);
	void DelEvaluationInput(size_t target, int slot);
	void RunEvaluation();
	void RunEvaluation(int target, int width, int height);
	void SetEvaluationOrder(const std::vector<size_t> nodeOrderList);
	void SetTargetDirty(size_t target);

	void Clear();

	// API
	static int ReadImage(const char *filename, Image *image);
	static int WriteImage(const char *filename, Image *image, int format, int quality);
	static int GetEvaluationImage(int target, Image *image);
	static int SetEvaluationImage(int target, Image *image);
	static int SetThumbnailImage(Image *image);
	static int AllocateImage(Image *image);
	static int FreeImage(Image *image);
	static unsigned int UploadImage(Image *image);
	static void Evaluate(int target, int width, int height);

	// synchronous texture cache
	// use for simple textures(stock) or to replace with a more efficient one
	unsigned int GetTexture(const std::string& filename);
protected:
	void APIInit();
	std::map<std::string, unsigned int> mSynchronousTextureCache;
	

	unsigned int equiRectTexture;
	int mDirtyCount;

	void ClearEvaluators();
	struct Evaluator
	{
		Evaluator() : mGLSLProgram(0), mCFunction(0), mMem(0) {}
		unsigned int mGLSLProgram;
		int(*mCFunction)(void *parameters, void *evaluationInfo);
		void *mMem;
	};

	std::vector<Evaluator> mEvaluatorPerNodeType;

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

	struct Input
	{
		Input()
		{
			memset(mInputs, -1, sizeof(int) * 8);
		}
		int mInputs[8];
	};

	struct EvaluationStage
	{
		RenderTarget mTarget;
		size_t mNodeType;
		unsigned int mParametersBuffer;
		void *mParameters;
		size_t mParametersSize;
		Input mInput;
		std::vector<InputSampler> mInputSamplers;
		bool mbDirty;
		bool mbForceEval;
		int mEvaluationType; // 0 = GLSL. 1 = CPU C
		void Clear();
	};

	// Transient textures
	struct TransientTarget
	{
		RenderTarget mTarget;
		int mUseCount;
	};

	std::vector<EvaluationStage> mEvaluations;
	std::vector<size_t> mEvaluationOrderList;

	void BindGLSLParameters(EvaluationStage& stage);
	void EvaluateGLSL(const EvaluationStage& evaluation, const RenderTarget &tg, std::vector<TransientTarget*> *evaluationTransientTargets);
	void EvaluateC(const EvaluationStage& evaluation, size_t index);
	void FinishEvaluation();

	static std::vector<TransientTarget*> mFreeTargets;
	static int mTransientTextureMaxCount;
	static TransientTarget* GetTransientTarget(int width, int height, int useCount);
	static void LoseTransientTarget(TransientTarget *transientTarget);
};