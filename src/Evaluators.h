// https://github.com/CedricGuillemet/Imogen
//
// The MIT License(MIT)
//
// Copyright(c) 2019 Cedric Guillemet
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

#include <vector>
#include <map>
#include <string>
#include "Imogen.h"
#if USE_PYTHON
#include "pybind11/embed.h"
#endif

struct EvaluationContext;

enum EvaluationMask
{
    EvaluationC = 1 << 0,
    EvaluationGLSL = 1 << 1,
    EvaluationPython = 1 << 2,
    EvaluationGLSLCompute = 1 << 3
};

struct EvaluationInfo
{
    float viewRot[16];
    float viewProjection[16];
    float viewInverse[16];
    float model[16];
    float modelViewProjection[16];
    float viewport[4];

    int targetIndex;
    int forcedDirty;
    int uiPass;
    int passNumber;
    float mouse[4];
    int keyModifier[4];
    int inputIndices[8];

    int frame;
    int localFrame;
    int vertexSpace;
    int dirtyFlag;

    int mipmapNumber;
    int mipmapCount;
};

typedef int(*NodeFunction)(void* parameters, EvaluationInfo* evaluation, EvaluationContext* context);

struct Evaluator
{
    Evaluator() : mGLSLProgram(0), mCFunction(0)
    {
    }
    unsigned int mGLSLProgram;
	NodeFunction mCFunction;
#if USE_PYTHON    
    pybind11::module mPyModule;

    void RunPython() const;
#endif

    std::string mName;
    size_t mNodeType;
};

struct Evaluators
{
    Evaluators();
    ~Evaluators();

    void SetEvaluators(const std::vector<EvaluatorFile>& evaluatorfilenames);
    std::string GetEvaluator(const std::string& filename);
    static void InitPython();
    int GetMask(size_t nodeType);
    void ClearEvaluators();

    const Evaluator& GetEvaluator(size_t nodeType) const
    {
        return mEvaluatorPerNodeType[nodeType];
    }

    void InitPythonModules();
#if USE_PYTHON    
    pybind11::module mImogenModule;
#endif
    static void ReloadPlugins();
    protected:
    struct EvaluatorScript
    {
        EvaluatorScript() : mProgram(0), mCFunction(0), mType(-1)
        {
        }
        EvaluatorScript(const std::string& text) : mText(text), mProgram(0), mCFunction(0), mType(-1)
        {
        }
        std::string mText;
        unsigned int mProgram;
		NodeFunction mCFunction;
        int mType;
#if USE_PYTHON        
        pybind11::module mPyModule;
#endif
    };

    std::map<std::string, EvaluatorScript> mEvaluatorScripts;
    std::vector<Evaluator> mEvaluatorPerNodeType;
};

extern Evaluators gEvaluators;
struct EvaluationContext;
struct Image;
struct EvaluationStages;
struct Scene;

namespace EvaluationAPI
{
    // API
    int GetEvaluationImage(EvaluationContext* evaluationContext, int target, Image* image);
    int SetEvaluationImage(EvaluationContext* evaluationContext, int target, const Image* image);
    int SetEvaluationImageCube(EvaluationContext* evaluationContext, int target, const Image* image, int cubeFace);
    int SetThumbnailImage(EvaluationContext* evaluationContext, Image* image);
    int AllocateImage(Image* image);

    // static int Evaluate(int target, int width, int height, Image *image);
    void SetBlendingMode(EvaluationContext* evaluationContext, int target, int blendSrc, int blendDst);
    void EnableDepthBuffer(EvaluationContext* evaluationContext, int target, int enable);
    void EnableFrameClear(EvaluationContext* evaluationContext, int target, int enable);
    void SetVertexSpace(EvaluationContext* evaluationContext, int target, int vertexSpace);
    int OverrideInput(EvaluationContext* evaluationContext, int target, int inputIndex, int newInputTarget);

    // int SetNodeImage(int target, Image *image);
    int GetEvaluationSize(const EvaluationContext* evaluationContext, int target, int* imageWidth, int* imageHeight);
    int SetEvaluationSize(EvaluationContext* evaluationContext, int target, int imageWidth, int imageHeight);
    int SetEvaluationCubeSize(EvaluationContext* evaluationContext, int target, int faceWidth, int mipmapCount);
    int Job(EvaluationContext* evaluationContext, int (*jobFunction)(void*), void* ptr, unsigned int size);
    int JobMain(EvaluationContext* evaluationContext, int (*jobMainFunction)(void*), void* ptr, unsigned int size);
    void SetProcessing(EvaluationContext* context, int target, int processing);
    int AllocateComputeBuffer(EvaluationContext* context, int target, int elementCount, int elementSize);

    int LoadScene(const char* filename, void** scene);
    int SetEvaluationScene(EvaluationContext* evaluationContext, int target, void* scene);
    int GetEvaluationScene(EvaluationContext* evaluationContext, int target, void** scene);
    int SetEvaluationRTScene(EvaluationContext* evaluationContext, int target, void* scene);
    int GetEvaluationRTScene(EvaluationContext* evaluationContext, int target, void** scene);

    const char* GetEvaluationSceneName(EvaluationContext* evaluationContext, int target);
    int GetEvaluationRenderer(EvaluationContext* evaluationContext, int target, void** renderer);
    int InitRenderer(EvaluationContext* evaluationContext, int target, int mode, void* scene);
    int UpdateRenderer(EvaluationContext* evaluationContext, int target);

    int Read(EvaluationContext* evaluationContext, const char* filename, Image* image);
    int Write(EvaluationContext* evaluationContext, const char* filename, Image* image, int format, int quality);
    int Evaluate(EvaluationContext* evaluationContext, int target, int width, int height, Image* image);

    int ReadGLTF(EvaluationContext* evaluationContext, const char* filename, Scene** scene);
    int GLTFReadAsync(EvaluationContext* context, const char* filename, int target);
    int ReadImageAsync(EvaluationContext* context, const char *filename, int target, int face);
} // namespace EvaluationAPI