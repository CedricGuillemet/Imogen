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

bx::AllocatorI* getDefaultAllocator()
{
    static bx::DefaultAllocator s_allocator;
    return &s_allocator;
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

void SaveCapture(const std::string& filemane, int x, int y, int w, int h)
{
    w &= 0xFFFFFFFC;
    h &= 0xFFFFFFFC;
	
	//bgfx::readTexture(
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
    image.mHasMipmaps = false;
    image.mIsCubemap = false;
    image.mFormat = TextureFormat::RGB8; // todo:shoud be bgr8
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
    image->mHasMipmaps = false;
    image->mIsCubemap = false;
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

TextureHandle Image::Upload(const Image* image, TextureHandle textureHandle, int cubeFace, int mipmap)
{
    bool allocTexture = false;
    if (!textureHandle.idx)
    {
		textureHandle = bgfx::createTexture2D(
			uint16_t(image->mWidth)
			, uint16_t(image->mHeight)
			, image->mHasMipmaps
			, uint16_t(1)
			, bgfx::TextureFormat::Enum(image->mFormat)
			, uint64_t(0)
			, nullptr
		);
		allocTexture = true;
    }
	assert(textureHandle.idx);
	assert(cubeFace == -1);

	unsigned int texelSize = bimg::getBitsPerPixel(image->mFormat)/8;
	assert(texelSize == 4);
	unsigned int offset = 0;
	if (image->mIsCubemap)
	{
		for (int face = 0; face < cubeFace; face++)
		{
			for (auto i = 0; i < image->GetMipmapCount(); i++)
			{
				offset += (image->mWidth >> i) * (image->mWidth >> i) * texelSize;
			}
		}
	}
	for (int i = 0; i < mipmap; i++)
	{
		offset += (image->mWidth >> i) * (image->mWidth >> i) * texelSize;
	}

	uint16_t w = uint16_t(image->mWidth >> mipmap);
	uint16_t h = uint16_t(image->mHeight >> mipmap);
	
	bgfx::updateTexture2D(
		textureHandle
		, 0
		, mipmap
		, 0
		, 0
		, w
		, h
		, bgfx::copy(image->GetBits() + offset, w * h * texelSize)
	);

	if (allocTexture)
	{
		vramTextureAlloc((image->mWidth >> mipmap) * (image->mHeight >> mipmap) * (bimg::getBitsPerPixel(image->mFormat)/8));
	}

	return textureHandle;
	/*
    unsigned int targetType = (cubeFace == -1) ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
    glBindTexture(targetType, textureId);

    unsigned int inputFormat = glInputFormats[image->mFormat];
    unsigned int internalFormat = glInternalFormats[image->mFormat];
	

	

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

    glBindTexture(targetType, 0);
	
    return textureId;
	*/
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
        image->mHasMipmaps = imageContainer->m_numMips > 1;
        image->mIsCubemap = imageContainer->m_cubeMap;
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

    const int components = bimg::getBitsPerPixel(image->mFormat)/8;
    
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
        //auto texformat = image->mFormat;//GetBIMGFormat((TextureFormat::Enum)image->mFormat);

        switch (format)
        {
        case 0: // jpeg
            //bimg::imageWriteJpg(&writer, image->mWidth, image->mHeight, image->mWidth * components, image->GetBits(), texformat, false/*_yflip*/, quality, &err);
            break;
        case 1: // png
            bimg::imageWritePng(&writer, image->mWidth, image->mHeight, image->mWidth * components, image->GetBits(), image->mFormat, false/*_yflip*/, &err);
            break;
        case 2: // tga
            bimg::imageWriteTga(&writer, image->mWidth, image->mHeight, image->mWidth * components, image->GetBits(), image->mFormat, false/*_yflip*/, &err);
            break;
        case 4: // hdr
            bimg::imageWriteHdr(&writer, image->mWidth, image->mHeight, image->mWidth * components, image->GetBits(), image->mFormat, false/*_yflip*/, &err);
            break;
        case 5: // dds
            {
                bimg::ImageContainer* imageContainer = bimg::imageAlloc(
                    g_allocator
                    , image->mFormat
                    , image->mWidth
                    , image->mHeight
                    , 1
                    , 1
                    , image->mIsCubemap
                    , image->mHasMipmaps
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
                , image->mFormat
                , image->mIsCubemap
                , image->mWidth
                , image->mHeight
                , 1
                , image->GetMipmapCount()
                , 1
                , image->GetBits()
                , &err
            );
            break;
        case 7: // exr
            bimg::imageWriteExr(&writer, image->mWidth, image->mHeight, image->mWidth * components, image->GetBits(), image->mFormat, false/*_yflip*/, &err);
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

TextureHandle ImageCache::GetTexture(const std::string& filename)
{
    auto iter = mSynchronousTextureCache.find(filename);
    if (iter != mSynchronousTextureCache.end())
	{
        return iter->second;
	}

    Image image;
	TextureHandle textureHandle{0};
    if (Image::Read(filename.c_str(), &image) == EVAL_OK)
    {
		mImageSizes[filename] = std::make_pair<uint16_t, uint16_t>(uint16_t(image.mWidth), uint16_t(image.mHeight));
		textureHandle = Image::Upload(&image, {0});
        Image::Free(&image);
    }

    mSynchronousTextureCache[filename] = textureHandle;
    return textureHandle;
}

const std::pair<uint16_t, uint16_t> ImageCache::GetImageSize(const std::string& filename)
{
	auto iter = mImageSizes.find(filename);
	if (iter != mImageSizes.end())
	{
		return iter->second;
	}
	return std::make_pair<uint16_t, uint16_t>(0, 0);
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
		mImageSizes[filepath] = std::make_pair<uint16_t, uint16_t>(uint16_t(image->mWidth), uint16_t(image->mHeight));
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
	if (mFrameBuffer.idx)
	{
		//bgfx::destroy(mFrameBuffer);
	}
	mGLTexDepth = {0};
    mImage = Image();
	mGLTexID = {0};
}

void RenderTarget::Clone(const RenderTarget& other)
{
    // TODO: clone other type of render target
    InitBuffer(other.mImage.mWidth, other.mImage.mHeight, other.mGLTexDepth.idx != 0);
}

void RenderTarget::Swap(RenderTarget& other)
{
    ::Swap(mImage, other.mImage);
    ::Swap(mGLTexID, other.mGLTexID);
    ::Swap(mGLTexDepth, other.mGLTexDepth);
    ::Swap(mFrameBuffer, other.mFrameBuffer);
}

void RenderTarget::InitBuffer(int width, int height, bool depthBuffer)
{
    if ((width == mImage.mWidth) && (mImage.mHeight == height) && !mImage.mIsCubemap &&
        (!(depthBuffer ^ (mGLTexDepth.idx != 0))))
        return;
    Destroy();

    mImage.mWidth = width;
    mImage.mHeight = height;
    mImage.mHasMipmaps = false;
    mImage.mIsCubemap = false;
    mImage.mFormat = TextureFormat::RGBA8;

	if (!width || !height)
	{
		Log("Trying to init FBO with 0 sized dimension.\n");
		return;
	}
	/*mGLTexID = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::BGRA8);
	if (depthBuffer)
	{
		mGLTexDepth = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::D24F);
	}

	TextureHandle textureHandles[] = { mGLTexID , mGLTexDepth };
	
	mFrameBuffer = bgfx::createFrameBuffer(depthBuffer?2:1, textureHandles, true);
	*/
	assert(!depthBuffer);
	if (!depthBuffer)
	{
		mFrameBuffer = bgfx::createFrameBuffer(width, height, bgfx::TextureFormat::Enum(mImage.mFormat));
		mGLTexID = bgfx::getTexture(mFrameBuffer);
		bgfx::setName(mFrameBuffer, "RenderTargetBuffer");
	}
	
}

void RenderTarget::InitCube(int width, bool hasMipmaps)
{
    if ((width == mImage.mWidth) && (mImage.mHeight == width) && mImage.mIsCubemap &&
        (mImage.mHasMipmaps == hasMipmaps))
        return;
    Destroy();

    mImage.mWidth = width;
    mImage.mHeight = width;
    mImage.mHasMipmaps = hasMipmaps;
    mImage.mIsCubemap = true;
    mImage.mFormat = TextureFormat::RGBA8;

	if (!width)
	{
		Log("Trying to init FBO with 0 sized dimension.\n");
		return;
	}

	mGLTexID = bgfx::createTextureCube(width, hasMipmaps, 1, bgfx::TextureFormat::Enum(mImage.mFormat));
	mFrameBuffer = bgfx::createFrameBuffer(1, &mGLTexID, true);
}
