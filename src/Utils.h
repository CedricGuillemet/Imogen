#pragma once

#include <string>

typedef unsigned int TextureID;
static const int SemUV0 = 0;

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


void TexParam(TextureID MinFilter, TextureID MagFilter, TextureID WrapS, TextureID WrapT, TextureID texMode);

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);

unsigned int LoadShader(const std::string &shaderString, const char *fileName);
int Log(const char *szFormat, ...);