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
#include "EvaluationStages.h"
#include <vector>
#include <algorithm>
#include <assert.h>
#include <SDL.h>



#include <fstream>
#include <streambuf>
#include "imgui.h"
#include "imgui_internal.h"
#include "TaskScheduler.h"
#include "NodesDelegate.h"
#include "cmft/print.h"


extern enki::TaskScheduler g_TS;



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
    if (mGLTexDepth)
        glDeleteTextures(1, &mGLTexDepth);
    if (mFbo)
        glDeleteFramebuffers(1, &mFbo);
    if (mDepthBuffer)
        glDeleteRenderbuffers(1, &mDepthBuffer);
    mFbo = 0;
    mImage.mWidth = mImage.mHeight = 0;
    mGLTexID = 0;
}

void RenderTarget::Clone(const RenderTarget &other)
{
    // TODO: clone other type of render target
    InitBuffer(other.mImage.mWidth, other.mImage.mHeight, other.mDepthBuffer);
}

void RenderTarget::Swap(RenderTarget &other)
{
    ::Swap(mImage, other.mImage);
    ::Swap(mGLTexID, other.mGLTexID);
    ::Swap(mGLTexDepth, other.mGLTexDepth);
    ::Swap(mDepthBuffer, other.mDepthBuffer);
    ::Swap(mFbo, other.mFbo);
}

void RenderTarget::InitBuffer(int width, int height, bool depthBuffer)
{
    if ((width == mImage.mWidth) && (mImage.mHeight == height) && mImage.mNumFaces == 1 && (!(depthBuffer ^ (mDepthBuffer != 0))))
        return;
    Destroy();

    mImage.mWidth = width;
    mImage.mHeight = height;
    mImage.mNumMips = 1;
    mImage.mNumFaces = 1;
    mImage.mFormat = TextureFormat::RGBA8;

    glGenFramebuffers(1, &mFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

    // diffuse
    glGenTextures(1, &mGLTexID);
    glBindTexture(GL_TEXTURE_2D, mGLTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    TexParam(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_2D);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mGLTexID, 0);

    if (depthBuffer)
    {
        // Z
        glGenTextures(1, &mGLTexDepth);
        glBindTexture(GL_TEXTURE_2D, mGLTexDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        TexParam(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_2D);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mGLTexDepth, 0);
    }

    static const GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(sizeof(DrawBuffers) / sizeof(GLenum), DrawBuffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CheckFBO();

    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    BindAsTarget();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT|(depthBuffer?GL_DEPTH_BUFFER_BIT:0));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
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

Image EvaluationStage::DecodeImage()
{
    return ::DecodeImage(mDecoder.get(), mLocalTime);
}




int EvaluationStages::GetEvaluationImage(int target, Image *image)
{
    if (target == -1 || target >= gNodeDelegate.mEvaluationStages.mStages.size())
        return EVAL_ERR;

    RenderTarget& tgt = *gCurrentContext->GetRenderTarget(target);

    // compute total size
    Image& img = tgt.mImage;
    unsigned int texelSize = GetTexelSize(img.mFormat);
    unsigned int texelFormat = glInternalFormats[img.mFormat];
    uint32_t size = 0;// img.mNumFaces * img.mWidth * img.mHeight * texelSize;
    for (int i = 0;i<img.mNumMips;i++)
        size += img.mNumFaces * (img.mWidth >> i) * (img.mHeight >> i) * texelSize;

    image->Allocate(size);
    image->mWidth = img.mWidth;
    image->mHeight = img.mHeight;
    image->mNumMips = img.mNumMips;
    image->mFormat = img.mFormat;
    image->mNumFaces = img.mNumFaces;

    glBindTexture(GL_TEXTURE_2D, tgt.mGLTexID);
    unsigned char *ptr = image->GetBits();
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

int EvaluationStages::SetEvaluationImage(int target, Image *image)
{
    EvaluationStage &stage = gNodeDelegate.mEvaluationStages.mStages[target];
    auto tgt = gCurrentContext->GetRenderTarget(target);
    if (!tgt)
        return EVAL_ERR;
    unsigned int texelSize = GetTexelSize(image->mFormat);
    unsigned int inputFormat = glInputFormats[image->mFormat];
    unsigned int internalFormat = glInternalFormats[image->mFormat];
    unsigned char *ptr = image->GetBits();
    if (image->mNumFaces == 1)
    {
        tgt->InitBuffer(image->mWidth, image->mHeight, stage.mbDepthBuffer);

        glBindTexture(GL_TEXTURE_2D, tgt->mGLTexID);

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
        tgt->InitCube(image->mWidth);
        glBindTexture(GL_TEXTURE_CUBE_MAP, tgt->mGLTexID);

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
    if (stage.mDecoder.get() != (FFMPEGCodec::Decoder*)image->mDecoder)
        stage.mDecoder = std::shared_ptr<FFMPEGCodec::Decoder>((FFMPEGCodec::Decoder*)image->mDecoder);
    gCurrentContext->SetTargetDirty(target, true);
    return EVAL_OK;
}

int EvaluationStages::SetEvaluationImageCube(int target, Image *image, int cubeFace)
{
    if (image->mNumFaces != 1)
        return EVAL_ERR;
    RenderTarget& tgt = *gCurrentContext->GetRenderTarget(target);

    tgt.InitCube(image->mWidth);

    UploadImage(image, tgt.mGLTexID, cubeFace);
    gCurrentContext->SetTargetDirty(target, true);
    return EVAL_OK;
}



int EvaluationStages::AllocateImage(Image *image)
{
    return EVAL_OK;
}




int EvaluationStages::SetThumbnailImage(Image *image)
{
    std::vector<unsigned char> pngImage;
    if (Image::EncodePng(image, pngImage) == EVAL_ERR)
        return EVAL_ERR;

    extern Library library;
    extern Imogen imogen;

    int materialIndex = imogen.GetCurrentMaterialIndex();
    Material & material = library.mMaterials[materialIndex];
    material.mThumbnail = pngImage;
    material.mThumbnailTextureId = 0;
    return EVAL_OK;
}

int EvaluationStages::SetNodeImage(int target, Image *image)
{
    std::vector<unsigned char> pngImage;
    if (Image::EncodePng(image, pngImage) == EVAL_ERR)
        return EVAL_ERR;

    extern Library library;
    extern Imogen imogen;

    int materialIndex = imogen.GetCurrentMaterialIndex();
    Material & material = library.mMaterials[materialIndex];
    material.mMaterialNodes[target].mImage = pngImage;

    return EVAL_OK;
}

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

void EvaluationStages::SetProcessing(int target, int processing)
{
    gCurrentContext->StageSetProcessing(target, processing);
}

int EvaluationStages::AllocateComputeBuffer(int target, int elementCount, int elementSize)
{
    gCurrentContext->AllocateComputeBuffer(target, elementCount, elementSize);
    return EVAL_OK;
}

int EvaluationStages::Job(int(*jobFunction)(void*), void *ptr, unsigned int size)
{
    if (gCurrentContext->IsSynchronous())
    {
        return jobFunction(ptr);
    }
    else
    {
        g_TS.AddTaskSetToPipe(new CFunctionTaskSet(jobFunction, ptr, size));
    }
    return EVAL_OK;
}

int EvaluationStages::JobMain(int(*jobMainFunction)(void*), void *ptr, unsigned int size)
{
    if (gCurrentContext->IsSynchronous())
    {
        return jobMainFunction(ptr);
    }
    else
    {
        g_TS.AddPinnedTask(new CFunctionMainTask(jobMainFunction, ptr, size));
    }
    return EVAL_OK;
}

void EvaluationStages::SetBlendingMode(int target, int blendSrc, int blendDst)
{
    EvaluationStage& evaluation = gNodeDelegate.mEvaluationStages.mStages[target];

    evaluation.mBlendingSrc = blendSrc;
    evaluation.mBlendingDst = blendDst;
}

void EvaluationStages::EnableDepthBuffer(int target, int enable)
{
    EvaluationStage& evaluation = gNodeDelegate.mEvaluationStages.mStages[target];
    evaluation.mbDepthBuffer = enable != 0;
}

void EvaluationStages::BindGLSLParameters(EvaluationStage& stage)
{
    if (!stage.mParametersBuffer)
    {
        glGenBuffers(1, &stage.mParametersBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, stage.mParametersBuffer);

        glBufferData(GL_UNIFORM_BUFFER, stage.mParameters.size(), stage.mParameters.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, stage.mParametersBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
    else
    {
        glBindBuffer(GL_UNIFORM_BUFFER, stage.mParametersBuffer);
        glBufferData(GL_UNIFORM_BUFFER, stage.mParameters.size(), stage.mParameters.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

void EvaluationStage::Clear()
{
    if (gEvaluationMask&EvaluationGLSL)
        glDeleteBuffers(1, &mParametersBuffer);
    mParametersBuffer = 0;
}




int EvaluationStages::GetEvaluationSize(int target, int *imageWidth, int *imageHeight)
{
    if (target < 0 || target >= gNodeDelegate.mEvaluationStages.mStages.size())
        return EVAL_ERR;
    auto renderTarget = gCurrentContext->GetRenderTarget(target);
    if (!renderTarget)
        return EVAL_ERR;
    *imageWidth = renderTarget->mImage.mWidth;
    *imageHeight = renderTarget->mImage.mHeight;
    return EVAL_OK;
}

int EvaluationStages::SetEvaluationSize(int target, int imageWidth, int imageHeight)
{
    if (target < 0 || target >= gNodeDelegate.mEvaluationStages.mStages.size())
        return EVAL_ERR;
    auto renderTarget = gCurrentContext->GetRenderTarget(target);
    if (!renderTarget)
        return EVAL_ERR;
    //if (gCurrentContext->GetEvaluationInfo().uiPass)
    //    return EVAL_OK;
    renderTarget->InitBuffer(imageWidth, imageHeight, gNodeDelegate.mEvaluationStages.mStages[target].mbDepthBuffer);
    return EVAL_OK;
}

int EvaluationStages::SetEvaluationCubeSize(int target, int faceWidth)
{
    if (target < 0 || target >= gNodeDelegate.mEvaluationStages.mStages.size())
        return EVAL_ERR;

    auto renderTarget = gCurrentContext->GetRenderTarget(target);
    if (!renderTarget)
        return EVAL_ERR;
    renderTarget->InitCube(faceWidth);
    return EVAL_OK;
}

#include "Scene.h"
#include "Loader.h"
#include "TiledRenderer.h"
#include "ProgressiveRenderer.h"
#include "GPUBVH.h"
#include "Camera.h"

int EvaluationStages::LoadScene(const char *filename, void **pscene)
{
    // todo: make a real good cache system
    static std::map<std::string, GLSLPathTracer::Scene *> cachedScenes;
    std::string sFilename(filename);
    auto iter = cachedScenes.find(sFilename);
    if (iter != cachedScenes.end())
    {
        *pscene = iter->second;
        return EVAL_OK;
    }

    GLSLPathTracer::Scene *scene = GLSLPathTracer::LoadScene(sFilename);
    if (!scene)
    {
        Log("Unable to load scene\n");
        return EVAL_ERR;
    }
    cachedScenes.insert(std::make_pair(sFilename, scene));
    *pscene = scene;

    Log("Scene Loaded\n\n");

    scene->buildBVH();

    // --------Print info on memory usage ------------- //

    Log("Triangles: %d\n", scene->triangleIndices.size());
    Log("Triangle Indices: %d\n", scene->gpuBVH->bvhTriangleIndices.size());
    Log("Vertices: %d\n", scene->vertexData.size());

    long long scene_data_bytes =
        sizeof(GLSLPathTracer::GPUBVHNode) * scene->gpuBVH->bvh->getNumNodes() +
        sizeof(GLSLPathTracer::TriangleData) * scene->gpuBVH->bvhTriangleIndices.size() +
        sizeof(GLSLPathTracer::VertexData) * scene->vertexData.size() +
        sizeof(GLSLPathTracer::NormalTexData) * scene->normalTexData.size() +
        sizeof(GLSLPathTracer::MaterialData) * scene->materialData.size() +
        sizeof(GLSLPathTracer::LightData) * scene->lightData.size();

    Log("GPU Memory used for BVH and scene data: %d MB\n", scene_data_bytes / 1048576);

    long long tex_data_bytes =
        scene->texData.albedoTextureSize.x * scene->texData.albedoTextureSize.y * scene->texData.albedoTexCount * 3 +
        scene->texData.metallicRoughnessTextureSize.x * scene->texData.metallicRoughnessTextureSize.y * scene->texData.metallicRoughnessTexCount * 3 +
        scene->texData.normalTextureSize.x * scene->texData.normalTextureSize.y * scene->texData.normalTexCount * 3 +
        scene->hdrLoaderRes.width * scene->hdrLoaderRes.height * sizeof(GL_FLOAT) * 3;

    Log("GPU Memory used for Textures: %d MB\n", tex_data_bytes / 1048576);

    Log("Total GPU Memory used: %d MB\n", (scene_data_bytes + tex_data_bytes) / 1048576);

    return EVAL_OK;
}

int EvaluationStages::SetEvaluationScene(int target, void *scene)
{
    gNodeDelegate.mEvaluationStages.mStages[target].scene = scene;
    return EVAL_OK;
}

int EvaluationStages::GetEvaluationScene(int target, void **scene)
{
    *scene = gNodeDelegate.mEvaluationStages.mStages[target].scene;
    return EVAL_OK;
}

int EvaluationStages::GetEvaluationRenderer(int target, void **renderer)
{
    *renderer = gNodeDelegate.mEvaluationStages.mStages[target].renderer;
    return EVAL_OK;
}

int EvaluationStages::InitRenderer(int target, int mode, void *scene)
{
    GLSLPathTracer::Scene *rdscene = (GLSLPathTracer::Scene *)scene;
    gNodeDelegate.mEvaluationStages.mStages[target].scene = scene;

    GLSLPathTracer::Renderer *currentRenderer = (GLSLPathTracer::Renderer*)gNodeDelegate.mEvaluationStages.mStages[target].renderer;
    if (!currentRenderer)
    {
        //auto renderer = new GLSLPathTracer::TiledRenderer(rdscene, "Stock/PathTracer/Tiled/");
        auto renderer = new GLSLPathTracer::ProgressiveRenderer(rdscene, "Stock/PathTracer/Progressive/");
        renderer->init();
        gNodeDelegate.mEvaluationStages.mStages[target].renderer = renderer;
    }
    return EVAL_OK;
}

int EvaluationStages::UpdateRenderer(int target)
{
    GLSLPathTracer::Renderer *renderer = (GLSLPathTracer::Renderer *)gNodeDelegate.mEvaluationStages.mStages[target].renderer;
    GLSLPathTracer::Scene *rdscene = (GLSLPathTracer::Scene *)gNodeDelegate.mEvaluationStages.mStages[target].scene;
 
    Camera* camera = gNodeDelegate.GetCameraParameter(target);
    if (camera)
    {
        Vec4 pos = camera->mPosition;
        Vec4 lk = camera->mPosition + camera->mDirection;
        GLSLPathTracer::Camera newCam(glm::vec3(pos.x, pos.y, pos.z), glm::vec3(lk.x, lk.y, lk.z), 90.f);
        newCam.updateCamera();
        *rdscene->camera = newCam;
    }

    renderer->update(0.0166f);
    auto tgt = gCurrentContext->GetRenderTarget(target);
    renderer->render();

    tgt->BindAsTarget();
    renderer->present();

    float progress = renderer->getProgress();
    gCurrentContext->StageSetProgress(target, progress);
    bool renderDone = progress >= 1.f - FLT_EPSILON;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);

    if (renderDone)
    {
        gCurrentContext->StageSetProcessing(target, false);
        return EVAL_OK;
    }
    return EVAL_DIRTY;
}

void EvaluationStages::DrawUIProgress(size_t nodeIndex)
{
    glUseProgram(gDefaultShader.mProgressShader);
    glUniform1f(glGetUniformLocation(gDefaultShader.mProgressShader, "time"), float(double(SDL_GetTicks()) / 1000.0));
    gFSQuad.Render();
}

void EvaluationStages::DrawUISingle(size_t nodeIndex)
{
    EvaluationInfo evaluationInfo;
    evaluationInfo.forcedDirty = 1;
    evaluationInfo.uiPass = 1;
    gCurrentContext->RunSingle(nodeIndex, evaluationInfo);
}

void EvaluationStages::DrawUICubemap(size_t nodeIndex)
{
    glUseProgram(gDefaultShader.mDisplayCubemapShader);
    int tgt = glGetUniformLocation(gDefaultShader.mDisplayCubemapShader, "samplerCubemap");
    glUniform1i(tgt, 0);
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, gCurrentContext->GetEvaluationTexture(nodeIndex));
    gFSQuad.Render();
}