#include "ImApp.h"
#include "Evaluation.h"
#include <vector>
#include <algorithm>

extern int Log(const char *szFormat, ...);
static const int SemUV0 = 0;

void RenderTarget::bindAsTarget() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, mWidth, mHeight);
}

void RenderTarget::destroy()
{
	if (mGLTexID)
		glDeleteTextures(1, &mGLTexID);
	if (fbo)
		glDeleteFramebuffers(1, &fbo);
	if (depthbuffer)
		glDeleteRenderbuffers(1, &depthbuffer);
	fbo = depthbuffer = 0;
	mWidth = mHeight = 0;
	mGLTexID = 0;
}

void RenderTarget::clear()
{
	glClear(GL_COLOR_BUFFER_BIT | (depthbuffer ? GL_DEPTH_BUFFER_BIT : 0));
}

void RenderTarget::initBuffer(int width, int height, bool hasZBuffer)
{
	if ((width == mWidth) && (mHeight == height) && (!(hasZBuffer ^ (depthbuffer != 0))))
		return;
	destroy();

	mWidth = width;
	mHeight = height;

	//LOG("New Target %dx%d\n", width, height);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// diffuse
	glGenTextures(1, &mGLTexID);
	glBindTexture(GL_TEXTURE_2D, mGLTexID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	TexParam(GL_NEAREST, GL_NEAREST, GL_CLAMP, GL_CLAMP, GL_TEXTURE_2D);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mGLTexID, 0);
	/*
	if (hasZBuffer)
	{
		// Z
		glGenTextures(1, &mGLTexID2);
		glBindTexture(GL_TEXTURE_2D, mGLTexID2);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		TexParam(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_2D);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mGLTexID2, 0);
	}
	*/
	static const GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(sizeof(DrawBuffers) / sizeof(GLenum), DrawBuffers);

	checkFBO();
}

void RenderTarget::checkFBO()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

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
	const char *shaderTypeStrings[] = { "\n#version 330 core\n#define VERTEX_SHADER\n", "\n#version 330 core\n#define FRAGMENT_SHADER\n" };
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
		stringLength[0] = strlen(shaderTypeStrings[i]);
		strings[stringsCount - 1] = shaderString.c_str();
		stringLength[stringsCount - 1] = shaderString.length();

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

	//glBindAttribLocation(programObject, SemPosition, "inPosition");
	//glBindAttribLocation(programObject, SemNormal, "inNormal");
	glBindAttribLocation(programObject, SemUV0, "inUV");

	//glBindAttribLocation(programObject, SemInstanceMatrix, "inInstanceMatrix");
	//glBindAttribLocation(programObject, SemInstanceColor, "inInstanceColor");


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

inline std::string ReadFile(const char *szFileName, int &bufSize)
{
	FILE *fp = fopen(szFileName, "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		bufSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char *buf = new char[bufSize];
		fread(buf, bufSize, 1, fp);
		fclose(fp);
		std::string res(buf, bufSize);
		return res;
	}
	return NULL;
}

struct Input
{
	Input()
	{
		memset(mInputs, -1, sizeof(int) * 8);
	}
	int mInputs[8];
};

struct Evaluation
{
	RenderTarget mTarget;
	unsigned int mShader;
	Input mInput;
};

std::vector<Evaluation> mEvaluations;
std::vector<int> mEvaluationOrderList;
std::string mBaseShader;
FullScreenTriangle mFSQuad;

void InitEvaluation()
{
	mFSQuad.Init();

	int bufSize;
	mBaseShader = ReadFile("Shader.glsl", bufSize);
}

unsigned int AddEvaluationTarget()
{
	Evaluation evaluation;
	evaluation.mTarget.initBuffer(256, 256, false);
	mEvaluations.push_back(evaluation);
	return mEvaluations.size() - 1;
}

void DelEvaluationTarget(int target)
{
	Evaluation& ev = mEvaluations[target];
	glDeleteProgram(ev.mShader);
	ev.mTarget.destroy();
	mEvaluations.erase(mEvaluations.begin() + target);

	// shift all connections
	for (auto& evaluation : mEvaluations)
	{
		for (auto& inp : evaluation.mInput.mInputs)
		{
			if (inp >= target)
				inp--;
		}
	}
}

unsigned int GetEvaluationTexture(int target)
{
	return mEvaluations[target].mTarget.mGLTexID;
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

void SetEvaluationCall(int target, const std::string& shaderCall)
{
	std::string shd = mBaseShader;
	shd = ReplaceAll(shd, std::string("__FUNCTION__"), shaderCall);
	unsigned int& program = mEvaluations[target].mShader;
	if (program)
		glDeleteProgram(program);
	program = LoadShader(shd, "shd");
}

void RunEvaluation()
{
	if (mEvaluationOrderList.empty())
		return;

	static const char* samplerName[] = { "Sampler0", "Sampler1", "Sampler2", "Sampler3", "Sampler4", "Sampler5", "Sampler6", "Sampler7" };
	for (size_t i = 0; i < mEvaluationOrderList.size(); i++)
	{
		const Evaluation& evaluation = mEvaluations[mEvaluationOrderList[i]];

		const Input& input = evaluation.mInput;
		const RenderTarget &tg = evaluation.mTarget;
		tg.bindAsTarget();
		glUseProgram(evaluation.mShader);
		for (size_t inputIndex = 0; inputIndex < 8; inputIndex++)
		{
			unsigned int parameter = glGetUniformLocation(evaluation.mShader, samplerName[inputIndex]);
			if (parameter == 0xFFFFFFFF)
				continue;
			glUniform1i(parameter, inputIndex);
			glActiveTexture(GL_TEXTURE0 + inputIndex);

			int targetIndex = input.mInputs[inputIndex];
			if (targetIndex == -1)
			{
				glBindTexture(GL_TEXTURE_2D, 0);
				continue;
			}

			glBindTexture(GL_TEXTURE_2D, mEvaluations[targetIndex].mTarget.mGLTexID);
			TexParam(GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT, GL_TEXTURE_2D);


		}
		mFSQuad.Render();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
}

void AddEvaluationInput(int target, int slot, int source)
{
	mEvaluations[target].mInput.mInputs[slot] = source;
}

void DelEvaluationInput(int target, int slot)
{
	mEvaluations[target].mInput.mInputs[slot] = -1;
}
void SetEvaluationOrder(const std::vector<int> nodeOrderList)
{
	mEvaluationOrderList = nodeOrderList;
}

void SetTargetDirty(int target)
{

}