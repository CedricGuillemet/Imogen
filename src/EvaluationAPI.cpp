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
#include "Evaluation.h"
#include <vector>
#include <algorithm>
#include <assert.h>
#include <SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <fstream>
#include <streambuf>
#include "imgui.h"
#include "imgui_internal.h"
#include "cmft/image.h"
#include "cmft/cubemapfilter.h"
#include "TaskScheduler.h"
#include "NodesDelegate.h"
#include "cmft/print.h"
#include "ffmpegDecode.h"

extern enki::TaskScheduler g_TS;

static const int SemUV0 = 0;
static const unsigned int wrap[] = { GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, GL_MIRRORED_REPEAT };
static const unsigned int filter[] = { GL_LINEAR, GL_NEAREST };
static const char* samplerName[] = { "Sampler0", "Sampler1", "Sampler2", "Sampler3", "Sampler4", "Sampler5", "Sampler6", "Sampler7", "CubeSampler0" };
static const unsigned int GLBlends[] = { GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR,GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA, GL_SRC_ALPHA_SATURATE };
static const unsigned int glInputFormats[] = {
		GL_BGR,
		GL_RGB,
		GL_RGB16,
		GL_RGB16F,
		GL_RGB32F,
		GL_RGBA, // RGBE

		GL_BGRA,
		GL_RGBA,
		GL_RGBA16,
		GL_RGBA16F,
		GL_RGBA32F,

		GL_RGBA, // RGBM
};
static const unsigned int glInternalFormats[] = {
	GL_RGB,
	GL_RGB,
	GL_RGB16,
	GL_RGB16F,
	GL_RGB32F,
	GL_RGBA, // RGBE

	GL_RGBA,
	GL_RGBA,
	GL_RGBA16,
	GL_RGBA16F,
	GL_RGBA32F,

	GL_RGBA, // RGBM
};
static const unsigned int glCubeFace[] = {
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};
static const unsigned int textureFormatSize[] = {    3,3,6,6,12, 4,4,4,8,8,16,4 };
static const unsigned int textureComponentCount[] = { 3,3,3,3,3, 4,4,4,4,4,4,4 };

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

unsigned int GetTexelSize(uint8_t fmt)
{
	return textureFormatSize[fmt];
}

inline void TexParam(TextureID MinFilter, TextureID MagFilter, TextureID WrapS, TextureID WrapT, TextureID texMode)
{
	glTexParameteri(texMode, GL_TEXTURE_MIN_FILTER, MinFilter);
	glTexParameteri(texMode, GL_TEXTURE_MAG_FILTER, MagFilter);
	glTexParameteri(texMode, GL_TEXTURE_WRAP_S, WrapS);
	glTexParameteri(texMode, GL_TEXTURE_WRAP_T, WrapT);
}

void RenderTarget::BindAsTarget() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
	glViewport(0, 0, mImage.mWidth, mImage.mHeight);
}

void RenderTarget::BindAsCubeTarget() const
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, mGLTexID);
	glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
	glViewport(0, 0, mImage.mWidth, mImage.mHeight);
}

void RenderTarget::BindCubeFace(size_t face)
{
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face), mGLTexID, 0);
}

void RenderTarget::Destroy()
{
	if (mGLTexID)
		glDeleteTextures(1, &mGLTexID);
	if (mFbo)
		glDeleteFramebuffers(1, &mFbo);
	mFbo = 0;
	mImage.mWidth = mImage.mHeight = 0;
	mGLTexID = 0;
}

void RenderTarget::InitBuffer(int width, int height)
{
	if ((width == mImage.mWidth) && (mImage.mHeight == height) && mImage.mNumFaces == 1)
		return;
	Destroy();

	mImage.mWidth = width;
	mImage.mHeight = height;
	mImage.mNumMips = 1;
	mImage.mNumFaces = 1;
	mImage.mStream = NULL;
	mImage.mFormat = TextureFormat::RGBA8;

	glGenFramebuffers(1, &mFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

	// diffuse
	glGenTextures(1, &mGLTexID);
	glBindTexture(GL_TEXTURE_2D, mGLTexID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	TexParam(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_2D);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mGLTexID, 0);

	static const GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(sizeof(DrawBuffers) / sizeof(GLenum), DrawBuffers);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	CheckFBO();
	BindAsTarget();
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void RenderTarget::InitCube(int width)
{
	if ( (width == mImage.mWidth) && (mImage.mHeight == width) && mImage.mNumFaces == 6)
		return;
	Destroy();

	mImage.mWidth = width;
	mImage.mHeight = width;
	mImage.mNumMips = 1;
	mImage.mNumFaces = 6;
	mImage.mStream = NULL;
	mImage.mFormat = TextureFormat::RGBA8;

	glGenFramebuffers(1, &mFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

	glGenTextures(1, &mGLTexID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mGLTexID);
	
	for (int i = 0; i < 6; i++)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, width, width, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		

	TexParam(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_CUBE_MAP);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, mGLTexID, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	CheckFBO();
}

void RenderTarget::CheckFBO()
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

	int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch (status)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		//Log("Framebuffer complete.\n");
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		Log("[ERROR] Framebuffer incomplete: Attachment is NOT complete.");
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		Log("[ERROR] Framebuffer incomplete: No image is attached to FBO.");
		break;
		/*
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		Log("[ERROR] Framebuffer incomplete: Attached images have different dimensions.");
		break;

		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
		Log("[ERROR] Framebuffer incomplete: Color attached images have different internal formats.");
		break;
		*/
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		Log("[ERROR] Framebuffer incomplete: Draw buffer.\n");
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		Log("[ERROR] Framebuffer incomplete: Read buffer.\n");
		break;

	case GL_FRAMEBUFFER_UNSUPPORTED:
		Log("[ERROR] Unsupported by FBO implementation.\n");
		break;

	default:
		Log("[ERROR] Unknow error.\n");
		break;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

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

void FullScreenTriangle::Init()
{
	TextureID fsVA;

	float fsVts[] = { 0.f,0.f, 2.f,0.f, 0.f,2.f };
	glGenBuffers(1, &fsVA);
	glBindBuffer(GL_ARRAY_BUFFER, fsVA);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(float) * 2, fsVts, GL_STATIC_DRAW);

	glGenVertexArrays(1, &mGLFullScreenVertexArrayName);
	glBindVertexArray(mGLFullScreenVertexArrayName);
	glBindBuffer(GL_ARRAY_BUFFER, fsVA);
	glVertexAttribPointer(SemUV0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(SemUV0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void FullScreenTriangle::Render()
{
	glBindVertexArray(mGLFullScreenVertexArrayName);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

unsigned int LoadShader(const std::string &shaderString, const char *fileName)
{
	TextureID programObject = glCreateProgram();
	if (programObject == 0)
		return 0;

	GLint compiled;
	const char *shaderTypeStrings[] = { "\n#version 430 core\n#define VERTEX_SHADER\n", "\n#version 430 core\n#define FRAGMENT_SHADER\n" };
	TextureID shaderTypes[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
	TextureID compiledShader[2];

	for (int i = 0; i<2; i++)
	{
		// Create the shader object
		int shader = glCreateShader(shaderTypes[i]);

		if (shader == 0)
			return false;

		int stringsCount = 2;
		const char ** strings = (const char**)malloc(sizeof(char*) * stringsCount); //new const char*[stringsCount];
		int * stringLength = (int*)malloc(sizeof(int) * stringsCount); //new int[stringsCount];
		strings[0] = shaderTypeStrings[i];
		stringLength[0] = int(strlen(shaderTypeStrings[i]));
		strings[stringsCount - 1] = shaderString.c_str();
		stringLength[stringsCount - 1] = int(shaderString.length());

		// Load and compile the shader source
		glShaderSource(shader, stringsCount, strings, stringLength);
		glCompileShader(shader);


		free(stringLength);
		free(strings);

		// Check the compile status
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (compiled == 0)
		{
			GLint info_len = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
			if (info_len > 1)
			{
				char* info_log = (char*)malloc(sizeof(char) * info_len);
				glGetShaderInfoLog(shader, info_len, NULL, info_log);
				Log("Error compiling shader: %s \n", fileName);
				Log(info_log);
				Log("\n");
				free(info_log);
			}
			glDeleteShader(shader);
			return 0;
		}
		compiledShader[i] = shader;
	}



	GLint linked;

	for (int i = 0; i<2; i++)
		glAttachShader(programObject, compiledShader[i]);


	// Link the program
	glLinkProgram(programObject);

	glBindAttribLocation(programObject, SemUV0, "inUV");

	// Check the link status
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
	if (linked == 0)
	{
		GLint info_len = 0;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &info_len);
		if (info_len > 1)
		{
			char* info_log = (char*)malloc(sizeof(char) * info_len);
			glGetProgramInfoLog(programObject, info_len, NULL, info_log);
			Log("Error linking program:\n");
			Log(info_log);
			free(info_log);
		}
		glDeleteProgram(programObject);
		return 0;
	}

	// Delete these here because they are attached to the program object.
	for (int i = 0; i<2; i++)
		glDeleteShader(compiledShader[i]);

	// attributes
	return programObject;
}

FullScreenTriangle mFSQuad;

void Evaluation::APIInit()
{
	mFSQuad.Init();

	std::ifstream prgStr("Stock/ProgressingNode.glsl");
	std::ifstream cubStr("Stock/DisplayCubemap.glsl");

	mProgressShader = prgStr.good() ? LoadShader(std::string(std::istreambuf_iterator<char>(prgStr), std::istreambuf_iterator<char>()), "progressShader") : 0;
	mDisplayCubemapShader = cubStr.good() ? LoadShader(std::string(std::istreambuf_iterator<char>(cubStr), std::istreambuf_iterator<char>()), "cubeDisplay") : 0;
}

static void libtccErrorFunc(void *opaque, const char *msg)
{
	Log(msg);
	Log("\n");
}

struct EValuationFunction
{
	const char *szFunctionName;
	void *function;
};

extern Evaluation gEvaluation;

Image_t Evaluation::EvaluationStream::DecodeImage()
{
	decoder.ReadFrame(gEvaluationTime + 1);
	Image_t image;
	image.mNumMips = 1;
	image.mNumFaces = 1;
	image.mFormat = TextureFormat::BGR8;
	image.mWidth = decoder.mWidth;
	image.mHeight = decoder.mHeight;
	image.mStream = this;
	image.mFrameDuration = decoder.mFrameCount;
	size_t lineSize = image.mWidth * 3;
	size_t imgDataSize = lineSize * image.mHeight;
	image.mBits = (unsigned char*)malloc(imgDataSize);

	unsigned char *pdst = image.mBits;
	unsigned char *psrc = (unsigned char*)decoder.GetRGBData();
	
	psrc += imgDataSize - lineSize;
	for (int j = 0; j < image.mHeight; j++)
	{
		memcpy(pdst, psrc, lineSize);
		pdst += lineSize;
		psrc -= lineSize;
	}
	return image;
}

int Evaluation::ReadImage(const char *filename, Image *image)
{
	int components;
	unsigned char *bits = stbi_load(filename, &image->mWidth, &image->mHeight, &components, 0);
	if (!bits)
	{
		cmft::Image img;
		if (!cmft::imageLoad(img, filename))
		{
			EvaluationStream *stream = new EvaluationStream;
			if (stream->decoder.Open(std::string(filename)))
			{
				*image = stream->DecodeImage();
				return EVAL_OK;
			}
			delete stream;
			return EVAL_ERR;
		}
		cmft::imageTransformUseMacroInstead(&img, cmft::IMAGE_OP_FLIP_X, UINT32_MAX);
		image->mBits = (unsigned char*)img.m_data;
		image->mWidth = img.m_width;
		image->mHeight = img.m_height;
		image->mDataSize = img.m_dataSize;
		image->mNumMips = img.m_numMips;
		image->mNumFaces = img.m_numFaces;
		image->mFormat = img.m_format;
		image->mStream = NULL;

		return EVAL_OK;
	}

	image->mBits = bits;
	image->mDataSize = image->mWidth * image->mHeight * components;
	image->mNumMips = 1;
	image->mNumFaces = 1;
	image->mFormat = (components == 3) ? TextureFormat::RGB8 : TextureFormat::RGBA8;
	image->mStream = NULL;

	return EVAL_OK;
}

int Evaluation::ReadImageMem(unsigned char *data, size_t dataSize, Image *image)
{
	int components;
	unsigned char *bits = stbi_load_from_memory(data, int(dataSize), &image->mWidth, &image->mHeight, &components, 0);
	if (!bits)
		return EVAL_ERR;
	image->mBits = bits;
	return EVAL_OK;
}

int Evaluation::WriteImage(const char *filename, Image *image, int format, int quality)
{
	int components = textureComponentCount[image->mFormat];
	switch (format)
	{
	case 0:
		if (!stbi_write_jpg(filename, image->mWidth, image->mHeight, components, image->mBits, quality))
			return EVAL_ERR;
		break;
	case 1:
		if (!stbi_write_png(filename, image->mWidth, image->mHeight, components, image->mBits, image->mWidth * components))
			return EVAL_ERR;
		break;
	case 2:
		if (!stbi_write_tga(filename, image->mWidth, image->mHeight, components, image->mBits))
			return EVAL_ERR;
		break;
	case 3:
		if (!stbi_write_bmp(filename, image->mWidth, image->mHeight, components, image->mBits))
			return EVAL_ERR;
		break;
	case 4:
		//if (stbi_write_hdr(filename, image->width, image->height, image->components, image->bits))
			return EVAL_ERR;
		break;
	case 5:
	{
		cmft::Image img;
		img.m_format = (cmft::TextureFormat::Enum)image->mFormat;
		img.m_width = image->mWidth;
		img.m_height = image->mHeight;
		img.m_numFaces = image->mNumFaces;
		img.m_numMips = image->mNumMips;
		img.m_data = image->mBits;
		img.m_dataSize = image->mDataSize;
		if (img.m_format == cmft::TextureFormat::RGBA8)
			cmft::imageConvert(img, cmft::TextureFormat::BGRA8);
		else if (img.m_format == cmft::TextureFormat::RGB8)
			cmft::imageConvert(img, cmft::TextureFormat::BGR8);
		image->mBits = (unsigned char*)img.m_data;
		if (!cmft::imageSave(img, filename, cmft::ImageFileType::DDS))
			return EVAL_ERR;
	}
		break;
	case 6:
	{
		cmft::Image img;
		img.m_format = (cmft::TextureFormat::Enum)image->mFormat;
		img.m_width = image->mWidth;
		img.m_height = image->mHeight;
		img.m_numFaces = image->mNumFaces;
		img.m_numMips = image->mNumMips;
		img.m_data = image->mBits;
		img.m_dataSize = image->mDataSize;
		if (!cmft::imageSave(img, filename, cmft::ImageFileType::KTX))
			return EVAL_ERR;
	}
	break;
	}
	return EVAL_OK;
}

int Evaluation::GetEvaluationImage(int target, Image *image)
{
	if (target == -1 || target >= gEvaluation.mEvaluationStages.size())
		return EVAL_ERR;

	Evaluation::EvaluationStage &evaluation = gEvaluation.mEvaluationStages[target];
	if (!evaluation.mTarget)
		return EVAL_ERR;

	RenderTarget& tgt = *evaluation.mTarget;

	// compute total size
	Image_t& img = tgt.mImage;
	unsigned int texelSize = GetTexelSize(img.mFormat);
	unsigned int texelFormat = glInternalFormats[img.mFormat];
	uint32_t size = 0;// img.mNumFaces * img.mWidth * img.mHeight * texelSize;
	for (int i = 0;i<img.mNumMips;i++)
		size += img.mNumFaces * (img.mWidth >> i) * (img.mHeight >> i) * texelSize;

	image->mBits = (unsigned char*)malloc(size);
	image->mDataSize = size;
	image->mWidth = img.mWidth;
	image->mHeight = img.mHeight;
	image->mNumMips = img.mNumMips;
	image->mFormat = img.mFormat;
	image->mNumFaces = img.mNumFaces;
	image->mStream = img.mStream;
	image->mFrameDuration = 1;

	glBindTexture(GL_TEXTURE_2D, tgt.mGLTexID);
	unsigned char *ptr = (unsigned char *)image->mBits;
	if (img.mNumFaces == 1)
	{
		for (int i = 0; i < img.mNumMips; i++)
		{
			glGetTexImage(GL_TEXTURE_2D, i, texelFormat, GL_UNSIGNED_BYTE, ptr);
			ptr += (img.mWidth >> i) * (img.mHeight >> i) * texelSize;
		}
	}
	else
	{
		for (int cube = 0; cube < img.mNumFaces; cube++)
		{
			for (int i = 0; i < img.mNumMips; i++)
			{
				glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+cube, i, texelFormat, GL_UNSIGNED_BYTE, ptr);
				ptr += (img.mWidth >> i) * (img.mHeight >> i) * texelSize;
			}
		}
	}
	return EVAL_OK;
}

int Evaluation::SetEvaluationImage(int target, Image *image)
{
	Evaluation::EvaluationStage &evaluation = gEvaluation.mEvaluationStages[target];
	if (!evaluation.mTarget)
	{
		evaluation.mTarget = new RenderTarget;
	}
	evaluation.mbFreeSizing = false;
	unsigned int texelSize = GetTexelSize(image->mFormat);
	unsigned int inputFormat = glInputFormats[image->mFormat];
	unsigned int internalFormat = glInternalFormats[image->mFormat];
	unsigned char *ptr = (unsigned char *)image->mBits;
	if (image->mNumFaces == 1)
	{
		evaluation.mTarget->InitBuffer(image->mWidth, image->mHeight);

		glBindTexture(GL_TEXTURE_2D, evaluation.mTarget->mGLTexID);

		for (int i = 0; i < image->mNumMips; i++)
		{
			glTexImage2D(GL_TEXTURE_2D, i, internalFormat, image->mWidth >> i, image->mHeight >> i, 0, inputFormat, GL_UNSIGNED_BYTE, ptr);
			ptr += (image->mWidth >> i) * (image->mHeight >> i) * texelSize;
		}

		if (image->mNumMips > 1)
			TexParam(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_2D);
		else
			TexParam(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_2D);
	}
	else
	{
		evaluation.mTarget->InitCube(image->mWidth);
		glBindTexture(GL_TEXTURE_CUBE_MAP, evaluation.mTarget->mGLTexID);

		for (int face = 0; face < image->mNumFaces; face++)
		{
			for (int i = 0; i < image->mNumMips; i++)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, i, internalFormat, image->mWidth >> i, image->mWidth >> i, 0, inputFormat, GL_UNSIGNED_BYTE, ptr);
				ptr += (image->mWidth >> i) * (image->mWidth >> i) * texelSize;
			}
		}

		if (image->mNumMips > 1)
			TexParam(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_CUBE_MAP);
		else
			TexParam(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_CUBE_MAP);

	}
	evaluation.mTarget->mImage.mFrameDuration = image->mFrameDuration;
	evaluation.mTarget->mImage.mStream = image->mStream;

	// not a big fan of this
	if (image->mFrameDuration > 1)
		TileNodeEditGraphDelegate::GetInstance()->SetTimeDuration(target, image->mFrameDuration);
	gEvaluation.SetTargetDirty(target, true);
	return EVAL_OK;
}

int Evaluation::SetEvaluationImageCube(int target, Image *image, int cubeFace)
{
	if (image->mNumFaces != 1)
		return EVAL_ERR;
	Evaluation::EvaluationStage &evaluation = gEvaluation.mEvaluationStages[target];
	if (!evaluation.mTarget)
	{
		evaluation.mTarget = new RenderTarget;
	}
	evaluation.mbFreeSizing = false;
	evaluation.mTarget->InitCube(image->mWidth);

	UploadImage(image, evaluation.mTarget->mGLTexID, cubeFace);
	gEvaluation.SetTargetDirty(target, true);
	return EVAL_OK;
}

int Evaluation::CubemapFilter(Image *image, int faceSize, int lightingModel, int excludeBase, int glossScale, int glossBias)
{
	cmft::Image img;
	img.m_data = image->mBits;
	img.m_dataSize = image->mDataSize;
	img.m_numMips = image->mNumMips;
	img.m_numFaces = image->mNumFaces;
	img.m_width = image->mWidth;
	img.m_height = image->mHeight;
	img.m_format = (cmft::TextureFormat::Enum)image->mFormat;

	extern unsigned int gCPUCount;

	cmft::setWarningPrintf(Log);
	cmft::setInfoPrintf(Log);

	faceSize = 16;
	if (!cmft::imageRadianceFilter(img
		, faceSize // face size
		, (cmft::LightingModel::Enum)lightingModel
		, (excludeBase != 0)
		, uint8_t(log2(faceSize)) // map mip count
		, glossScale
		, glossBias
		, cmft::EdgeFixup::None
		, gCPUCount))
		return EVAL_ERR;

	image->mBits = (unsigned char*)img.m_data;
	image->mDataSize = img.m_dataSize;
	image->mNumMips = img.m_numMips;
	image->mNumFaces = img.m_numFaces;
	image->mWidth = img.m_width;
	image->mHeight = img.m_height;
	image->mFormat = img.m_format;
	image->mStream = NULL;
	return EVAL_OK;
}

int Evaluation::AllocateImage(Image *image)
{
	return EVAL_OK;
}

int Evaluation::FreeImage(Image *image)
{
	free(image->mBits);
	image->mBits = NULL;
	return EVAL_OK;
}

int Evaluation::EncodePng(Image *image, std::vector<unsigned char> &pngImage)
{
	int outlen;
	int components = 4; // TODO
	unsigned char *bits = stbi_write_png_to_mem((unsigned char*)image->mBits, image->mWidth * components, image->mWidth, image->mHeight, components, &outlen);
	if (!bits)
		return EVAL_ERR;
	pngImage.resize(outlen);
	memcpy(pngImage.data(), bits, outlen);

	free(bits);
	return EVAL_OK;
}

int Evaluation::SetThumbnailImage(Image *image)
{
	std::vector<unsigned char> pngImage;
	if (EncodePng(image, pngImage) == EVAL_ERR)
		return EVAL_ERR;

	extern Library library;
	extern Imogen imogen;

	int materialIndex = imogen.GetCurrentMaterialIndex();
	Material & material = library.mMaterials[materialIndex];
	material.mThumbnail = pngImage;
	material.mThumbnailTextureId = 0;
	return EVAL_OK;
}

int Evaluation::SetNodeImage(int target, Image *image)
{
	std::vector<unsigned char> pngImage;
	if (EncodePng(image, pngImage) == EVAL_ERR)
		return EVAL_ERR;

	extern Library library;
	extern Imogen imogen;

	int materialIndex = imogen.GetCurrentMaterialIndex();
	Material & material = library.mMaterials[materialIndex];
	material.mMaterialNodes[target].mImage = pngImage;

	return EVAL_OK;
}

void Evaluation::RecurseGetUse(size_t target, std::vector<size_t>& usedNodes)
{
	EvaluationStage& evaluation = mEvaluationStages[target];
	const Input& input = evaluation.mInput;

	std::vector<RenderTarget*> usingTargets;
	for (size_t inputIndex = 0; inputIndex < 8; inputIndex++)
	{
		int targetIndex = input.mInputs[inputIndex];
		if (targetIndex == -1)
			continue;
		RecurseGetUse(targetIndex, usedNodes);
	}

	if (std::find(usedNodes.begin(), usedNodes.end(), target) == usedNodes.end())
		usedNodes.push_back(target);
}

int Evaluation::Evaluate(int target, int width, int height, Image *image)
{
	std::vector<size_t> svgEvalList = gEvaluation.mEvaluationOrderList;
	gEvaluation.mEvaluationOrderList.clear();

	gEvaluation.RecurseGetUse(target, gEvaluation.mEvaluationOrderList);

	gEvaluation.SetEvaluationMemoryMode(1);

	gEvaluation.RunEvaluation(width, height, true);
	GetEvaluationImage(target, image);
	gEvaluation.SetEvaluationMemoryMode(0);

	gEvaluation.mEvaluationOrderList = svgEvalList;

	gEvaluation.RunEvaluation(256, 256, true);
	return EVAL_OK;
}

static const EValuationFunction evaluationFunctions[] = {
	{ "Log", (void*)Log },
	{ "ReadImage", (void*)Evaluation::ReadImage },
	{ "WriteImage", (void*)Evaluation::WriteImage },
	{ "GetEvaluationImage", (void*)Evaluation::GetEvaluationImage },
	{ "SetEvaluationImage", (void*)Evaluation::SetEvaluationImage },
	{ "SetEvaluationImageCube", (void*)Evaluation::SetEvaluationImageCube },
	{ "AllocateImage", (void*)Evaluation::AllocateImage },
	{ "FreeImage", (void*)Evaluation::FreeImage },
	{ "SetThumbnailImage", (void*)Evaluation::SetThumbnailImage },
	{ "Evaluate", (void*)Evaluation::Evaluate},
	{ "SetBlendingMode", (void*)Evaluation::SetBlendingMode},
	{ "GetEvaluationSize", (void*)Evaluation::GetEvaluationSize},
	{ "SetEvaluationSize", (void*)Evaluation::SetEvaluationSize },
	{ "SetEvaluationCubeSize", (void*)Evaluation::SetEvaluationCubeSize },
	{ "CubemapFilter", (void*)Evaluation::CubemapFilter},
	{ "SetProcessing", (void*)Evaluation::SetProcessing},
	{ "Job", (void*)Evaluation::Job },
	{ "JobMain", (void*)Evaluation::JobMain },
	{ "memmove", memmove },
	{ "strcpy", strcpy },
	{ "strlen", strlen },
};

typedef int(*jobFunction)(void*);

struct CFunctionTaskSet : enki::ITaskSet
{
	CFunctionTaskSet(jobFunction function, void *ptr, unsigned int size) : enki::ITaskSet()
		, mFunction(function)
		, mBuffer(malloc(size))
	{
		memcpy(mBuffer, ptr, size);
	}
	virtual void    ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum)
	{
		mFunction(mBuffer);
		free(mBuffer);
		delete this;
	}
	jobFunction mFunction;
	void *mBuffer;
};

struct CFunctionMainTask : enki::IPinnedTask
{
	CFunctionMainTask(jobFunction function, void *ptr, unsigned int size)
		: enki::IPinnedTask(0) // set pinned thread to 0
		, mFunction(function)
		, mBuffer(malloc(size))
	{
		memcpy(mBuffer, ptr, size);
	}
	virtual void Execute()
	{
		mFunction(mBuffer);
		free(mBuffer);
		delete this;
	}
	jobFunction mFunction;
	void *mBuffer;
};

void Evaluation::SetProcessing(int target, int processing)
{
	gEvaluation.mEvaluationStages[target].mbProcessing = processing != 0;
}

int Evaluation::Job(int(*jobFunction)(void*), void *ptr, unsigned int size)
{
	g_TS.AddTaskSetToPipe(new CFunctionTaskSet(jobFunction, ptr, size));
	return EVAL_OK;
}

int Evaluation::JobMain(int(*jobMainFunction)(void*), void *ptr, unsigned int size)
{
	g_TS.AddPinnedTask(new CFunctionMainTask(jobMainFunction, ptr, size));
	return EVAL_OK;
}

void Evaluation::SetBlendingMode(int target, int blendSrc, int blendDst)
{
	EvaluationStage& evaluation = gEvaluation.mEvaluationStages[target];

	evaluation.mBlendingSrc = blendSrc;
	evaluation.mBlendingDst = blendDst;
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to)
{
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}

void Evaluation::ClearEvaluators()
{
	// clear
	for (auto& program : mEvaluatorPerNodeType)
	{
		if (program.mGLSLProgram)
			glDeleteProgram(program.mGLSLProgram);
		if (program.mMem)
			free(program.mMem);
	}
}

void Evaluation::SetEvaluators(const std::vector<EvaluatorFile>& evaluatorfilenames)
{
	ClearEvaluators();

	mEvaluatorPerNodeType.clear();
	mEvaluatorPerNodeType.resize(evaluatorfilenames.size(), Evaluator());

	// GLSL
	for (auto& file : evaluatorfilenames)
	{
		if (file.mEvaluatorType != EVALUATOR_GLSL)
			continue;
		const std::string filename = file.mFilename;

		std::ifstream t(file.mDirectory + filename);
		if (t.good())
		{
			std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
			if (mEvaluatorScripts.find(filename) == mEvaluatorScripts.end())
				mEvaluatorScripts[filename] = EvaluatorScript(str);
			else
				mEvaluatorScripts[filename].mText = str;
		}
	}

	std::string baseShader = mEvaluatorScripts["Shader.glsl"].mText;
	for (auto& file : evaluatorfilenames)
	{
		if (file.mEvaluatorType != EVALUATOR_GLSL)
			continue;
		const std::string filename = file.mFilename;

		if (filename == "Shader.glsl")
			continue;

		EvaluatorScript& shader = mEvaluatorScripts[filename];
		std::string shaderText = ReplaceAll(baseShader, "__NODE__", shader.mText);
		std::string nodeName = ReplaceAll(filename, ".glsl", "");
		shaderText = ReplaceAll(shaderText, "__FUNCTION__", nodeName + "()");

		unsigned int program = LoadShader(shaderText, filename.c_str());

		int parameterBlockIndex = glGetUniformBlockIndex(program, (nodeName + "Block").c_str());
		if (parameterBlockIndex != -1)
			glUniformBlockBinding(program, parameterBlockIndex, 1);

		parameterBlockIndex = glGetUniformBlockIndex(program, "EvaluationBlock");
		if (parameterBlockIndex != -1)
			glUniformBlockBinding(program, parameterBlockIndex, 2);
		shader.mProgram = program;
		if (shader.mNodeType != -1)
			mEvaluatorPerNodeType[shader.mNodeType].mGLSLProgram = program;
	}

	if (!gEvaluation.mEvaluationStateGLSLBuffer)
	{
		glGenBuffers(1, &mEvaluationStateGLSLBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, mEvaluationStateGLSLBuffer);

		glBufferData(GL_UNIFORM_BUFFER, sizeof(EvaluationInfo), NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 2, mEvaluationStateGLSLBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	// C
	for (auto& file : evaluatorfilenames)
	{
		if (file.mEvaluatorType != EVALUATOR_C)
			continue;
		const std::string filename = file.mFilename;
		try
		{
			std::ifstream t(file.mDirectory + filename);
			if (!t.good())
			{
				Log("%s - Unable to load file.\n", filename.c_str());
				continue;
			}
			std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
			if (mEvaluatorScripts.find(filename) == mEvaluatorScripts.end())
				mEvaluatorScripts[filename] = EvaluatorScript(str);
			else
				mEvaluatorScripts[filename].mText = str;

			EvaluatorScript& program = mEvaluatorScripts[filename];
			TCCState *s = tcc_new();

			int *noLib = (int*)s;
			noLib[2] = 1; // no stdlib

			tcc_set_error_func(s, 0, libtccErrorFunc);
			tcc_add_include_path(s, "C\\");
			tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

			if (tcc_compile_string(s, program.mText.c_str()) != 0)
			{
				Log("%s - Compilation error!\n", filename.c_str());
				continue;
			}

			for (auto& evaluationFunction : evaluationFunctions)
				tcc_add_symbol(s, evaluationFunction.szFunctionName, evaluationFunction.function);

			int size = tcc_relocate(s, NULL);
			if (size == -1)
			{
				Log("%s - Libtcc unable to relocate program!\n", filename.c_str());
				continue;
			}
			program.mMem = malloc(size);
			tcc_relocate(s, program.mMem);

			*(void**)(&program.mCFunction) = tcc_get_symbol(s, "main");
			if (!program.mCFunction)
			{
				Log("%s - No main function!\n", filename.c_str());
			}
			tcc_delete(s);

			if (program.mNodeType != -1)
			{
				mEvaluatorPerNodeType[program.mNodeType].mCFunction = program.mCFunction;
				mEvaluatorPerNodeType[program.mNodeType].mMem = program.mMem;
			}
		}
		catch (...)
		{
			Log("Error at compiling %s", filename.c_str());
		}
	}
}

void Evaluation::BindGLSLParameters(EvaluationStage& stage)
{
	if (!stage.mParametersBuffer)
	{
		glGenBuffers(1, &stage.mParametersBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, stage.mParametersBuffer);

		glBufferData(GL_UNIFORM_BUFFER, stage.mParametersSize, stage.mParameters, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, stage.mParametersBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
	else
	{
		glBindBuffer(GL_UNIFORM_BUFFER, stage.mParametersBuffer);
		glBufferData(GL_UNIFORM_BUFFER, stage.mParametersSize, stage.mParameters, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}

void Evaluation::SetMouseInfos(EvaluationInfo &evaluationInfo, EvaluationStage &evaluationStage) const
{
	evaluationInfo.mouse[0] = evaluationStage.mRx;
	evaluationInfo.mouse[1] = evaluationStage.mRy;
	evaluationInfo.mouse[2] = evaluationStage.mLButDown ? 1.f : 0.f;
	evaluationInfo.mouse[3] = evaluationStage.mRButDown ? 1.f : 0.f;
}
void Evaluation::EvaluateGLSL(EvaluationStage& evaluationStage, EvaluationInfo& evaluationInfo)
{
	const Input& input = evaluationStage.mInput;

	RenderTarget* tgt = evaluationStage.mTarget;
	if (!evaluationInfo.uiPass)
	{
		if (tgt->mImage.mNumFaces == 6)
			tgt->BindAsCubeTarget();
		else
			tgt->BindAsTarget();
	}
	unsigned int program = mEvaluatorPerNodeType[evaluationStage.mNodeType].mGLSLProgram;
	const int blendOps[] = { evaluationStage.mBlendingSrc, evaluationStage.mBlendingDst };
	unsigned int blend[] = { GL_ONE, GL_ZERO };


	for (int i = 0; i < 2; i++)
	{
		if (blendOps[i] < BLEND_LAST)
			blend[i] = GLBlends[blendOps[i]];
	}

	evaluationInfo.targetIndex = 0;
	memcpy(evaluationInfo.inputIndices, input.mInputs, sizeof(evaluationInfo.inputIndices));
	evaluationInfo.forcedDirty = evaluationStage.mbForceEval ? 1 : 0;
	SetMouseInfos(evaluationInfo, evaluationStage);
	//evaluationInfo.uiPass = 1;

	glEnable(GL_BLEND);
	glBlendFunc(blend[0], blend[1]);

	glUseProgram(program);

	size_t faceCount = evaluationInfo.uiPass ? 1 : tgt->mImage.mNumFaces;
	for (size_t face = 0; face < faceCount; face++)
	{
		if (tgt->mImage.mNumFaces == 6)
			tgt->BindCubeFace(face);

		memcpy(evaluationInfo.viewRot, rotMatrices[face], sizeof(float) * 16);
		glBindBuffer(GL_UNIFORM_BUFFER, mEvaluationStateGLSLBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(EvaluationInfo), &evaluationInfo, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);


		glBindBufferBase(GL_UNIFORM_BUFFER, 1, evaluationStage.mParametersBuffer);
		glBindBufferBase(GL_UNIFORM_BUFFER, 2, mEvaluationStateGLSLBuffer);

		int samplerIndex = 0;
		for (size_t inputIndex = 0; inputIndex < sizeof(samplerName)/sizeof(const char*); inputIndex++)
		{
			unsigned int parameter = glGetUniformLocation(program, samplerName[inputIndex]);
			if (parameter == 0xFFFFFFFF)
				continue;
			glUniform1i(parameter, samplerIndex);
			glActiveTexture(GL_TEXTURE0 + samplerIndex);
			
			int targetIndex = input.mInputs[samplerIndex];
			if (targetIndex < 0)
			{
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			else
			{
				auto* tgt = mEvaluationStages[targetIndex].mTarget;
				if (tgt)
				{
					const InputSampler& inputSampler = evaluationStage.mInputSamplers[samplerIndex];
					if (tgt->mImage.mNumFaces == 1)
					{
						glBindTexture(GL_TEXTURE_2D, mEvaluationStages[targetIndex].mTarget->mGLTexID);
						TexParam(filter[inputSampler.mFilterMin], filter[inputSampler.mFilterMag], wrap[inputSampler.mWrapU], wrap[inputSampler.mWrapV], GL_TEXTURE_2D);
					}
					else
					{
						glBindTexture(GL_TEXTURE_CUBE_MAP, mEvaluationStages[targetIndex].mTarget->mGLTexID);
						TexParam(filter[inputSampler.mFilterMin], filter[inputSampler.mFilterMag], wrap[inputSampler.mWrapU], wrap[inputSampler.mWrapV], GL_TEXTURE_CUBE_MAP);
					}
				}
			}
			samplerIndex++;
		}
		//
		mFSQuad.Render();
	}
	glDisable(GL_BLEND);
}

void Evaluation::EvaluateC(EvaluationStage& evaluationStage, size_t index, EvaluationInfo& evaluationInfo)
{
	const Input& input = evaluationStage.mInput;

	//EvaluationInfo evaluationInfo;
	evaluationInfo.targetIndex = int(index);
	memcpy(evaluationInfo.inputIndices, input.mInputs, sizeof(evaluationInfo.inputIndices));
	SetMouseInfos(evaluationInfo, evaluationStage);
	//evaluationInfo.forcedDirty = evaluation.mbForceEval ? 1 : 0;
	//evaluationInfo.uiPass = false;
	try // todo: find a better solution than a try catch
	{
		mEvaluatorPerNodeType[evaluationStage.mNodeType].mCFunction(evaluationStage.mParameters, &evaluationInfo);
	}
	catch (...)
	{

	}
}

void Evaluation::EvaluationStage::Clear()
{
	if (mEvaluationMask&EvaluationGLSL)
		glDeleteBuffers(1, &mParametersBuffer);
	//gEvaluation.UnreferenceRenderTarget(&mTarget);
}

void Evaluation::FinishEvaluation()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
}

unsigned int Evaluation::UploadImage(Image *image, unsigned int textureId, int cubeFace)
{
	if (!textureId)
		glGenTextures(1, &textureId);

	unsigned int targetType = (cubeFace == -1) ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
	glBindTexture(targetType, textureId);

	unsigned int inputFormat = glInputFormats[image->mFormat];
	unsigned int internalFormat = glInternalFormats[image->mFormat];
	glTexImage2D((cubeFace==-1)? GL_TEXTURE_2D: glCubeFace[cubeFace], 0, internalFormat, image->mWidth, image->mHeight, 0, inputFormat, GL_UNSIGNED_BYTE, image->mBits);
	TexParam(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, targetType);

	glBindTexture(targetType, 0);
	return textureId;
}

unsigned int Evaluation::GetTexture(const std::string& filename)
{
	auto iter = mSynchronousTextureCache.find(filename);
	if (iter != mSynchronousTextureCache.end())
		return iter->second;

	Image image;
	unsigned int textureId = 0;
	if (ReadImage(filename.c_str(), &image) == EVAL_OK)
	{
		textureId = UploadImage(&image, 0);
	}

	mSynchronousTextureCache[filename] = textureId;
	return textureId;
}

void Evaluation::NodeUICallBack(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
	// Backup GL state
	GLenum last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
	glActiveTexture(GL_TEXTURE0);
	GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
	GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
#ifdef GL_SAMPLER_BINDING
	GLint last_sampler; glGetIntegerv(GL_SAMPLER_BINDING, &last_sampler);
#endif
	GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
#ifdef GL_POLYGON_MODE
	GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
#endif
	GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
	GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
	GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
	GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
	GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
	GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
	GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
	GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
	GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
	GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
	GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
	GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
	ImGuiIO& io = ImGui::GetIO();

	if (!mCallbackRects.empty())
	{
		const ImogenDrawCallback& cb = mCallbackRects[intptr_t(cmd->UserCallbackData)];

		ImRect cbRect = cb.mOrginalRect;
		float h = cbRect.Max.y - cbRect.Min.y;
		float w = cbRect.Max.x - cbRect.Min.x;
		glViewport(int(cbRect.Min.x), int(io.DisplaySize.y - cbRect.Max.y), int(w), int(h));

		cbRect.Min.x = ImMax(cbRect.Min.x, cmd->ClipRect.x);
		ImRect clippedRect = cb.mClippedRect;
		glScissor(int(clippedRect.Min.x), int(io.DisplaySize.y - clippedRect.Max.y), int(clippedRect.Max.x - clippedRect.Min.x), int(clippedRect.Max.y - clippedRect.Min.y));
		glEnable(GL_SCISSOR_TEST);

		switch (cb.mType)
		{
		case CBUI_Node:
		{
			EvaluationInfo evaluationInfo;
			evaluationInfo.forcedDirty = 1;
			evaluationInfo.uiPass = 1;
			gEvaluation.PerformEvaluationForNode(cb.mNodeIndex, int(w), int(h), true, evaluationInfo);
		}
		break;
		case CBUI_Progress:
		{
			glUseProgram(gEvaluation.mProgressShader);
			glUniform1f(glGetUniformLocation(gEvaluation.mProgressShader, "time"), float(double(SDL_GetTicks())/1000.0));
			mFSQuad.Render();
		}
		break;
		case CBUI_Cubemap:
		{
			glUseProgram(gEvaluation.mDisplayCubemapShader);
			int tgt = glGetUniformLocation(gEvaluation.mDisplayCubemapShader, "samplerCubemap");
			glUniform1i(tgt, 0);
			glActiveTexture(GL_TEXTURE0);

			glBindTexture(GL_TEXTURE_CUBE_MAP, gEvaluation.GetEvaluationTexture(cb.mNodeIndex));
			mFSQuad.Render();
		}
		break;
		}
	}
	// Restore modified GL state
	glUseProgram(last_program);
	glBindTexture(GL_TEXTURE_2D, last_texture);
#ifdef GL_SAMPLER_BINDING
	glBindSampler(0, last_sampler);
#endif
	glActiveTexture(last_active_texture);
	glBindVertexArray(last_vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
	glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
	if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
	if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
	if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
	if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
#ifdef GL_POLYGON_MODE
	glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
#endif
	glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
	glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int Evaluation::GetEvaluationSize(int target, int *imageWidth, int *imageHeight)
{
	if (target < 0 || target >= gEvaluation.mEvaluationStages.size())
		return EVAL_ERR;
	RenderTarget* renderTarget = gEvaluation.mEvaluationStages[target].mTarget;
	if (!renderTarget)
		return EVAL_ERR;
	*imageWidth = renderTarget->mImage.mWidth;
	*imageHeight = renderTarget->mImage.mHeight;
	return EVAL_OK;
}

int Evaluation::SetEvaluationSize(int target, int imageWidth, int imageHeight)
{
	if (target < 0 || target >= gEvaluation.mEvaluationStages.size())
		return EVAL_ERR;
	auto& stage = gEvaluation.mEvaluationStages[target];
	RenderTarget* renderTarget = stage.mTarget;
	if (!renderTarget)
		return EVAL_ERR;
	stage.mbFreeSizing = false;
	renderTarget->InitBuffer(imageWidth, imageHeight);
	return EVAL_OK;
}

int Evaluation::SetEvaluationCubeSize(int target, int faceWidth)
{
	if (target < 0 || target >= gEvaluation.mEvaluationStages.size())
		return EVAL_ERR;
	auto& stage = gEvaluation.mEvaluationStages[target];
	RenderTarget* renderTarget = stage.mTarget;
	if (!renderTarget)
		return EVAL_ERR;
	stage.mbFreeSizing = false;
	renderTarget->InitCube(faceWidth);
	return EVAL_OK;
}