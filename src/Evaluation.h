#pragma once
#include <string>
#include <vector>
typedef unsigned int TextureID;

inline void TexParam(TextureID MinFilter, TextureID MagFilter, TextureID WrapS, TextureID WrapT, TextureID texMode)
{
	glTexParameteri(texMode, GL_TEXTURE_MIN_FILTER, MinFilter);
	glTexParameteri(texMode, GL_TEXTURE_MAG_FILTER, MagFilter);
	glTexParameteri(texMode, GL_TEXTURE_WRAP_S, WrapS);
	glTexParameteri(texMode, GL_TEXTURE_WRAP_T, WrapT);
}

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

class FullScreenTriangle
{
public:
	FullScreenTriangle() : mGLFullScreenVertexArrayName(-1)
	{
	}
	~FullScreenTriangle()
	{
	}
	void Init();
	void Render();
protected:
	TextureID mGLFullScreenVertexArrayName;
};

// simple API
void InitEvaluation(const std::string& shaderString);
void UpdateEvaluationShader(const std::string& shaderString);
unsigned int AddEvaluationTarget();
void DelEvaluationTarget(int target);
unsigned int GetEvaluationTexture(int target);
void SetEvaluationCall(int target, const std::string& shaderCall);
void AddEvaluationInput(int target, int slot, int source);
void DelEvaluationInput(int target, int slot);
void RunEvaluation();
void SetEvaluationOrder(const std::vector<int> nodeOrderList);
void SetTargetDirty(int target);
