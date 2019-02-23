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
struct EvaluationContext;

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
    std::string mTypename;
#endif
    std::shared_ptr<FFMPEGCodec::Decoder> mDecoder;
    size_t mType;
    unsigned int mRuntimeUniqueId;
    unsigned int mParametersBuffer;
    std::vector<unsigned char> mParameters;
    Input mInput;
    std::vector<InputSampler> mInputSamplers;
    int gEvaluationMask; // see EvaluationMask
    int mUseCountByOthers;
    int mBlendingSrc;
    int mBlendingDst;
    int mLocalTime;
    int mStartFrame, mEndFrame;
    bool mbDepthBuffer;
    // Camera
    Mat4x4 mParameterViewMatrix = Mat4x4::GetIdentity();
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

    bool operator != (const EvaluationStage& other) const
    {
        if (mType != other.mType)
            return true;
        if (mParameters != other.mParameters)
            return true;
        if (mRuntimeUniqueId != other.mRuntimeUniqueId)
            return true;
        if (mStartFrame != other.mStartFrame)
            return true;
        if (mEndFrame != other.mEndFrame)
            return true;
        if (mInputSamplers.size() != other.mInputSamplers.size())
            return true;
        /*
        for (size_t i = 0; i < mInputSamplers.size(); i++)
        {
            if (mInputSamplers[i] != other.mInputSamplers[i])
                return true;
        }
        */
        if (mParameterViewMatrix != other.mParameterViewMatrix)
            return true;
        return false;
    }
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
    size_t GetStageType(size_t target) const { return mStages[target].mType; }
    size_t GetEvaluationImageDuration(size_t target);
    
    void SetEvaluationParameters(size_t target, const std::vector<unsigned char>& parameters);
    void SetEvaluationSampler(size_t target, const std::vector<InputSampler>& inputSamplers);
    void AddEvaluationInput(size_t target, int slot, int source);
    void DelEvaluationInput(size_t target, int slot);
    void SetEvaluationOrder(const std::vector<size_t> nodeOrderList);
    void SetMouse(int target, float rx, float ry, bool lButDown, bool rButDown);
    void Clear();
    
    void SetStageLocalTime(EvaluationContext *evaluationContext, size_t target, int localTime, bool updateDecoder);

    



    const std::vector<size_t>& GetForwardEvaluationOrder() const { return mEvaluationOrderList; }

    
    const EvaluationStage& GetEvaluationStage(size_t index) const {    return mStages[index]; }

    const std::vector<AnimTrack>& GetAnimTrack() const { return mAnimTrack; }
    Camera *GetCameraParameter(size_t index);
    int GetIntParameter(size_t index, const char *parameterName, int defaultValue);
    Mat4x4* GetParameterViewMatrix(size_t index) { if (index >= mStages.size()) return NULL; return &mStages[index].mParameterViewMatrix; }
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

    void StageIsAdded(int index);
    void StageIsDeleted(int index);

};

extern FullScreenTriangle gFSQuad;