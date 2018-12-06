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
#include <string.h>
#include <stdio.h>
#include "ffmpegCodec.h"
#include <memory>
#include "Utils.h"


struct ImDrawList;
struct ImDrawCmd;


enum BlendOp
{
	ZERO,
	ONE,
	SRC_COLOR,
	ONE_MINUS_SRC_COLOR,
	DST_COLOR,
	ONE_MINUS_DST_COLOR,
	SRC_ALPHA,
	ONE_MINUS_SRC_ALPHA,
	DST_ALPHA,
	ONE_MINUS_DST_ALPHA,
	CONSTANT_COLOR,
	ONE_MINUS_CONSTANT_COLOR,
	CONSTANT_ALPHA,
	ONE_MINUS_CONSTANT_ALPHA,
	SRC_ALPHA_SATURATE,
	BLEND_LAST
};

enum EvaluationStatus
{
	EVAL_OK,
	EVAL_ERR,
};

struct EvaluationInfo
{
	float viewRot[16];

	int targetIndex;
	int forcedDirty;
	int uiPass;
	int padding;
	float mouse[4];
	int inputIndices[8];
	float pad2[4];
	
	float viewport[2];
	size_t mFrame;
	size_t mLocalFrame;
};

struct TextureFormat
{
	enum Enum
	{
		BGR8,
		RGB8,
		RGB16,
		RGB16F,
		RGB32F,
		RGBE,

		BGRA8,
		RGBA8,
		RGBA16,
		RGBA16F,
		RGBA32F,

		RGBM,

		Count,
		Null = -1,
	};
};

typedef struct Image_t
{
	Image_t() : mDecoder(NULL), mBits(NULL){}
	unsigned char *mBits;
	void *mDecoder;
	int mWidth, mHeight;
	uint32_t mDataSize;
	uint8_t mNumMips;
	uint8_t mNumFaces;
	uint8_t mFormat;
} Image;

class RenderTarget
{

public:
	RenderTarget() : mGLTexID(0), mFbo(0)
	{
		memset(&mImage, 0, sizeof(Image_t));
	}

	void InitBuffer(int width, int height);
	void InitCube(int width);
	void BindAsTarget() const;
	void BindAsCubeTarget() const;
	void BindCubeFace(size_t face);
	void Destroy();
	void CheckFBO();


	Image_t mImage;
	unsigned int mGLTexID;
	TextureID mFbo;
};

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
#ifdef _DEBUG
	std::string mNodeTypename;
#endif
	std::shared_ptr<FFMPEGCodec::Decoder> mDecoder;
	size_t mNodeType;
	unsigned int mParametersBuffer;
	std::vector<unsigned char> mParameters;
	Input mInput;
	std::vector<InputSampler> mInputSamplers;
	int gEvaluationMask; // see EvaluationMask
	int mUseCountByOthers;
	int mBlendingSrc;
	int mBlendingDst;
	int mLocalTime;
	// mouse
	float mRx;
	float mRy;
	bool mLButDown;
	bool mRButDown;
	void Clear();
	Image_t DecodeImage();
};

enum EvaluationMask
{
	EvaluationC = 1 << 0,
	EvaluationGLSL = 1 << 1,
	EvaluationPython = 1 << 2,
};

// simple API
struct Evaluation
{
	Evaluation();

	void Init();
	void Finish();


	void AddSingleEvaluation(size_t nodeType);
	void UserAddEvaluation(size_t nodeType);
	void UserDeleteEvaluation(size_t target);

	//
	size_t GetStagesCount() const { return mStages.size(); }
	size_t GetStageType(size_t target) const { return mStages[target].mNodeType; }
	size_t GetEvaluationImageDuration(size_t target);
	
	void SetEvaluationParameters(size_t target, const std::vector<unsigned char>& parameters);
	void SetEvaluationSampler(size_t target, const std::vector<InputSampler>& inputSamplers);
	void AddEvaluationInput(size_t target, int slot, int source);
	void DelEvaluationInput(size_t target, int slot);
	void SetEvaluationOrder(const std::vector<size_t> nodeOrderList);
	void SetMouse(int target, float rx, float ry, bool lButDown, bool rButDown);
	void Clear();
	
	void SetStageLocalTime(size_t target, int localTime, bool updateDecoder);

	// API
	static int ReadImage(const char *filename, Image *image);
	static int ReadImageMem(unsigned char *data, size_t dataSize, Image *image);
	static int WriteImage(const char *filename, Image *image, int format, int quality);
	static int GetEvaluationImage(int target, Image *image);
	static int SetEvaluationImage(int target, Image *image);
	static int SetEvaluationImageCube(int target, Image *image, int cubeFace);
	static int SetThumbnailImage(Image *image);
	static int AllocateImage(Image *image);
	static int FreeImage(Image *image);
	static unsigned int UploadImage(Image *image, unsigned int textureId, int cubeFace = -1);
	static int Evaluate(int target, int width, int height, Image *image);
	static void SetBlendingMode(int target, int blendSrc, int blendDst);
	static int EncodePng(Image *image, std::vector<unsigned char> &pngImage);
	static int SetNodeImage(int target, Image *image);
	static int GetEvaluationSize(int target, int *imageWidth, int *imageHeight);
	static int SetEvaluationSize(int target, int imageWidth, int imageHeight);
	static int SetEvaluationCubeSize(int target, int faceWidth);
	static int CubemapFilter(Image *image, int faceSize, int lightingModel, int excludeBase, int glossScale, int glossBias);
	static int Job(int(*jobFunction)(void*), void *ptr, unsigned int size);
	static int JobMain(int(*jobMainFunction)(void*), void *ptr, unsigned int size);
	static void SetProcessing(int target, int processing);

	static void NodeUICallBack(const ImDrawList* parent_list, const ImDrawCmd* cmd);
	// synchronous texture cache
	// use for simple textures(stock) or to replace with a more efficient one
	unsigned int GetTexture(const std::string& filename);


	const std::vector<size_t>& GetForwardEvaluationOrder() const { return mEvaluationOrderList; }

	
	const EvaluationStage& GetEvaluationStage(size_t index) const {	return mStages[index]; }


	// animation
	const std::vector<AnimTrack>& GetAnimTrack() const { return mAnimTrack; }
	void SetAnimTrack(const std::vector<AnimTrack>& animTrack) { mAnimTrack = animTrack; }

	void MakeKey(int frame, uint32_t nodeIndex, uint32_t parameterIndex);
	void GetKeyedParameters(int frame, uint32_t nodeIndex, std::vector<bool>& keyed);
	void ApplyAnimation(int frame);
	void RemoveAnimation(int nodeIndex);

	// error shader
	unsigned int mNodeErrorShader;
protected:
	void APIInit();
	std::map<std::string, unsigned int> mSynchronousTextureCache;

	std::vector<EvaluationStage> mStages;
	std::vector<AnimTrack> mAnimTrack;
	std::vector<size_t> mEvaluationOrderList;
	void BindGLSLParameters(EvaluationStage& evaluationStage);

	// ui callback shaders
	unsigned int mProgressShader;
	unsigned int mDisplayCubemapShader;


	// ffmpeg encoders
	FFMPEGCodec::Decoder* FindDecoder(const std::string& filename);

	static void StageIsAdded(int index);
	static void StageIsDeleted(int index);

	AnimTrack* GetAnimTrack(uint32_t nodeIndex, uint32_t parameterIndex);
};

extern Evaluation gEvaluation;
extern FullScreenTriangle gFSQuad;