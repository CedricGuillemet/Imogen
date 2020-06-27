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

#include <algorithm>
#include <iostream>
#include <fstream>
#include "Library.h"
#include "imgui.h"
#include <string.h>
#include "JSGlue.h"
#include "Libraries.h"

int Log(const char* szFormat, ...);

const std::vector<std::string> MetaNode::mCategories = {
    "Transform", "Generator", "Material", "Blend", "Filter", "Noise", "File", "Paint", "Cubemap", "Fur", "Tools"};

enum : uint32_t
{
    v_initial,
    v_materialComment,
    v_thumbnail,
    v_nodeImage,
    v_rugs,
    v_nodeTypeName,
    v_frameStartEnd,
    v_animation,
    v_pinnedParameters,
    v_backgroundNode,
    v_pinnedIO,
    v_multiplexInput,
    v_lastVersion
};
#define ADD(_fieldAdded, _fieldName)                                                                                   \
    if (dataVersion >= _fieldAdded)                                                                                    \
    {                                                                                                                  \
        Ser(_fieldName);                                                                                               \
    }
#define ADD_LOCAL(_localAdded, _type, _localName, _defaultValue)                                                       \
    _type _localName = (_defaultValue);                                                                                \
    if (dataVersion >= (_localAdded))                                                                                  \
    { Ser(_localName));                                                              \
    }
#define REM(_fieldAdded, _fieldRemoved, _type, _fieldName, _defaultValue)                                              \
    _type _fieldName = (_defaultValue);                                                                                \
    if (dataVersion >= (_fieldAdded) && dataVersion < (_fieldRemoved))                                                 \
    {                                                                                                                  \
        Ser(_fieldName);                                                                                               \
    }
#define VERSION_IN_RANGE(_from, _to) (dataVersion >= (_from) && dataVersion < (_to))

template<bool doWrite>
struct Serialize
{
    Serialize(const char* szFilename)
    {
        fp = fopen(szFilename, doWrite ? "wb" : "rb");
        if (!fp)
        {
            Log("Unable to open file %s for %s\n", szFilename, doWrite ? "writing" : "reading");
        }
        else
        {
            Log("Open file %s for %s\n", szFilename, doWrite ? "writing" : "reading");
        }
    }

    ~Serialize()
    {
        if (fp)
            fclose(fp);
        SyncJSDirectory();
    }

    template<typename T>
    void Ser(T& data)
    {
        if (doWrite)
            fwrite(&data, sizeof(T), 1, fp);
        else
            fread(&data, sizeof(T), 1, fp);
    }

    void Ser(std::string& data)
    {
        if (doWrite)
        {
            uint32_t len = uint32_t(strlen(data.c_str())); // uint32_t(data.length());
            fwrite(&len, sizeof(uint32_t), 1, fp);
            fwrite(data.c_str(), len, 1, fp);
        }
        else
        {
            uint32_t len;
            fread(&len, sizeof(uint32_t), 1, fp);
            data.resize(len);
            fread(&data[0], len, 1, fp);
            data = std::string(data.c_str(), strlen(data.c_str()));
        }
    }

    template<typename T>
    void Ser(std::vector<T>& data)
    {
        uint32_t count = uint32_t(data.size());
        Ser(count);
        data.resize(count);
        for (auto& item : data)
        {
            Ser(&item);
        }
    }

    template<typename T>
    void SerArray(std::vector<T>& data)
    {
        uint32_t count = uint32_t(data.size());
        Ser(count);
        if (!count)
            return;
        if (doWrite)
        {
            fwrite(data.data(), count * sizeof(T), 1, fp);
        }
        else
        {
            data.resize(count);
            fread(&data[0], count * sizeof(T), 1, fp);
        }
    }

    void Ser(std::vector<uint8_t>& data)
    {
        SerArray(data);
    }

    void Ser(std::vector<uint32_t>& data)
    {
        SerArray(data);
    }

    void Ser(std::vector<int32_t>& data)
    {
        SerArray(data);
    }

    void Ser(AnimationBase* animBase)
    {
        ADD(v_animation, animBase->mFrames);
        if (doWrite)
        {
            fwrite(animBase->GetData(), animBase->GetValuesByteLength(), 1, fp);
        }
        else
        {
            animBase->Allocate(animBase->mFrames.size());
            fread(animBase->GetData(), animBase->GetValuesByteLength(), 1, fp);
        }
    }

    void Ser(AnimTrack* animTrack)
    {
        ADD(v_animation, animTrack->mNodeIndex);
        ADD(v_animation, animTrack->mParamIndex);
        ADD(v_animation, animTrack->mValueType);
        if (!doWrite)
        {
            animTrack->mAnimation = AllocateAnimation(animTrack->mValueType);
        }
        ADD(v_animation, animTrack->mAnimation);
    }

    void Ser(MultiplexInput* multiplexInput)
    {
        if (doWrite)
        {
            fwrite(multiplexInput, sizeof(MultiplexInput), 1, fp);
        }
        else
        {
            fread(multiplexInput, sizeof(MultiplexInput), 1, fp);
        }
    }
    void Ser(InputSampler* inputSampler)
    {
        ADD(v_initial, inputSampler->mWrapU);
        ADD(v_initial, inputSampler->mWrapV);
        ADD(v_initial, inputSampler->mFilterMin);
        ADD(v_initial, inputSampler->mFilterMag);
    }

    void Ser(MaterialNode* materialNode)
    {
        ADD(v_initial, materialNode->mNodeType);
        ADD(v_nodeTypeName, materialNode->mTypeName);
        ADD(v_initial, materialNode->mPosX);
        ADD(v_initial, materialNode->mPosY);
        ADD(v_initial, materialNode->mInputSamplers);
        ADD(v_initial, materialNode->mParameters);
        ADD(v_nodeImage, materialNode->mImage);
        ADD(v_frameStartEnd, materialNode->mFrameStart);
        ADD(v_frameStartEnd, materialNode->mFrameEnd);
    }

    void Ser(MaterialNodeRug* materialNodeRug)
    {
        ADD(v_rugs, materialNodeRug->mPosX);
        ADD(v_rugs, materialNodeRug->mPosY);
        ADD(v_rugs, materialNodeRug->mSizeX);
        ADD(v_rugs, materialNodeRug->mSizeY);
        ADD(v_rugs, materialNodeRug->mColor);
        ADD(v_rugs, materialNodeRug->mComment);
    }

    void Ser(MaterialConnection* materialConnection)
    {
        ADD(v_initial, materialConnection->mInputNodeIndex);
        ADD(v_initial, materialConnection->mOutputNodeIndex);
        ADD(v_initial, materialConnection->mInputSlotIndex);
        ADD(v_initial, materialConnection->mOutputSlotIndex);
    }

    void Ser(Material* material)
    {
        ADD(v_initial, material->mName);
        REM(v_materialComment, v_rugs, std::string, (material->mComment), "");
        ADD(v_initial, material->mMaterialNodes);
        ADD(v_initial, material->mMaterialConnections);
        ADD(v_thumbnail, material->mThumbnail);
        ADD(v_rugs, material->mMaterialRugs);
        ADD(v_animation, material->mAnimTrack);
        ADD(v_animation, material->mFrameMin);
        ADD(v_animation, material->mFrameMax);
        ADD(v_pinnedParameters, material->mPinnedParameters);
        ADD(v_pinnedIO, material->mPinnedIO);
        ADD(v_backgroundNode, material->mBackgroundNode);
        ADD(v_multiplexInput, material->mMultiplexInputs)
        if (!doWrite)
        {
            auto nodeCount = material->mMaterialNodes.size();
            material->mMultiplexInputs.resize(nodeCount);
            material->mPinnedParameters.resize(nodeCount);
            material->mPinnedIO.resize(nodeCount);
        }
    }

    void Ser(RecentLibraries::RecentLibrary* recentLibrary)
    {
        ADD(v_initial, recentLibrary->mName);
        ADD(v_initial, recentLibrary->mPath);
    }

    bool Ser(Library* library)
    {
        if (!fp)
            return false;
        if (doWrite)
            dataVersion = v_lastVersion - 1;
        Ser(dataVersion);
        if (dataVersion > v_lastVersion)
            return false; // no forward compatibility
        ADD(v_initial, library->mMaterials);
        return true;
    }

    bool Ser(RecentLibraries* recent)
    {
        if (!fp)
            return false;
        if (doWrite)
            dataVersion = v_lastVersion - 1;
        Ser(dataVersion);
        if (dataVersion > v_lastVersion)
            return false; // no forward compatibility
        ADD(v_initial, recent->mRecentLibraries);
        ADD(v_initial, recent->mMostRecentLibrary);
        return true;
    }

    FILE* fp;
    uint32_t dataVersion;
};

typedef Serialize<true> SerializeWrite;
typedef Serialize<false> SerializeRead;

void LoadLib(Library* library, const std::string& filename)
{
    if (!filename.size())
    {
        return;
    }
    SerializeRead loadSer(filename.c_str());
    library->mFilename = filename;
    loadSer.Ser(library);

    for (auto& material : library->mMaterials)
    {
        material.mThumbnailTextureHandle = {bgfx::kInvalidHandle};
        for (auto& node : material.mMaterialNodes)
        {
            if (loadSer.dataVersion >= v_nodeTypeName)
            {
                node.mNodeType = uint32_t(GetMetaNodeIndex(node.mTypeName));
            }
        }
    }
}

void SaveLib(Library* library, const std::string& filename)
{
    SerializeWrite(filename.c_str()).Ser(library);
}

void LoadRecent(RecentLibraries* recent, const char* szFilename)
{
    SerializeRead loadSer(szFilename);
    loadSer.Ser(recent);
}

void SaveRecent(RecentLibraries* recent, const char* szFilename)
{
    SerializeWrite(szFilename).Ser(recent);
}
