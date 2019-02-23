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

#include "EvaluationStages.h"
#include "EvaluationContext.h"
#include "Evaluators.h"
#include <vector>
#include <algorithm>
#include <map>
#include <GL/gl3w.h>    // Initialize with gl3wInit()

EvaluationStages::EvaluationStages() : mFrameMin(0), mFrameMax(1)
{
    
}

void EvaluationStages::AddSingleEvaluation(size_t nodeType)
{
    EvaluationStage evaluation;
#ifdef _DEBUG
    evaluation.mTypename          = gMetaNodes[nodeType].mName;;
#endif
    evaluation.mDecoder               = NULL;
    evaluation.mUseCountByOthers      = 0;
    evaluation.mType              = nodeType;
    evaluation.mParametersBuffer      = 0;
    evaluation.mBlendingSrc           = ONE;
    evaluation.mBlendingDst           = ZERO;
    evaluation.mLocalTime             = 0;
    evaluation.gEvaluationMask        = gEvaluators.GetMask(nodeType);
    evaluation.mbDepthBuffer          = false;
    evaluation.scene = nullptr;
    evaluation.renderer = nullptr;
    mStages.push_back(evaluation);
}

void EvaluationStages::StageIsAdded(int index)
{
    for (size_t i = 0;i< mStages.size();i++)
    {
        if (i == index)
            continue;
        auto& evaluation = mStages[i];
        for (auto& inp : evaluation.mInput.mInputs)
        {
            if (inp >= index)
                inp++;
        }
    }
}

void EvaluationStages::StageIsDeleted(int index)
{
    EvaluationStage& ev = mStages[index];
    ev.Clear();

    // shift all connections
    for (auto& evaluation : mStages)
    {
        for (auto& inp : evaluation.mInput.mInputs)
        {
            if (inp >= index)
                inp--;
        }
    }
}

void EvaluationStages::UserAddEvaluation(size_t nodeType)
{
    URAdd<EvaluationStage> undoRedoAddStage(int(mStages.size()), [&]() {return &mStages; },
        [&](int index) {StageIsDeleted(index); }, [&](int index) {StageIsAdded(index); });

    AddSingleEvaluation(nodeType);
}

void EvaluationStages::UserDeleteEvaluation(size_t target)
{
    URDel<EvaluationStage> undoRedoDelStage(int(target), [&]() {return &mStages; },
        [&](int index) {StageIsDeleted(index); }, [&](int index) {StageIsAdded(index); });

    StageIsDeleted(int(target));
    mStages.erase(mStages.begin() + target);
}

void EvaluationStages::SetEvaluationParameters(size_t target, const std::vector<unsigned char> &parameters)
{
    EvaluationStage& stage = mStages[target];
    stage.mParameters = parameters;

    if (stage.gEvaluationMask&EvaluationGLSL)
        BindGLSLParameters(stage);
    if (stage.mDecoder)
        stage.mDecoder = NULL;
}

void EvaluationStages::SetEvaluationSampler(size_t target, const std::vector<InputSampler>& inputSamplers)
{
    mStages[target].mInputSamplers = inputSamplers;
}

void EvaluationStages::AddEvaluationInput(size_t target, int slot, int source)
{
    if (mStages[target].mInput.mInputs[slot] == source)
        return;
    mStages[target].mInput.mInputs[slot] = source;
    mStages[source].mUseCountByOthers++;
}

void EvaluationStages::DelEvaluationInput(size_t target, int slot)
{
    mStages[mStages[target].mInput.mInputs[slot]].mUseCountByOthers--;
    mStages[target].mInput.mInputs[slot] = -1;
}

void EvaluationStages::SetEvaluationOrder(const std::vector<size_t> nodeOrderList)
{
    mEvaluationOrderList = nodeOrderList;
}

void EvaluationStages::Clear()
{
    for (auto& ev : mStages)
        ev.Clear();

    mStages.clear();
    mEvaluationOrderList.clear();
    mAnimTrack.clear();
}

void EvaluationStages::SetMouse(int target, float rx, float ry, bool lButDown, bool rButDown)
{
    for (auto& ev : mStages)
    {
        ev.mRx = -9999.f;
        ev.mRy = -9999.f;
        ev.mLButDown = false;
        ev.mRButDown = false;
    }
    auto& ev = mStages[target];
    ev.mRx = rx;
    ev.mRy = 1.f - ry; // inverted for UI
    ev.mLButDown = lButDown;
    ev.mRButDown = rButDown;
}

size_t EvaluationStages::GetEvaluationImageDuration(size_t target)
{
    auto& stage = mStages[target];
    if (!stage.mDecoder)
        return 1;
    if (stage.mDecoder->mFrameCount > 2000)
    {
        int a = 1;
    }
    return stage.mDecoder->mFrameCount;
}

void EvaluationStages::SetStageLocalTime(size_t target, int localTime, bool updateDecoder)
{
    auto& stage = mStages[target];
    int newLocalTime = ImMin(localTime, int(GetEvaluationImageDuration(target)));
    if (stage.mDecoder && updateDecoder && stage.mLocalTime != newLocalTime)
    {
        stage.mLocalTime = newLocalTime;
        Image image = stage.DecodeImage();
        SetEvaluationImage(this, int(target), &image);
        Image::Free(&image);
    }
    else
    {
        stage.mLocalTime = newLocalTime;
    }
}
/* TODO
int EvaluationStages::Evaluate(int target, int width, int height, Image *image)
{
    EvaluationContext *previousContext = gCurrentContext;
    EvaluationContext context(*this, true, width, height);
    gCurrentContext = &context;
    while (context.RunBackward(target))
    {
        // processing... maybe good on next run
    }
    GetEvaluationImage(target, image);
    gCurrentContext = previousContext;
    return EVAL_OK;
}
*/
FFMPEGCodec::Decoder* EvaluationStages::FindDecoder(const std::string& filename)
{
    for (auto& evaluation : mStages)
    {
        if (evaluation.mDecoder && evaluation.mDecoder->GetFilename() == filename)
            return evaluation.mDecoder.get();
    }
    auto decoder = new FFMPEGCodec::Decoder;
    decoder->Open(filename);
    return decoder;
}


Camera *EvaluationStages::GetCameraParameter(size_t index)
{
    if (index >= mStages.size())
        return NULL;
    EvaluationStage& stage = mStages[index];
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[stage.mType];
    const size_t paramsSize = ComputeNodeParametersSize(stage.mType);
    stage.mParameters.resize(paramsSize);
    unsigned char *paramBuffer = stage.mParameters.data();
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (param.mType == Con_Camera)
        {
            Camera *cam = (Camera*)paramBuffer;
            return cam;
        }
        paramBuffer += GetParameterTypeSize(param.mType);
    }

    return NULL;
}
// TODO : create parameter struct with templated accessors
int EvaluationStages::GetIntParameter(size_t index, const char *parameterName, int defaultValue)
{
    if (index >= mStages.size())
        return NULL;
    EvaluationStage& stage = mStages[index];
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[stage.mType];
    const size_t paramsSize = ComputeNodeParametersSize(stage.mType);
    stage.mParameters.resize(paramsSize);
    unsigned char *paramBuffer = stage.mParameters.data();
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (param.mType == Con_Int)
        {
            if (!strcmp(param.mName.c_str(), parameterName))
            {
                int *value = (int*)paramBuffer;
                return *value;
            }
        }
        paramBuffer += GetParameterTypeSize(param.mType);
    }
    return defaultValue;
}


void EvaluationStages::SetBlendingMode(EvaluationStages *evaluationStages, int target, int blendSrc, int blendDst)
{
    EvaluationStage& evaluation = evaluationStages->mStages[target];

    evaluation.mBlendingSrc = blendSrc;
    evaluation.mBlendingDst = blendDst;
}

void EvaluationStages::EnableDepthBuffer(EvaluationStages *evaluationStages, int target, int enable)
{
    EvaluationStage& evaluation = evaluationStages->mStages[target];
    evaluation.mbDepthBuffer = enable != 0;
}


int EvaluationStages::GetEvaluationSize(EvaluationStages *evaluationStages, int target, int *imageWidth, int *imageHeight)
{
    if (target < 0 || target >= evaluationStages->mStages.size())
        return EVAL_ERR;
    auto renderTarget = gCurrentContext->GetRenderTarget(target);
    if (!renderTarget)
        return EVAL_ERR;
    *imageWidth = renderTarget->mImage.mWidth;
    *imageHeight = renderTarget->mImage.mHeight;
    return EVAL_OK;
}

int EvaluationStages::SetEvaluationSize(EvaluationStages *evaluationStages, int target, int imageWidth, int imageHeight)
{
    if (target < 0 || target >= evaluationStages->mStages.size())
        return EVAL_ERR;
    auto renderTarget = gCurrentContext->GetRenderTarget(target);
    if (!renderTarget)
        return EVAL_ERR;
    //if (gCurrentContext->GetEvaluationInfo().uiPass)
    //    return EVAL_OK;
    renderTarget->InitBuffer(imageWidth, imageHeight, evaluationStages->mStages[target].mbDepthBuffer);
    return EVAL_OK;
}

int EvaluationStages::SetEvaluationCubeSize(EvaluationStages *evaluationStages, int target, int faceWidth)
{
    if (target < 0 || target >= evaluationStages->mStages.size())
        return EVAL_ERR;

    auto renderTarget = gCurrentContext->GetRenderTarget(target);
    if (!renderTarget)
        return EVAL_ERR;
    renderTarget->InitCube(faceWidth);
    return EVAL_OK;
}


int EvaluationStages::SetEvaluationScene(EvaluationStages *evaluationStages, int target, void *scene)
{
    evaluationStages->mStages[target].scene = scene;
    return EVAL_OK;
}

int EvaluationStages::GetEvaluationScene(EvaluationStages *evaluationStages, int target, void **scene)
{
    *scene = evaluationStages->mStages[target].scene;
    return EVAL_OK;
}

int EvaluationStages::GetEvaluationRenderer(EvaluationStages *evaluationStages, int target, void **renderer)
{
    *renderer = evaluationStages->mStages[target].renderer;
    return EVAL_OK;
}


int EvaluationStages::GetEvaluationImage(EvaluationStages *evaluationStages, int target, Image *image)
{
    if (target == -1 || target >= evaluationStages->mStages.size())
        return EVAL_ERR;

    RenderTarget& tgt = *gCurrentContext->GetRenderTarget(target);

    // compute total size
    Image& img = tgt.mImage;
    unsigned int texelSize = textureFormatSize[img.mFormat];
    unsigned int texelFormat = glInternalFormats[img.mFormat];
    uint32_t size = 0;// img.mNumFaces * img.mWidth * img.mHeight * texelSize;
    for (int i = 0; i < img.mNumMips; i++)
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
                glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + cube, i, texelFormat, GL_UNSIGNED_BYTE, ptr);
                ptr += (img.mWidth >> i) * (img.mHeight >> i) * texelSize;
            }
        }
    }
    return EVAL_OK;
}

int EvaluationStages::SetEvaluationImage(EvaluationStages *evaluationStages, int target, Image *image)
{
    EvaluationStage &stage = evaluationStages->mStages[target];
    auto tgt = gCurrentContext->GetRenderTarget(target);
    if (!tgt)
        return EVAL_ERR;
    unsigned int texelSize = textureFormatSize[image->mFormat];
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

int EvaluationStages::InitRenderer(EvaluationStages *evaluationStages, int target, int mode, void *scene)
{
    GLSLPathTracer::Scene *rdscene = (GLSLPathTracer::Scene *)scene;
    evaluationStages->mStages[target].scene = scene;

    GLSLPathTracer::Renderer *currentRenderer = (GLSLPathTracer::Renderer*)evaluationStages->mStages[target].renderer;
    if (!currentRenderer)
    {
        //auto renderer = new GLSLPathTracer::TiledRenderer(rdscene, "Stock/PathTracer/Tiled/");
        auto renderer = new GLSLPathTracer::ProgressiveRenderer(rdscene, "Stock/PathTracer/Progressive/");
        renderer->init();
        evaluationStages->mStages[target].renderer = renderer;
    }
    return EVAL_OK;
}

int EvaluationStages::UpdateRenderer(EvaluationStages *evaluationStages, int target)
{
    GLSLPathTracer::Renderer *renderer = (GLSLPathTracer::Renderer *)evaluationStages->mStages[target].renderer;
    GLSLPathTracer::Scene *rdscene = (GLSLPathTracer::Scene *)evaluationStages->mStages[target].scene;

    Camera* camera = evaluationStages->GetCameraParameter(target);
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
