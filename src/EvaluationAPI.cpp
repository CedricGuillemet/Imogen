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
#include "ffmpegCodec.h"

extern enki::TaskScheduler g_TS;
extern cmft::ClContext* clContext;

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


unsigned int GetTexelSize(uint8_t fmt)
{
    return textureFormatSize[fmt];
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

    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    BindAsTarget();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
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

void Evaluation::APIInit()
{
    std::ifstream prgStr("Stock/ProgressingNode.glsl");
    std::ifstream cubStr("Stock/DisplayCubemap.glsl");
    std::ifstream nodeErrStr("Stock/NodeError.glsl");

    mProgressShader = prgStr.good() ? LoadShader(std::string(std::istreambuf_iterator<char>(prgStr), std::istreambuf_iterator<char>()), "progressShader") : 0;
    mDisplayCubemapShader = cubStr.good() ? LoadShader(std::string(std::istreambuf_iterator<char>(cubStr), std::istreambuf_iterator<char>()), "cubeDisplay") : 0;
    mNodeErrorShader = nodeErrStr.good() ? LoadShader(std::string(std::istreambuf_iterator<char>(nodeErrStr), std::istreambuf_iterator<char>()), "nodeError") : 0;
}

static Image_t DecodeImage(FFMPEGCodec::Decoder *decoder, int frame)
{
    decoder->ReadFrame(frame);
    Image_t image;
    image.mDecoder = decoder;
    image.mNumMips = 1;
    image.mNumFaces = 1;
    image.mFormat = TextureFormat::BGR8;
    image.mWidth = int(decoder->mWidth);
    image.mHeight = int(decoder->mHeight);
    size_t lineSize = image.mWidth * 3;
    size_t imgDataSize = lineSize * image.mHeight;
    image.Allocate(imgDataSize);

    unsigned char *pdst = image.GetBits();
    unsigned char *psrc = (unsigned char*)decoder->GetRGBData();
    if (psrc && pdst)
    {
        psrc += imgDataSize - lineSize;
        for (int j = 0; j < image.mHeight; j++)
        {
            memcpy(pdst, psrc, lineSize);
            pdst += lineSize;
            psrc -= lineSize;
        }
    }
    return image;
}

Image_t EvaluationStage::DecodeImage()
{
    return ::DecodeImage(mDecoder.get(), mLocalTime);
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
            auto decoder = gEvaluation.FindDecoder(filename);
            *image = ::DecodeImage(decoder, gEvaluationTime);
            return EVAL_OK;
        }
        cmft::imageTransformUseMacroInstead(&img, cmft::IMAGE_OP_FLIP_X, UINT32_MAX);
        image->SetBits((unsigned char*)img.m_data);
        image->mWidth = img.m_width;
        image->mHeight = img.m_height;
        image->mDataSize = img.m_dataSize;
        image->mNumMips = img.m_numMips;
        image->mNumFaces = img.m_numFaces;
        image->mFormat = img.m_format;
        image->mDecoder = NULL;
        return EVAL_OK;
    }

    image->SetBits(bits);
    image->mDataSize = image->mWidth * image->mHeight * components;
    image->mNumMips = 1;
    image->mNumFaces = 1;
    image->mFormat = (components == 3) ? TextureFormat::RGB8 : TextureFormat::RGBA8;
    image->mDecoder = NULL;
    return EVAL_OK;
}

int Evaluation::ReadImageMem(unsigned char *data, size_t dataSize, Image *image)
{
    int components;
    unsigned char *bits = stbi_load_from_memory(data, int(dataSize), &image->mWidth, &image->mHeight, &components, 0);
    if (!bits)
        return EVAL_ERR;
    image->SetBits(bits);
    return EVAL_OK;
}

int Evaluation::WriteImage(const char *filename, Image *image, int format, int quality)
{
    int components = textureComponentCount[image->mFormat];
    switch (format)
    {
    case 0:
        if (!stbi_write_jpg(filename, image->mWidth, image->mHeight, components, image->GetBits(), quality))
            return EVAL_ERR;
        break;
    case 1:
        if (!stbi_write_png(filename, image->mWidth, image->mHeight, components, image->GetBits(), image->mWidth * components))
            return EVAL_ERR;
        break;
    case 2:
        if (!stbi_write_tga(filename, image->mWidth, image->mHeight, components, image->GetBits()))
            return EVAL_ERR;
        break;
    case 3:
        if (!stbi_write_bmp(filename, image->mWidth, image->mHeight, components, image->GetBits()))
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
        img.m_data = image->GetBits();
        img.m_dataSize = image->mDataSize;
        if (img.m_format == cmft::TextureFormat::RGBA8)
            cmft::imageConvert(img, cmft::TextureFormat::BGRA8);
        else if (img.m_format == cmft::TextureFormat::RGB8)
            cmft::imageConvert(img, cmft::TextureFormat::BGR8);
        image->SetBits((unsigned char*)img.m_data);
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
        img.m_data = image->GetBits();
        img.m_dataSize = image->mDataSize;
        if (!cmft::imageSave(img, filename, cmft::ImageFileType::KTX))
            return EVAL_ERR;
    }
    break;
    case 7:
    {
        FFMPEGCodec::Encoder *encoder = gCurrentContext->GetEncoder(std::string(filename), image->mWidth, image->mHeight);
        std::string fn(filename);
        encoder->AddFrame(image->GetBits(), image->mWidth, image->mHeight);
    }
        break;
    }
    return EVAL_OK;
}

int Evaluation::GetEvaluationImage(int target, Image *image)
{
    if (target == -1 || target >= gEvaluation.mStages.size())
        return EVAL_ERR;

    RenderTarget& tgt = *gCurrentContext->GetRenderTarget(target);

    // compute total size
    Image_t& img = tgt.mImage;
    unsigned int texelSize = GetTexelSize(img.mFormat);
    unsigned int texelFormat = glInternalFormats[img.mFormat];
    uint32_t size = 0;// img.mNumFaces * img.mWidth * img.mHeight * texelSize;
    for (int i = 0;i<img.mNumMips;i++)
        size += img.mNumFaces * (img.mWidth >> i) * (img.mHeight >> i) * texelSize;

    image->Allocate(size);
    image->mDataSize = size;
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

int Evaluation::SetEvaluationImage(int target, Image *image)
{
    EvaluationStage &stage = gEvaluation.mStages[target];
    auto tgt = gCurrentContext->GetRenderTarget(target);
    if (!tgt)
        return EVAL_ERR;
    unsigned int texelSize = GetTexelSize(image->mFormat);
    unsigned int inputFormat = glInputFormats[image->mFormat];
    unsigned int internalFormat = glInternalFormats[image->mFormat];
    unsigned char *ptr = image->GetBits();
    if (image->mNumFaces == 1)
    {
        tgt->InitBuffer(image->mWidth, image->mHeight);

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

int Evaluation::SetEvaluationImageCube(int target, Image *image, int cubeFace)
{
    if (image->mNumFaces != 1)
        return EVAL_ERR;
    RenderTarget& tgt = *gCurrentContext->GetRenderTarget(target);

    tgt.InitCube(image->mWidth);

    UploadImage(image, tgt.mGLTexID, cubeFace);
    gCurrentContext->SetTargetDirty(target, true);
    return EVAL_OK;
}

int Evaluation::CubemapFilter(Image *image, int faceSize, int lightingModel, int excludeBase, int glossScale, int glossBias)
{
    cmft::Image img;
    img.m_data = image->GetBits();
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
        , gCPUCount
        , clContext))
        return EVAL_ERR;

    image->SetBits((unsigned char*)img.m_data);
    image->mDataSize = img.m_dataSize;
    image->mNumMips = img.m_numMips;
    image->mNumFaces = img.m_numFaces;
    image->mWidth = img.m_width;
    image->mHeight = img.m_height;
    image->mFormat = img.m_format;
    return EVAL_OK;
}

int Evaluation::AllocateImage(Image *image)
{
    return EVAL_OK;
}

int Evaluation::FreeImage(Image *image)
{
    image->Free();
    return EVAL_OK;
}

int Evaluation::EncodePng(Image *image, std::vector<unsigned char> &pngImage)
{
    int outlen;
    int components = 4; // TODO
    unsigned char *bits = stbi_write_png_to_mem(image->GetBits(), image->mWidth * components, image->mWidth, image->mHeight, components, &outlen);
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
    gCurrentContext->StageSetProcessing(target, processing != 0);
}

int Evaluation::Job(int(*jobFunction)(void*), void *ptr, unsigned int size)
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

int Evaluation::JobMain(int(*jobMainFunction)(void*), void *ptr, unsigned int size)
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

void Evaluation::SetBlendingMode(int target, int blendSrc, int blendDst)
{
    EvaluationStage& evaluation = gEvaluation.mStages[target];

    evaluation.mBlendingSrc = blendSrc;
    evaluation.mBlendingDst = blendDst;
}

void Evaluation::BindGLSLParameters(EvaluationStage& stage)
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

unsigned int Evaluation::UploadImage(Image *image, unsigned int textureId, int cubeFace)
{
    if (!textureId)
        glGenTextures(1, &textureId);

    unsigned int targetType = (cubeFace == -1) ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
    glBindTexture(targetType, textureId);

    unsigned int inputFormat = glInputFormats[image->mFormat];
    unsigned int internalFormat = glInternalFormats[image->mFormat];
    glTexImage2D((cubeFace==-1)? GL_TEXTURE_2D: glCubeFace[cubeFace], 0, internalFormat, image->mWidth, image->mHeight, 0, inputFormat, GL_UNSIGNED_BYTE, image->GetBits());
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
        FreeImage(&image);
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
            gCurrentContext->RunSingle(cb.mNodeIndex, evaluationInfo);
        }
        break;
        case CBUI_Progress:
        {
            glUseProgram(gEvaluation.mProgressShader);
            glUniform1f(glGetUniformLocation(gEvaluation.mProgressShader, "time"), float(double(SDL_GetTicks())/1000.0));
            gFSQuad.Render();
        }
        break;
        case CBUI_Cubemap:
        {
            glUseProgram(gEvaluation.mDisplayCubemapShader);
            int tgt = glGetUniformLocation(gEvaluation.mDisplayCubemapShader, "samplerCubemap");
            glUniform1i(tgt, 0);
            glActiveTexture(GL_TEXTURE0);

            glBindTexture(GL_TEXTURE_CUBE_MAP, gCurrentContext->GetEvaluationTexture(cb.mNodeIndex));
            gFSQuad.Render();
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
    if (target < 0 || target >= gEvaluation.mStages.size())
        return EVAL_ERR;
    auto renderTarget = gCurrentContext->GetRenderTarget(target);
    if (!renderTarget)
        return EVAL_ERR;
    *imageWidth = renderTarget->mImage.mWidth;
    *imageHeight = renderTarget->mImage.mHeight;
    return EVAL_OK;
}

int Evaluation::SetEvaluationSize(int target, int imageWidth, int imageHeight)
{
    if (target < 0 || target >= gEvaluation.mStages.size())
        return EVAL_ERR;
    auto renderTarget = gCurrentContext->GetRenderTarget(target);
    if (!renderTarget)
        return EVAL_ERR;
    //if (gCurrentContext->GetEvaluationInfo().uiPass)
    //    return EVAL_OK;
    renderTarget->InitBuffer(imageWidth, imageHeight);
    return EVAL_OK;
}

int Evaluation::SetEvaluationCubeSize(int target, int faceWidth)
{
    if (target < 0 || target >= gEvaluation.mStages.size())
        return EVAL_ERR;

    auto renderTarget = gCurrentContext->GetRenderTarget(target);
    if (!renderTarget)
        return EVAL_ERR;
    renderTarget->InitCube(faceWidth);
    return EVAL_OK;
}