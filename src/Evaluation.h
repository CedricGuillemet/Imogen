#pragma once
#include <string>
#include <vector>
#include <map>
#include "Library.h"

typedef unsigned int TextureID;

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

	void SetEvaluationGLSL(const std::vector<std::string>& filenames);
	std::string GetEvaluationGLSL(const std::string& filename);


	void LoadEquiRectHDREnvLight(const std::string& filepath);
	void LoadEquiRect(const std::string& filepath);

	size_t AddEvaluationTarget(size_t nodeType, const std::string& nodeName);
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
protected:
	
	unsigned int equiRectTexture;
	int mDirtyCount;
	std::vector<unsigned int> mProgramPerNodeType;
	struct Shader
	{
		std::string mShaderText;
		unsigned int mProgram;
		int mNodeType;
	};
	std::map<std::string, Shader> mGLSLs;

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

		void Clear();
	};

	std::vector<EvaluationStage> mEvaluations;
	std::vector<size_t> mEvaluationOrderList;
};