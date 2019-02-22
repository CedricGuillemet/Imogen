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
#include <fstream>
#include "Bitmap.h"
#include "Utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define NANOSVG_ALL_COLOR_KEYWORDS	// Include full list of color keywords.
#define NANOSVG_IMPLEMENTATION	// Expands implementation
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#include "cmft/image.h"
#include "cmft/cubemapfilter.h"
#include "cmft/print.h"
#include "ffmpegCodec.h"

extern cmft::ClContext* clContext;
ImageCache gImageCache;
DefaultShaders gDefaultShader;

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
const unsigned int glCubeFace[] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};
const unsigned int textureFormatSize[] = { 3,3,6,6,12, 4,4,4,8,8,16,4 };
const unsigned int textureComponentCount[] = { 3,3,3,3,3, 4,4,4,4,4,4,4 };

Image Image::DecodeImage(FFMPEGCodec::Decoder *decoder, int frame)
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

int Image::LoadSVG(const char *filename, Image *image, float dpi)
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

    Image::VFlip(image);
    nsvgDelete(svgImage);
    nsvgDeleteRasterizer(rast);
    return EVAL_OK;
}

int Image::Read(const char *filename, Image *image)
{
    FILE * fp = fopen(filename, "rb");
    if (!fp)
        return EVAL_ERR;
    fclose(fp);

    int components;
    unsigned char *bits = stbi_load(filename, &image->mWidth, &image->mHeight, &components, 0);
    if (!bits)
    {
        cmft::Image img;
        if (!cmft::imageLoad(img, filename))
        {
            // TODO
            //auto decoder = gNodeDelegate.mEvaluationStages.FindDecoder(filename);
            //*image = Image::DecodeImage(decoder, gEvaluationTime);
            return EVAL_OK;
        }
        cmft::imageTransformUseMacroInstead(&img, cmft::IMAGE_OP_FLIP_X, UINT32_MAX);
        image->SetBits((unsigned char*)img.m_data, img.m_dataSize);
        image->mWidth = img.m_width;
        image->mHeight = img.m_height;
        image->mNumMips = img.m_numMips;
        image->mNumFaces = img.m_numFaces;
        image->mFormat = img.m_format;
        image->mDecoder = NULL;
        return EVAL_OK;
    }

    image->SetBits(bits, image->mWidth * image->mHeight * components);
    image->mNumMips = 1;
    image->mNumFaces = 1;
    image->mFormat = (components == 3) ? TextureFormat::RGB8 : TextureFormat::RGBA8;
    image->mDecoder = NULL;
    return EVAL_OK;
}

int Image::Free(Image *image)
{
    image->DoFree();
    return EVAL_OK;
}

unsigned int Image::Upload(Image *image, unsigned int textureId, int cubeFace)
{
    if (!textureId)
        glGenTextures(1, &textureId);

    unsigned int targetType = (cubeFace == -1) ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
    glBindTexture(targetType, textureId);

    unsigned int inputFormat = glInputFormats[image->mFormat];
    unsigned int internalFormat = glInternalFormats[image->mFormat];
    glTexImage2D((cubeFace == -1) ? GL_TEXTURE_2D : glCubeFace[cubeFace], 0, internalFormat, image->mWidth, image->mHeight, 0, inputFormat, GL_UNSIGNED_BYTE, image->GetBits());
    TexParam(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, targetType);

    glBindTexture(targetType, 0);
    return textureId;
}

int Image::ReadMem(unsigned char *data, size_t dataSize, Image *image)
{
    int components;
    unsigned char *bits = stbi_load_from_memory(data, int(dataSize), &image->mWidth, &image->mHeight, &components, 0);
    if (!bits)
        return EVAL_ERR;
    image->SetBits(bits, image->mWidth * image->mHeight * components);
    return EVAL_OK;
}

void Image::VFlip(Image *image)
{
    int pixelSize = (image->mFormat == TextureFormat::RGB8) ? 3 : 4;
    int stride = image->mWidth * pixelSize;
    for (int y = 0; y < image->mHeight / 2; y++)
    {
        for (int x = 0; x < stride; x++)
        {
            unsigned char * p1 = &image->GetBits()[y * stride + x];
            unsigned char * p2 = &image->GetBits()[(image->mHeight - 1 - y) * stride + x];
            Swap(*p1, *p2);
        }
    }
}

int Image::Write(const char *filename, Image *image, int format, int quality)
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
        image->SetBits((unsigned char*)img.m_data, img.m_dataSize);
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
        // TODO
        /*FFMPEGCodec::Encoder *encoder = gCurrentContext->GetEncoder(std::string(filename), image->mWidth, image->mHeight);
        std::string fn(filename);
        encoder->AddFrame(image->GetBits(), image->mWidth, image->mHeight);*/
    }
    break;
    }
    return EVAL_OK;
}

int Image::EncodePng(Image *image, std::vector<unsigned char> &pngImage)
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

int Image::CubemapFilter(Image *image, int faceSize, int lightingModel, int excludeBase, int glossScale, int glossBias)
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

    image->SetBits((unsigned char*)img.m_data, img.m_dataSize);
    image->mNumMips = img.m_numMips;
    image->mNumFaces = img.m_numFaces;
    image->mWidth = img.m_width;
    image->mHeight = img.m_height;
    image->mFormat = img.m_format;
    return EVAL_OK;
}

void DefaultShaders::Init()
{
    std::ifstream prgStr("Stock/ProgressingNode.glsl");
    std::ifstream cubStr("Stock/DisplayCubemap.glsl");
    std::ifstream nodeErrStr("Stock/NodeError.glsl");

    mProgressShader = prgStr.good() ? LoadShader(std::string(std::istreambuf_iterator<char>(prgStr), std::istreambuf_iterator<char>()), "progressShader") : 0;
    mDisplayCubemapShader = cubStr.good() ? LoadShader(std::string(std::istreambuf_iterator<char>(cubStr), std::istreambuf_iterator<char>()), "cubeDisplay") : 0;
    mNodeErrorShader = nodeErrStr.good() ? LoadShader(std::string(std::istreambuf_iterator<char>(nodeErrStr), std::istreambuf_iterator<char>()), "nodeError") : 0;
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

