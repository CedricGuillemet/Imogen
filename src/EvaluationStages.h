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
#include "Bitmap.h"

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


struct EvaluationInfo
{
    float viewRot[16];
    float viewProjection[16];
    float viewInverse[16];

    int targetIndex;
    int forcedDirty;
    int uiPass;
    int passNumber;
    float mouse[4];
    int inputIndices[8];
    float pad2[4];
    
    float viewport[2];
    int mFrame;
    int mLocalFrame;
};


class RenderTarget
{

public:
    RenderTarget() : mGLTexID(0), mGLTexDepth(0), mFbo(0), mDepthBuffer(0)
    {
        memset(&mImage, 0, sizeof(Image));
    }

    void InitBuffer(int width, int height, bool depthBuffer);
    void InitCube(int width);
    void BindAsTarget() const;
    void BindAsCubeTarget() const;
    void BindCubeFace(size_t face);
    void Destroy();
    void CheckFBO();
    void Clone(const RenderTarget &other);
    void Swap(RenderTarget &other);


    Image mImage;
    unsigned int mGLTexID;
    unsigned int mGLTexDepth;
    TextureID mDepthBuffer;
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
    bool mbDepthBuffer;
    // mouse
    float mRx;
    float mRy;
    bool mLButDown;
    bool mRButDown;
    void Clear();
    // scene render
    void *scene;
    void *renderer;
    Image DecodeImage();
};

enum EvaluationMask
{
    EvaluationC = 1 << 0,
    EvaluationGLSL = 1 << 1,
    EvaluationPython = 1 << 2,
    EvaluationGLSLCompute = 1 << 3,
};

// simple API
struct EvaluationStages
{
    EvaluationStages();

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
    static int GetEvaluationImage(int target, Image *image);
    static int SetEvaluationImage(int target, Image *image);
    static int SetEvaluationImageCube(int target, Image *image, int cubeFace);
    static int SetThumbnailImage(Image *image);
    static int AllocateImage(Image *image);

    static int Evaluate(int target, int width, int height, Image *image);
    static void SetBlendingMode(int target, int blendSrc, int blendDst);
    static void EnableDepthBuffer(int target, int enable);
    static int SetNodeImage(int target, Image *image);
    static int GetEvaluationSize(int target, int *imageWidth, int *imageHeight);
    static int SetEvaluationSize(int target, int imageWidth, int imageHeight);
    static int SetEvaluationCubeSize(int target, int faceWidth);
    static int CubemapFilter(Image *image, int faceSize, int lightingModel, int excludeBase, int glossScale, int glossBias);
    static int Job(int(*jobFunction)(void*), void *ptr, unsigned int size);
    static int JobMain(int(*jobMainFunction)(void*), void *ptr, unsigned int size);
    static void SetProcessing(int target, int processing);
    static int AllocateComputeBuffer(int target, int elementCount, int elementSize);

    static int LoadScene(const char *filename, void **scene);
    static int SetEvaluationScene(int target, void *scene);
    static int GetEvaluationScene(int target, void **scene);
    static int GetEvaluationRenderer(int target, void **renderer);
    static int InitRenderer(int target, int mode, void *scene);
    static int UpdateRenderer(int target);

    static void DrawUICubemap(size_t nodeIndex);
    static void DrawUISingle(size_t nodeIndex);
    static void DrawUIProgress(size_t nodeIndex);



    const std::vector<size_t>& GetForwardEvaluationOrder() const { return mEvaluationOrderList; }

    
    const EvaluationStage& GetEvaluationStage(size_t index) const {    return mStages[index]; }

    const std::vector<AnimTrack>& GetAnimTrack() const { return mAnimTrack; }


    // Data
    std::vector<AnimTrack> mAnimTrack;
    std::vector<EvaluationStage> mStages;
    std::vector<size_t> mEvaluationOrderList;
    std::vector<uint32_t> mPinnedParameters;
    int mFrameMin, mFrameMax;

protected:
    void BindGLSLParameters(EvaluationStage& evaluationStage);



    // ffmpeg encoders
    FFMPEGCodec::Decoder* FindDecoder(const std::string& filename);

    static void StageIsAdded(int index);
    static void StageIsDeleted(int index);

};

extern FullScreenTriangle gFSQuad;