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
#include <string>
#include <map>
#include <vector>
#include <string.h>
#include <mutex>
#include <memory>
#include <stdlib.h>
#include "Mem.h"
#include <bimg/encode.h>

#if USE_FFMPEG
namespace FFMPEGCodec
{
    class Decoder;
};
#endif
struct TextureFormat
{
    enum Enum
    {
        BGR8,
        RGB8,
        RGB16,
        RGB16F,
        RGB32F,
        RGBE,

        BGRA8,
        RGBA8,
        RGBA16,
        RGBA16F,
        RGBA32F,

        RGBM,

        Count,
        Null = -1,
    };
};

bimg::TextureFormat::Enum GetBIMGFormat(TextureFormat::Enum format);
bimg::Quality::Enum GetQuality(int quality);

struct Image
{
    Image() : mDecoder(NULL), mWidth(0), mHeight(0), mNumMips(0), mNumFaces(0), mBits(NULL), mDataSize(0)
    {
    }
    Image(Image&& other)
    {
        mDecoder = other.mDecoder;
        mWidth = other.mWidth;
        mHeight = other.mHeight;
        mDataSize = other.mDataSize;
        mNumMips = other.mNumMips;
        mNumFaces = other.mNumFaces;
        mFormat = other.mFormat;
        mBits = other.mBits;

        other.mDecoder = 0;
        other.mWidth = 0;
        other.mHeight = 0;
        other.mDataSize = 0;
        other.mNumMips = 0;
        other.mNumFaces = 0;
        other.mFormat = 0;
        other.mBits = 0;
    }

    Image(const Image& other) : mBits(NULL), mDataSize(0)
    {
        *this = other;
    }
    ~Image()
    {
        free(mBits);
    }

    void* mDecoder;
    int mWidth, mHeight;
    uint32_t mDataSize;
    uint8_t mNumMips;
    uint8_t mNumFaces;
    uint8_t mFormat;
    Image& operator=(const Image& other)
    {
        mDecoder = other.mDecoder;
        mWidth = other.mWidth;
        mHeight = other.mHeight;
        mNumMips = other.mNumMips;
        mNumFaces = other.mNumFaces;
        mFormat = other.mFormat;
        SetBits(other.mBits, other.mDataSize);
        return *this;
    }
    unsigned char* GetBits() const
    {
        return mBits;
    }
    void SetBits(unsigned char* bits, size_t size)
    {
        Allocate(size);
        memcpy(mBits, bits, size);
    }
    void Allocate(size_t size)
    {
        if (mBits && mDataSize != size)
        {
            imageFree(mBits);
        }
        if (size)
        {
            mBits = (unsigned char*)imageMalloc(size);
        }
        mDataSize = uint32_t(size);
    }
    void DoFree()
    {
        imageFree(mBits);
        mBits = NULL;
        mDataSize = 0;
    }

    static int Read(const char* filename, Image* image);
    static int Free(Image* image);
    static unsigned int Upload(const Image* image, unsigned int textureId, int cubeFace = -1, int mipmap = 0);
    static int LoadSVG(const char* filename, Image* image, float dpi);
    static int ReadMem(unsigned char* data, size_t dataSize, Image* image, const char* filename = nullptr);
    static void VFlip(Image* image);
    static int Write(const char* filename, Image* image, int format, int quality);
    static int EncodePng(Image* image, std::vector<unsigned char>& pngImage);
#if USE_FFMPEG
    static Image DecodeImage(FFMPEGCodec::Decoder* decoder, int frame);
#endif
protected:
    unsigned char* mBits;
};

extern const unsigned int glInternalFormats[];
extern const unsigned int glInputFormats[];
extern const unsigned int textureFormatSize[];

struct ImageCache
{
    // synchronous texture cache
    // use for simple textures(stock) or to replace with a more efficient one
    unsigned int GetTexture(const std::string& filename);
    Image* GetImage(const std::string& filepath);
    void AddImage(const std::string& filepath, Image* image);

protected:
    std::map<std::string, unsigned int> mSynchronousTextureCache;
    std::map<std::string, Image> mImageCache;
    std::mutex mCacheAccess;
};
extern ImageCache gImageCache;

struct DefaultShaders
{
    // ui callback shaders
    unsigned int mProgressShader;
    unsigned int mDisplayCubemapShader;
    // error shader
    unsigned int mNodeErrorShader;

    void Init();
};

extern DefaultShaders gDefaultShader;
void SaveCapture(const std::string& filemane, int x, int y, int w, int h);

class RenderTarget
{
public:
    RenderTarget() : mGLTexID(0), mGLTexDepth(0), mFbo(0), mDepthBuffer(0)
    {
        mImage = std::make_shared<Image>();
    }

    void InitBuffer(int width, int height, bool depthBuffer);
    void InitCube(int width, int mipmapCount);
    void BindAsTarget() const;
    void BindAsCubeTarget() const;
    void BindCubeFace(size_t face, int mipmap, int faceWidth);
    void Destroy();
    void CheckFBO();
    void Clone(const RenderTarget& other);
    void Swap(RenderTarget& other);


    std::shared_ptr<Image> mImage;
    unsigned int mGLTexID;
    unsigned int mGLTexDepth;
    unsigned int mDepthBuffer;
    unsigned int mFbo;
};
