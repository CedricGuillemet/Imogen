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
#include "Types.h"
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
    float world[16];
    float worldViewProjection[16];
    float viewport[4];

    float mouse[4];
    float keyModifier[4];
    float inputIndices[8];


    float uiPass; // pass
    float passNumber;
    float frame;
    float localFrame;

    
    float targetIndex; //target
    float vertexSpace;
    float mipmapNumber;
    float mipmapCount;

	float textureSize[8 * 4];
    // not used by shaders
    uint32_t dirtyFlag;
};

typedef int(*NodeFunction)(void* parameters, EvaluationInfo* evaluation, EvaluationContext* context);

struct Evaluators
{
    Evaluators();
    ~Evaluators();

    void SetEvaluators();
    static void InitPython();
    int GetMask(size_t nodeType) const;
    void Clear();

    void InitPythonModules();
#if USE_PYTHON    
    pybind11::module mImogenModule;
#endif
    static void ReloadPlugins();

    struct EvaluatorScript
    {
        EvaluatorScript() : mProgram({0}), mCFunction(0), mType(-1), mMask(0)
        {
        }
        void Clear();

		bgfx::ProgramHandle mProgram;
        NodeFunction mCFunction;
        int mType;
        int mMask;
        std::vector<bgfx::UniformHandle> mUniformHandles;
#if USE_PYTHON    
        pybind11::module mPyModule;

        void RunPython() const;
#endif
    };

    const EvaluatorScript& GetEvaluator(size_t nodeType) const
    {
        return *mEvaluatorPerNodeType[nodeType];
    }
    void ApplyEvaluationInfo(const EvaluationInfo& evaluationInfo);
protected:
    std::map<std::string, EvaluatorScript> mEvaluatorScripts;
    std::vector<EvaluatorScript*> mEvaluatorPerNodeType;
    std::vector<bgfx::ShaderHandle> mShaderHandles;
public:
    bgfx::UniformHandle u_viewRot{ bgfx::kInvalidHandle };
    bgfx::UniformHandle u_viewProjection{ bgfx::kInvalidHandle };
    bgfx::UniformHandle u_viewInverse{ bgfx::kInvalidHandle };
    bgfx::UniformHandle u_world{ bgfx::kInvalidHandle };
    bgfx::UniformHandle u_worldViewProjection{ bgfx::kInvalidHandle };
    bgfx::UniformHandle u_mouse{ bgfx::kInvalidHandle };
    bgfx::UniformHandle u_keyModifier{ bgfx::kInvalidHandle };
    bgfx::UniformHandle u_inputIndices{ bgfx::kInvalidHandle };
    bgfx::UniformHandle u_target{ bgfx::kInvalidHandle };
    bgfx::UniformHandle u_pass{ bgfx::kInvalidHandle };
    bgfx::UniformHandle u_viewport{ bgfx::kInvalidHandle };

	bgfx::ProgramHandle mBlitProgram{ bgfx::kInvalidHandle };
	bgfx::ProgramHandle mProgressProgram{ bgfx::kInvalidHandle };
	bgfx::ProgramHandle mDisplayCubemapProgram{ bgfx::kInvalidHandle };
    std::vector<bgfx::UniformHandle> mSamplers2D;
    std::vector<bgfx::UniformHandle> mSamplersCube;
    bgfx::UniformHandle u_time{ bgfx::kInvalidHandle };
    bgfx::UniformHandle u_uvTransform{ bgfx::kInvalidHandle };
	bgfx::UniformHandle u_textureSize{ bgfx::kInvalidHandle };
};

extern Evaluators gEvaluators;
struct EvaluationContext;
struct Image;
struct EvaluationStages;
struct Scene;

namespace EvaluationAPI
{
    // API
    int GetEvaluationImage(EvaluationContext* evaluationContext, NodeIndex target, Image* image);
    int SetEvaluationImage(EvaluationContext* evaluationContext, NodeIndex target, const Image* image);
    int SetEvaluationImageCube(EvaluationContext* evaluationContext, NodeIndex target, const Image* image, int cubeFace);
    int SetThumbnailImage(EvaluationContext* evaluationContext, Image* image);

    void SetBlendingMode(EvaluationContext* evaluationContext, NodeIndex target, uint64_t blendSrc, uint64_t blendDst);
    void EnableDepthBuffer(EvaluationContext* evaluationContext, NodeIndex target, int enable);
    void EnableFrameClear(EvaluationContext* evaluationContext, NodeIndex target, int enable);
    void SetVertexSpace(EvaluationContext* evaluationContext, NodeIndex target, int vertexSpace);
    int OverrideInput(EvaluationContext* evaluationContext, NodeIndex target, int inputIndex, int newInputTarget);
	int IsBuilding(EvaluationContext* evaluationContext);

    int GetEvaluationSize(const EvaluationContext* evaluationContext, NodeIndex target, int* imageWidth, int* imageHeight);
    int SetEvaluationSize(EvaluationContext* evaluationContext, NodeIndex target, int imageWidth, int imageHeight);
	int SetEvaluationPersistent(EvaluationContext* evaluationContext, NodeIndex target, int persistent);
    int SetEvaluationCubeSize(EvaluationContext* evaluationContext, NodeIndex target, int faceWidth, int hasMipmap);
    int Job(EvaluationContext* evaluationContext, int (*jobFunction)(void*), void* ptr, unsigned int size);
    int JobMain(EvaluationContext* evaluationContext, int (*jobMainFunction)(void*), void* ptr, unsigned int size);
    void SetProcessing(EvaluationContext* context, NodeIndex target, int processing);

    int LoadScene(const char* filename, void** scene);
    int SetEvaluationScene(EvaluationContext* evaluationContext, NodeIndex target, void* scene);
    int GetEvaluationScene(EvaluationContext* evaluationContext, NodeIndex target, void** scene);
    int SetEvaluationRTScene(EvaluationContext* evaluationContext, NodeIndex target, void* scene);
    int GetEvaluationRTScene(EvaluationContext* evaluationContext, NodeIndex target, void** scene);

    const char* GetEvaluationSceneName(EvaluationContext* evaluationContext, NodeIndex target);
    int GetEvaluationRenderer(EvaluationContext* evaluationContext, NodeIndex target, void** renderer);
    int InitRenderer(EvaluationContext* evaluationContext, NodeIndex target, int mode, void* scene);
    int UpdateRenderer(EvaluationContext* evaluationContext, NodeIndex target);

    int Read(EvaluationContext* evaluationContext, const char* filename, Image* image);
    int Write(EvaluationContext* evaluationContext, const char* filename, Image* image, int format, int quality);

    int ReadGLTF(EvaluationContext* evaluationContext, const char* filename, Scene** scene);
    int GLTFReadAsync(EvaluationContext* context, const char* filename, NodeIndex target);
    int ReadImageAsync(EvaluationContext* context, const char *filename, NodeIndex target, int face);
} // namespace EvaluationAPI