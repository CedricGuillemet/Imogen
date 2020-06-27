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
#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <string>
#include <map>
#include <memory>
#include "Utils.h"
#include <assert.h>
#include "Types.h"
#include "MetaNodes.h"

// used to retrieve structure in library. left is index. right is uniqueId
// if item at index doesn't correspond to uniqueid, then a search is done
// based on the unique id
typedef std::pair<size_t, RuntimeId> ASyncId;
template<typename T>
T* GetByAsyncId(ASyncId id, std::vector<T>& items)
{
    if (items.size() > id.first && items[id.first].mRuntimeUniqueId == id.second)
    {
        return &items[id.first];
    }
    else
    {
        // find identifier
        for (auto& item : items)
        {
            if (item.mRuntimeUniqueId == id.second)
            {
                return &item;
            }
        }
    }
    return NULL;
}

struct MaterialNode
{
    uint32_t mNodeType;
    std::string mTypeName;
    int32_t mPosX;
    int32_t mPosY;
    std::vector<InputSampler> mInputSamplers;
    std::vector<uint8_t> mParameters;
    std::vector<uint8_t> mImage;

    uint32_t mFrameStart;
    uint32_t mFrameEnd;
    // runtime
	RuntimeId mRuntimeUniqueId;
};

struct MaterialNodeRug
{
    int32_t mPosX;
    int32_t mPosY;
    int32_t mSizeX;
    int32_t mSizeY;
    uint32_t mColor;
    std::string mComment;
};

struct MaterialConnection
{
    uint32_t mInputNodeIndex;
    uint32_t mOutputNodeIndex;
    uint8_t mInputSlotIndex;
    uint8_t mOutputSlotIndex;
};

struct Material
{
    std::string mName;
    std::string mComment;
    std::vector<MaterialNode> mMaterialNodes;
    std::vector<MaterialNodeRug> mMaterialRugs;
    std::vector<MaterialConnection> mMaterialConnections;
    std::vector<uint8_t> mThumbnail;

    int mFrameMin, mFrameMax;

    std::vector<uint32_t> mPinnedParameters;
    std::vector<uint32_t> mPinnedIO;
    std::vector<MultiplexInput> mMultiplexInputs;

    MaterialNode* Get(ASyncId id)
    {
        return GetByAsyncId(id, mMaterialNodes);
    }

    uint32_t mBackgroundNode;

    // run time
    bgfx::TextureHandle mThumbnailTextureHandle;
	RuntimeId mRuntimeUniqueId;
};

struct Library
{
    std::string mFilename; // set when loading
    std::vector<Material> mMaterials;
    Material* Get(ASyncId id)
    {
        return GetByAsyncId(id, mMaterials);
    }
    Material* GetByName(const char* materialName)
    {
        for (auto& material : mMaterials)
        {
            if (material.mName == materialName)
            {
                return &material;
            }
        }
        return nullptr;
    }
};

void LoadLib(Library* library, const std::string& filename);
void SaveLib(Library* library, const std::string& filename);

extern Library library;
struct RecentLibraries;
void LoadRecent(RecentLibraries* recent, const char* szFilename);
void SaveRecent(RecentLibraries* recent, const char* szFilename);
