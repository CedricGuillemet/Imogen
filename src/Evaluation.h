#pragma once
#include <string>
#include <vector>
#include <map>
#include "Library.h"
#include "libtcc/libtcc.h"

typedef unsigned int TextureID;

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

	void SetEvaluators(const std::vector<std::string>& GLSLfilenames, const std::vector<std::string>& Cfilenames);
	std::string GetEvaluationGLSL(const std::string& filename);

	std::string GetEvaluationC(const std::string& filename);

	size_t AddEvaluationGLSL(size_t nodeType, const std::string& nodeName);
	size_t AddEvaluationC(size_t nodeType, const std::string& nodeName);

	void DelEvaluationTarget(size_t target);
	unsigned int GetEvaluationTexture(size_t target);
	void SetEvaluationParameters(size_t target, void *parameters, size_t parametersSize);
	void SetEvaluationSampler(size_t target, const std::vector<InputSampler>& inputSamplers);
	void AddEvaluationInput(size_t target, int slot, int source);
	void DelEvaluationInput(size_t target, int slot);
	void RunEvaluation();
	void SetEvaluationOrder(const std::vector<size_t> nodeOrderList);
	void SetTargetDirty(size_t target);
	void Bake(const char *szFilename, size_t target, int width, int height);

	void Clear();

	// API
	static int ReadImage(char *filename, Image *image);
	static int WriteImage(char *filename, Image *image, int format, int quality);
	static int GetEvaluationImage(int target, Image *image);
	static int SetEvaluationImage(int target, Image *image);
	static int AllocateImage(Image *image);
	static int FreeImage(Image *image);

protected:
	
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

	std::vector<Evaluator> mProgramPerNodeType;
	struct Shader
	{
		std::string mShaderText;
		unsigned int mProgram;
		int mNodeType;
	};
	std::map<std::string, Shader> mGLSLs;

	struct CProgram
	{
		std::string mCText;
		int(*mCFunction)(void *parameters, void *evaluationInfo);
		void *mMem;
		int mNodeType;
	};
	std::map<std::string, CProgram> mCPrograms;

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
		int mEvaluationType; // 0 = GLSL. 1 = CPU C
		void Clear();
	};

	std::vector<EvaluationStage> mEvaluations;
	std::vector<size_t> mEvaluationOrderList;

};