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

#include "Platform.h"
#include <fstream>
#include "Bitmap.h"
#include "Utils.h"
#include "Mem.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define NANOSVG_ALL_COLOR_KEYWORDS // Include full list of color keywords.
#define NANOSVG_IMPLEMENTATION     // Expands implementation
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#include "cmft/image.h"
#if USE_FFMPEG
#include "ffmpegCodec.h"
#endif
ImageCache gImageCache;
DefaultShaders gDefaultShader;
#ifdef GL_BGR
const unsigned int glInputFormats[] = {
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
const unsigned int glInternalFormats[] = {
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
#else
const unsigned int glInputFormats[] = {
    GL_RGB,
    GL_RGB,
    GL_RGB,
    GL_RGB,
    GL_RGB,
    GL_RGBA, // RGBE

    GL_RGBA,
    GL_RGBA,
    GL_RGBA,
    GL_RGBA,
    GL_RGBA,

    GL_RGBA, // RGBM
};
const unsigned int glInternalFormats[] = {
    GL_RGB,
    GL_RGB,
    GL_RGB,
    GL_RGB,
    GL_RGB,
    GL_RGBA, // RGBE

    GL_RGBA,
    GL_RGBA,
    GL_RGBA,
    GL_RGBA,
    GL_RGBA,

    GL_RGBA, // RGBM
};

#endif
const unsigned int glCubeFace[] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};
const unsigned int textureFormatSize[] = {3, 3, 6, 6, 12, 4, 4, 4, 8, 8, 16, 4};
const unsigned int textureComponentCount[] = {3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4};

void SaveCapture(const std::string& filemane, int x, int y, int w, int h)
{
    w &= 0xFFFFFFFC;
    h &= 0xFFFFFFFC;

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    unsigned char* imgBits = new unsigned char[w * h * 4];

    glReadPixels(x, viewport[3] - y - h, w, h, GL_RGB, GL_UNSIGNED_BYTE, imgBits);

    stbi_write_png(filemane.c_str(), w, h, 3, imgBits, w * 3);
    delete[] imgBits;
}

#if USE_FFMPEG
Image Image::DecodeImage(FFMPEGCodec::Decoder* decoder, int frame)
{
    decoder->ReadFrame(frame);
    Image image;
    image.mDecoder = decoder;
    image.mNumMips = 1;
    image.mNumFaces = 1;
    image.mFormat = TextureFormat::BGR8;
    image.mWidth = int(decoder->mWidth);
    image.mHeight = int(decoder->mHeight);
    size_t lineSize = image.mWidth * 3;
    size_t imgDataSize = lineSize * image.mHeight;
    image.Allocate(imgDataSize);

    unsigned char* pdst = image.GetBits();
    unsigned char* psrc = (unsigned char*)decoder->GetRGBData();
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
#endif
int Image::LoadSVG(const char* filename, Image* image, float dpi)
{
    NSVGimage* svgImage;
    svgImage = nsvgParseFromFile(filename, "px", dpi);
    if (!svgImage)
        return EVAL_ERR;

    int width = (int)svgImage->width;
    int height = (int)svgImage->height;

    // Create rasterizer (can be used to render multiple images).
    NSVGrasterizer* rast = nsvgCreateRasterizer();

    // Allocate memory for image
    size_t imgSize = width * height * 4;
    unsigned char* img = (unsigned char*)malloc(imgSize);

    // Rasterize
    nsvgRasterize(rast, svgImage, 0, 0, 1, img, width, height, width * 4);

    image->SetBits(img, imgSize);
    image->mWidth = width;
    image->mHeight = height;
    image->mNumMips = 1;
    image->mNumFaces = 1;
    image->mFormat = TextureFormat::RGBA8;
    image->mDecoder = NULL;

    free(img);
    Image::VFlip(image);
    nsvgDelete(svgImage);
    nsvgDeleteRasterizer(rast);
    return EVAL_OK;
}

int Image::Read(const char* filename, Image* image)
{
    std::string filenameStr(filename);
    Image* cacheImage = gImageCache.GetImage(filenameStr);
    if (cacheImage)
    {
        *image = *cacheImage;
        return EVAL_OK;
    }
    FILE* fp = fopen(filename, "rb");
    if (!fp)
    {
        Log("Unable to open image %s\n", filename);
        return EVAL_ERR;
    }
    fclose(fp);

    int components;
    unsigned char* bits = stbi_load(filename, &image->mWidth, &image->mHeight, &components, 0);
    if (!bits)
    {
        cmft::Image img;
        if (!cmft::imageLoad(img, filename))
        {
            return EVAL_ERR;
        }
        cmft::imageTransformUseMacroInstead(&img, cmft::IMAGE_OP_FLIP_X, UINT32_MAX);
        image->SetBits((unsigned char*)img.m_data, img.m_dataSize);
        image->mWidth = img.m_width;
        image->mHeight = img.m_height;
        image->mNumMips = img.m_numMips;
        image->mNumFaces = img.m_numFaces;
        image->mFormat = img.m_format;
        image->mDecoder = NULL;
        gImageCache.AddImage(filenameStr, image);
        return EVAL_OK;
    }

    image->SetBits(bits, image->mWidth * image->mHeight * components);
    image->mNumMips = 1;
    image->mNumFaces = 1;
    image->mFormat = (components == 3) ? TextureFormat::RGB8 : TextureFormat::RGBA8;
    image->mDecoder = NULL;
    stbi_image_free(bits);
    gImageCache.AddImage(filenameStr, image);
    return EVAL_OK;
}

int Image::Free(Image* image)
{
    image->DoFree();
    return EVAL_OK;
}

unsigned int Image::Upload(const Image* image, unsigned int textureId, int cubeFace)
{
    bool allocTexture = false;
    if (!textureId)
    {
        glGenTextures(1, &textureId);
    }
    unsigned int targetType = (cubeFace == -1) ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
    glBindTexture(targetType, textureId);

    unsigned int inputFormat = glInputFormats[image->mFormat];
    unsigned int internalFormat = glInternalFormats[image->mFormat];
    glTexImage2D((cubeFace == -1) ? GL_TEXTURE_2D : glCubeFace[cubeFace],
                 0,
                 internalFormat,
                 image->mWidth,
                 image->mHeight,
                 0,
                 inputFormat,
                 GL_UNSIGNED_BYTE,
                 image->GetBits());
    TexParam(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, targetType);
    if (allocTexture)
    {
        vramTextureAlloc(image->mWidth * image->mHeight * textureFormatSize[image->mFormat]);
    }
    glBindTexture(targetType, 0);
    return textureId;
}

int Image::ReadMem(unsigned char* data, size_t dataSize, Image* image)
{
    int components;
    unsigned char* bits = stbi_load_from_memory(data, int(dataSize), &image->mWidth, &image->mHeight, &components, 0);
    if (!bits)
        return EVAL_ERR;
    image->SetBits(bits, image->mWidth * image->mHeight * components);
    stbi_image_free(bits);
    return EVAL_OK;
}

void Image::VFlip(Image* image)
{
    int pixelSize = (image->mFormat == TextureFormat::RGB8) ? 3 : 4;
    int stride = image->mWidth * pixelSize;
    for (int y = 0; y < image->mHeight / 2; y++)
    {
        for (int x = 0; x < stride; x++)
        {
            unsigned char* p1 = &image->GetBits()[y * stride + x];
            unsigned char* p2 = &image->GetBits()[(image->mHeight - 1 - y) * stride + x];
            Swap(*p1, *p2);
        }
    }
}

int Image::Write(const char* filename, Image* image, int format, int quality)
{
    if (!image->mWidth || !image->mHeight)
    {
        return EVAL_ERR;
    }

    int components = textureComponentCount[image->mFormat];
    switch (format)
    {
        case 0:
            if (!stbi_write_jpg(filename, image->mWidth, image->mHeight, components, image->GetBits(), quality))
            {
                return EVAL_ERR;
            }
            break;
        case 1:
            if (!stbi_write_png(
                    filename, image->mWidth, image->mHeight, components, image->GetBits(), image->mWidth * components))
            {
                return EVAL_ERR;
            }
            break;
        case 2:
            if (!stbi_write_tga(filename, image->mWidth, image->mHeight, components, image->GetBits()))
            {
                return EVAL_ERR;
            }
            break;
        case 3:
            if (!stbi_write_bmp(filename, image->mWidth, image->mHeight, components, image->GetBits()))
            {
                return EVAL_ERR;
            }
            break;
        case 4:
            // if (stbi_write_hdr(filename, image->width, image->height, image->components, image->bits))
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
            {
                cmft::imageConvert(img, cmft::TextureFormat::BGRA8);
            }
            else if (img.m_format == cmft::TextureFormat::RGB8)
            {
                cmft::imageConvert(img, cmft::TextureFormat::BGR8);
            }
            image->SetBits((unsigned char*)img.m_data, img.m_dataSize);
            if (!cmft::imageSave(img, filename, cmft::ImageFileType::DDS))
            {
                return EVAL_ERR;
            }
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
            {
                return EVAL_ERR;
            }
        }
        break;
        case 7:
        {
            assert(0);
        }
        break;
    }
    return EVAL_OK;
}

int Image::EncodePng(Image* image, std::vector<unsigned char>& pngImage)
{
    int outlen;
    int components = 4; // TODO
    unsigned char* bits = stbi_write_png_to_mem(
        image->GetBits(), image->mWidth * components, image->mWidth, image->mHeight, components, &outlen);
    if (!bits)
        return EVAL_ERR;
    pngImage.resize(outlen);
    memcpy(pngImage.data(), bits, outlen);

    free(bits);
    return EVAL_OK;
}

void DefaultShaders::Init()
{
    std::ifstream prgStr("Stock/ProgressingNode.glsl");
    std::ifstream cubStr("Stock/DisplayCubemap.glsl");
    std::ifstream nodeErrStr("Stock/NodeError.glsl");

    mProgressShader =
        prgStr.good()
            ? LoadShader(std::string(std::istreambuf_iterator<char>(prgStr), std::istreambuf_iterator<char>()),
                         "progressShader")
            : 0;
    mDisplayCubemapShader =
        cubStr.good()
            ? LoadShader(std::string(std::istreambuf_iterator<char>(cubStr), std::istreambuf_iterator<char>()),
                         "cubeDisplay")
            : 0;
    mNodeErrorShader =
        nodeErrStr.good()
            ? LoadShader(std::string(std::istreambuf_iterator<char>(nodeErrStr), std::istreambuf_iterator<char>()),
                         "nodeError")
            : 0;
}

unsigned int ImageCache::GetTexture(const std::string& filename)
{
    auto iter = mSynchronousTextureCache.find(filename);
    if (iter != mSynchronousTextureCache.end())
        return iter->second;

    Image image;
    unsigned int textureId = 0;
    if (Image::Read(filename.c_str(), &image) == EVAL_OK)
    {
        textureId = Image::Upload(&image, 0);
        Image::Free(&image);
    }

    mSynchronousTextureCache[filename] = textureId;
    return textureId;
}

Image* ImageCache::GetImage(const std::string& filepath)
{
    Image* ret = NULL;
    mCacheAccess.lock();
    auto iter = mImageCache.find(filepath);
    if (iter != mImageCache.end())
    {
        ret = &iter->second;
    }
    mCacheAccess.unlock();
    return ret;
}

void ImageCache::AddImage(const std::string& filepath, Image* image)
{
    mCacheAccess.lock();
    auto iter = mImageCache.find(filepath);
    if (iter == mImageCache.end())
    {
        mImageCache.insert(std::make_pair(filepath, *image));
    }
    mCacheAccess.unlock();
}

void RenderTarget::BindAsTarget() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
    glViewport(0, 0, mImage->mWidth, mImage->mHeight);
}

void RenderTarget::BindAsCubeTarget() const
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, mGLTexID);
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
}

void RenderTarget::BindCubeFace(size_t face, int mipmap, int faceWidth)
{
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face), mGLTexID, mipmap);
    glViewport(0, 0, faceWidth >> mipmap, faceWidth >> mipmap);
}

void RenderTarget::Destroy()
{
    if (mGLTexID)
    {
        glDeleteTextures(1, &mGLTexID);
        vramTextureFree(mImage->mWidth * mImage->mHeight * 4);
    }
    if (mGLTexDepth)
    {
        glDeleteTextures(1, &mGLTexDepth);
        vramTextureFree(mImage->mWidth * mImage->mHeight * 3);
    }
    if (mFbo)
    {
        if (glIsFramebuffer(mFbo))
        {
            glDeleteFramebuffers(1, &mFbo);
        }
        else
        {
            Log("Trying to delete FBO %d that is unknown to OpenGL\n", mFbo);
        }
    }
    if (mDepthBuffer)
    {
        glDeleteRenderbuffers(1, &mDepthBuffer);
    }
    mFbo = 0;
    mGLTexDepth = 0;
    mImage->mWidth = mImage->mHeight = 0;
    mGLTexID = 0;
}

void RenderTarget::Clone(const RenderTarget& other)
{
    // TODO: clone other type of render target
    InitBuffer(other.mImage->mWidth, other.mImage->mHeight, other.mDepthBuffer);
}

void RenderTarget::Swap(RenderTarget& other)
{
    ::Swap(mImage, other.mImage);
    ::Swap(mGLTexID, other.mGLTexID);
    ::Swap(mGLTexDepth, other.mGLTexDepth);
    ::Swap(mDepthBuffer, other.mDepthBuffer);
    ::Swap(mFbo, other.mFbo);
}

void RenderTarget::InitBuffer(int width, int height, bool depthBuffer)
{
    if ((width == mImage->mWidth) && (mImage->mHeight == height) && mImage->mNumFaces == 1 &&
        (!(depthBuffer ^ (mDepthBuffer != 0))))
        return;
    Destroy();
    if (!width || !height)
    {
        Log("Trying to init FBO with 0 sized dimension.\n");
    }
    mImage->mWidth = width;
    mImage->mHeight = height;
    mImage->mNumMips = 1;
    mImage->mNumFaces = 1;
    mImage->mFormat = TextureFormat::RGBA8;

    glGenFramebuffers(1, &mFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

    // diffuse
    glGenTextures(1, &mGLTexID);
    glBindTexture(GL_TEXTURE_2D, mGLTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    TexParam(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_2D);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mGLTexID, 0);
    vramTextureAlloc(mImage->mWidth * mImage->mHeight * 4);
    if (depthBuffer)
    {
        // Z
        glGenTextures(1, &mGLTexDepth);
        glBindTexture(GL_TEXTURE_2D, mGLTexDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        TexParam(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_2D);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mGLTexDepth, 0);
        vramTextureAlloc(mImage->mWidth * mImage->mHeight * 3);
    }

    static const GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(sizeof(drawBuffers) / sizeof(GLenum), drawBuffers);


    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CheckFBO();

    GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);
    BindAsTarget();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | (depthBuffer ? GL_DEPTH_BUFFER_BIT : 0));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}

void RenderTarget::InitCube(int width, int mipmapCount)
{
    if ((width == mImage->mWidth) && (mImage->mHeight == width) && mImage->mNumFaces == 6 &&
        (mImage->mNumMips == mipmapCount))
        return;
    Destroy();

    if (!width)
    {
        Log("Trying to init FBO with 0 sized dimension.\n");
    }

    mImage->mWidth = width;
    mImage->mHeight = width;
    mImage->mNumMips = mipmapCount;
    mImage->mNumFaces = 6;
    mImage->mFormat = TextureFormat::RGBA8;

    glGenFramebuffers(1, &mFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);

    glGenTextures(1, &mGLTexID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mGLTexID);

    for (int mip = 0; mip < mipmapCount; mip++)
    {
        for (int i = 0; i < 6; i++)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         mip,
                         GL_RGBA,
                         width >> mip,
                         width >> mip,
                         0,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         NULL);
            vramTextureAlloc((width >> mip) * (width >> mip) * 4);
        }
    }

    TexParam(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_CUBE_MAP);

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
            // Log("Framebuffer complete.\n");
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
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            Log("[ERROR] Framebuffer incomplete: Draw buffer.\n");
            break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            Log("[ERROR] Framebuffer incomplete: Read buffer.\n");
            break;
#endif
        case GL_FRAMEBUFFER_UNSUPPORTED:
            Log("[ERROR] Unsupported by FBO implementation.\n");
            break;

        default:
            Log("[ERROR] Unknow error.\n");
            break;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
