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
#include <bimg/decode.h>
#include <bimg/encode.h>
#include <bx/allocator.h>
#include <bx/file.h>
#include <bgfx/bgfx.h>
#include "JSGlue.h"

#define NANOSVG_ALL_COLOR_KEYWORDS // Include full list of color keywords.
#define NANOSVG_IMPLEMENTATION     // Expands implementation
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#if USE_FFMPEG
#include "ffmpegCodec.h"
#endif
ImageCache gImageCache;
DefaultShaders gDefaultShader;

bx::AllocatorI* getDefaultAllocator()
{
    static bx::DefaultAllocator s_allocator;
    return &s_allocator;
}

bimg::TextureFormat::Enum GetBIMGFormat(TextureFormat::Enum format)
{
    switch (format)
    {
    case TextureFormat::BGR8: return bimg::TextureFormat::RGB8;
    case TextureFormat::RGB8: return bimg::TextureFormat::RGB8;
    case TextureFormat::RGB16: assert(0);
    case TextureFormat::RGB16F: assert(0);
    case TextureFormat::RGB32F: assert(0);
    case TextureFormat::RGBE: return bimg::TextureFormat::RGB8;

    case TextureFormat::BGRA8: return bimg::TextureFormat::BGRA8;
    case TextureFormat::RGBA8: return bimg::TextureFormat::RGBA8;
    case TextureFormat::RGBA16: return bimg::TextureFormat::RGBA16;
    case TextureFormat::RGBA16F: assert(0);
    case TextureFormat::RGBA32F: return bimg::TextureFormat::RGBA32F;

    case TextureFormat::RGBM: return bimg::TextureFormat::RGB8;
    default:
        break;
    }
    return bimg::TextureFormat::Unknown;
}

bimg::Quality::Enum GetQuality(int quality)
{
    switch (quality)
    {
    case 0:
    case 1:
    case 2:
        return bimg::Quality::Highest;
    case 7:
    case 8:
    case 9:
        return bimg::Quality::Fastest;
    default:
        return bimg::Quality::Default;
    }
}

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
/* todogl
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
*/
const unsigned int glInputFormats[12] = { 0 };
const unsigned int glInternalFormats[12] = { 0 };
#endif
/* todogl
const unsigned int glCubeFace[] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};
*/
const unsigned int textureFormatSize[] = {3, 3, 6, 6, 12, 4, 4, 4, 8, 8, 16, 4};
const unsigned int textureComponentCount[] = {3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4};

void SaveCapture(const std::string& filemane, int x, int y, int w, int h)
{
    w &= 0xFFFFFFFC;
    h &= 0xFFFFFFFC;
	
	/* todogl
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
	*/
    unsigned char* imgBits = new unsigned char[w * h * 4];
	/*todogl
    glReadPixels(x, viewport[3] - y - h, w, h, GL_RGB, GL_UNSIGNED_BYTE, imgBits);
	*/
    bx::FileWriter writer;
    bx::Error err;

    if (bx::open(&writer, filemane.c_str(), false, &err))
    {
        bimg::imageWritePng(&writer, w, h, w * 3, imgBits, bimg::TextureFormat::RGB8, false/*_yflip*/, &err);
    }

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

    auto buffer = ReadFile(filename);

    if (buffer.size())
    {
        return ReadMem((unsigned char*)buffer.data(), buffer.size(), image, filename);
    }
    return EVAL_ERR;
}

int Image::Free(Image* image)
{
    image->DoFree();
    return EVAL_OK;
}

unsigned int Image::Upload(const Image* image, unsigned int textureId, int cubeFace, int mipmap)
{
	/* todogl
    bool allocTexture = false;
    if (!textureId)
    {
        glGenTextures(1, &textureId);
    }
    unsigned int targetType = (cubeFace == -1) ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
    glBindTexture(targetType, textureId);

    unsigned int inputFormat = glInputFormats[image->mFormat];
    unsigned int internalFormat = glInternalFormats[image->mFormat];
	unsigned int texelSize = textureFormatSize[image->mFormat];

	unsigned int offset = 0;
	if (image->mNumFaces > 1)
	{
		for (int face = 0; face < cubeFace; face++)
		{
			for (int i = 0; i < image->mNumMips; i++)
			{
				offset += (image->mWidth >> i) * (image->mWidth >> i) * texelSize;
			}
		}
	}
	for (int i = 0; i < mipmap; i++)
	{
		offset += (image->mWidth >> i) * (image->mWidth >> i) * texelSize;
	}

    glTexImage2D((cubeFace == -1) ? GL_TEXTURE_2D : glCubeFace[cubeFace],
				 mipmap,
                 internalFormat,
                 image->mWidth >> mipmap,
                 image->mHeight >> mipmap,
                 0,
                 inputFormat,
                 GL_UNSIGNED_BYTE,
                 image->GetBits() + offset);
    TexParam(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, targetType);
    if (allocTexture)
    {
        vramTextureAlloc((image->mWidth >> mipmap) * (image->mHeight >> mipmap) * textureFormatSize[image->mFormat]);
    }
    glBindTexture(targetType, 0);
	*/
    return textureId;
}

int Image::ReadMem(unsigned char* data, size_t dataSize, Image* image, const char* filename)
{
    bx::AllocatorI* g_allocator = getDefaultAllocator();
    bimg::ImageContainer* imageContainer = bimg::imageParse(g_allocator, data, (uint32_t)dataSize, bimg::TextureFormat::RGBA8); // todo handle HDR float
    if (imageContainer)
    {
        image->SetBits((unsigned char*)imageContainer->m_data, imageContainer->m_size);
        image->mWidth = imageContainer->m_width;
        image->mHeight = imageContainer->m_height;
        image->mNumMips = imageContainer->m_numMips;
        image->mNumFaces = imageContainer->m_cubeMap ? 6 : 1;
        image->mFormat = (imageContainer->m_format == bimg::TextureFormat::RGB8) ? TextureFormat::RGB8 : TextureFormat::RGBA8;
        image->mDecoder = NULL;
        if (filename)
        {
            gImageCache.AddImage(filename, image);
        }
        bimg::imageFree(imageContainer);
        return EVAL_OK;
    }

    return EVAL_ERR;
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

    const int components = textureComponentCount[image->mFormat];
    
    bx::AllocatorI* g_allocator = getDefaultAllocator();
    bx::Error err;
#ifdef __EMSCRIPTEN__
    bx::MemoryBlock mb(g_allocator);
    bx::MemoryWriter writer(&mb);
#else
    bx::FileWriter writer;
    if (bx::open(&writer, filename, false, &err))
#endif
    {
        auto texformat = GetBIMGFormat((TextureFormat::Enum)image->mFormat);

        switch (format)
        {
        case 0: // jpeg
            bimg::imageWriteJpg(&writer, image->mWidth, image->mHeight, image->mWidth * components, image->GetBits(), texformat, false/*_yflip*/, quality, &err);
        case 1: // png
            bimg::imageWritePng(&writer, image->mWidth, image->mHeight, image->mWidth * components, image->GetBits(), texformat, false/*_yflip*/, &err);
            break;
        case 2: // tga
            bimg::imageWriteTga(&writer, image->mWidth, image->mHeight, image->mWidth * components, image->GetBits(), texformat, false/*_yflip*/, &err);
            break;
        case 4: // hdr
            bimg::imageWriteHdr(&writer, image->mWidth, image->mHeight, image->mWidth * components, image->GetBits(), texformat, false/*_yflip*/, &err);
            break;
        case 5: // dds
            {
                bimg::ImageContainer* imageContainer = bimg::imageAlloc(
                    g_allocator
                    , texformat
                    , image->mWidth
                    , image->mHeight
                    , 1
                    , 1
                    , (image->mNumFaces == 6) ? true : false
                    , (image->mNumMips>1)
                    , image->GetBits()
                );

                if (imageContainer)
                {
                    bimg::imageWriteDds(
                        &writer
                        , *imageContainer
                        , image->GetBits()
                        , image->mDataSize
                        , &err
                    );
                    bimg::imageFree(imageContainer);
                }
            }
            break;
        case 6: // ktx
            bimg::imageWriteKtx(
                &writer
                , texformat
                , (image->mNumFaces == 6)
                , image->mWidth
                , image->mHeight
                , 1
                , image->mNumMips
                , 1
                , image->GetBits()
                , &err
            );
            break;
        case 7: // exr
            bimg::imageWriteExr(&writer, image->mWidth, image->mHeight, image->mWidth * components, image->GetBits(), texformat, false/*_yflip*/, &err);
            break;
        }
#ifdef __EMSCRIPTEN__
        DownloadImage(filename, strlen(filename), (const char*)mb.more(0), mb.getSize());
#else
        bx::close(&writer);
#endif
        return EVAL_OK;
    }
    return EVAL_ERR;
}

int Image::EncodePng(Image* image, std::vector<unsigned char>& pngImage)
{
    bx::AllocatorI* g_allocator = getDefaultAllocator();
    bx::MemoryBlock mb(g_allocator);
    bx::MemoryWriter writer(&mb);
    bx::Error err;

    bimg::imageWritePng(&writer, image->mWidth, image->mHeight, image->mWidth * 4, image->GetBits(), bimg::TextureFormat::RGBA8, false/*_yflip*/, &err);
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
    /* todogl
	glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
    glViewport(0, 0, mImage->mWidth, mImage->mHeight);
	*/
}

void RenderTarget::BindAsCubeTarget() const
{
    /* todogl
	
	glBindTexture(GL_TEXTURE_CUBE_MAP, mGLTexID);
    glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
	*/
}

void RenderTarget::BindCubeFace(size_t face, int mipmap, int faceWidth)
{
    /* todogl
	glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face), mGLTexID, mipmap);
    glViewport(0, 0, faceWidth >> mipmap, faceWidth >> mipmap);
	*/
}

void RenderTarget::Destroy()
{
    /*
	todogl
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
	*/
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
	/* todogl
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
	*/
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

	/*
	todogl
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
	*/
}

void RenderTarget::CheckFBO()
{
	/* todogl
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
	*/
}
