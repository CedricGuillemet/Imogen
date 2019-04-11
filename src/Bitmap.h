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
#include <stdlib.h>

namespace FFMPEGCodec
{
    class Decoder;
};

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

struct Image
{
    Image() : mDecoder(NULL), mBits(NULL), mDataSize(0)
    {
    }
    Image(const Image& other) : mBits(NULL), mDataSize(0)
    {
        *this = other;
    }
    ~Image()
    {
        free(mBits);
    }

    void *mDecoder;
    int mWidth, mHeight;
    uint32_t mDataSize;
    uint8_t mNumMips;
    uint8_t mNumFaces;
    uint8_t mFormat;
    Image& operator = (const Image &other)
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
    unsigned char *GetBits() const { return mBits; }
    void SetBits(unsigned char* bits, size_t size)
    {
        Allocate(size);
        memcpy(mBits, bits, size);
    }
    void Allocate(size_t size)
    {
        if (mDataSize != size)
            free(mBits);
        mBits = (unsigned char*)malloc(size);
        mDataSize = uint32_t(size);
    }
    void DoFree() {
        free(mBits); mBits = NULL; mDataSize = 0;
    }

    static int Read(const char *filename, Image *image);
    static int Free(Image *image);
    static unsigned int Upload(Image *image, unsigned int textureId, int cubeFace = -1);
    static int LoadSVG(const char *filename, Image *image, float dpi);
    static int ReadMem(unsigned char *data, size_t dataSize, Image *image);
    static void VFlip(Image *image);
    static int Write(const char *filename, Image *image, int format, int quality);
    static int EncodePng(Image *image, std::vector<unsigned char> &pngImage);
    static int CubemapFilter(Image *image, int faceSize, int lightingModel, int excludeBase, int glossScale, int glossBias);
    static Image DecodeImage(FFMPEGCodec::Decoder *decoder, int frame);

protected:
    unsigned char *mBits;
};

extern const unsigned int glInternalFormats[];
extern const unsigned int glInputFormats[];
extern const unsigned int textureFormatSize[];

struct ImageCache
{
    // synchronous texture cache
// use for simple textures(stock) or to replace with a more efficient one
    unsigned int GetTexture(const std::string& filename);

protected:
    std::map<std::string, unsigned int> mSynchronousTextureCache;
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


class RenderTarget
{

public:
    RenderTarget() : mGLTexID(0), mGLTexDepth(0), mFbo(0), mDepthBuffer(0)
    {
        memset(&mImage, 0, sizeof(Image));
    }

    void InitBuffer(int width, int height, bool depthBuffer);
    void InitCube(int width);
    void BindAsTarget() const;
    void BindAsCubeTarget() const;
    void BindCubeFace(size_t face);
    void Destroy();
    void CheckFBO();
    void Clone(const RenderTarget &other);
    void Swap(RenderTarget &other);


    Image mImage;
    unsigned int mGLTexID;
    unsigned int mGLTexDepth;
    unsigned int mDepthBuffer;
    unsigned int mFbo;
};
