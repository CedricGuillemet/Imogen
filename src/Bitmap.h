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

bimg::Quality::Enum GetQuality(int quality);

struct Image
{
    Image() : mDecoder(NULL), mWidth(0), mHeight(0), mHasMipmaps(false), mIsCubemap(false), mBits(NULL), mDataSize(0)
    {
    }
    Image(Image&& other)
    {
        mDecoder = other.mDecoder;
        mWidth = other.mWidth;
        mHeight = other.mHeight;
        mDataSize = other.mDataSize;
        mHasMipmaps = other.mHasMipmaps;
		mIsCubemap = other.mIsCubemap;
        mFormat = other.mFormat;
        mBits = other.mBits;

        other.mDecoder = 0;
        other.mWidth = 0;
        other.mHeight = 0;
        other.mDataSize = 0;
        other.mHasMipmaps = false;
        other.mIsCubemap = 0;
        other.mFormat = TextureFormat::Unknown;
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
    bool mHasMipmaps;
    bool mIsCubemap;
    TextureFormat mFormat;
    Image& operator=(const Image& other)
    {
        mDecoder = other.mDecoder;
        mWidth = other.mWidth;
        mHeight = other.mHeight;
		mHasMipmaps = other.mHasMipmaps;
		mIsCubemap = other.mIsCubemap;
        mFormat = other.mFormat;
        SetBits(other.mBits, other.mDataSize);
        return *this;
    }
	unsigned int GetMipmapCount() const 
	{
		if (!mHasMipmaps)
		{
			return 1;
		}
		return (unsigned int)(log2(mWidth));
	}
	unsigned int GetFaceCount() const
	{
		return mIsCubemap ? 6 : 1;
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
    static TextureHandle Upload(const Image* image, TextureHandle textureHandle, int cubeFace = -1, int mipmap = 0);
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
/*
extern const unsigned int glInternalFormats[];
extern const unsigned int glInputFormats[];
extern const unsigned int textureFormatSize[];
*/
struct ImageCache
{
    // synchronous texture cache
    // use for simple textures(stock) or to replace with a more efficient one
	TextureHandle GetTexture(const std::string& filename);
    Image* GetImage(const std::string& filepath);
    void AddImage(const std::string& filepath, Image* image);
	const std::pair<uint16_t, uint16_t> GetImageSize(const std::string& filename);
protected:
    std::map<std::string, TextureHandle> mSynchronousTextureCache;
    std::map<std::string, Image> mImageCache;
	std::map < std::string, std::pair<uint16_t, uint16_t> > mImageSizes;
    std::mutex mCacheAccess;
};
extern ImageCache gImageCache;

void SaveCapture(const std::string& filemane, int x, int y, int w, int h);

class RenderTarget
{
public:
    RenderTarget() = default;

    void InitBuffer(int width, int height, bool depthBuffer);
    void InitCube(int width, bool hasMipmaps);
    void Destroy();
    void Clone(const RenderTarget& other);
    void Swap(RenderTarget& other);


    Image mImage;
	TextureHandle mGLTexID = {0};
	TextureHandle mGLTexDepth = {0};
	FrameBufferHandle mFrameBuffer = { 0 };
};
