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

#include <algorithm>
#include <iostream>
#include <fstream>
#include "Library.h"
#include "imgui.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

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
    }

    ~Serialize()
    {
        if (fp)
            fclose(fp);
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
            Ser(&item);
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
        ADD(v_initial, materialNode->mType);
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

    FILE* fp;
    uint32_t dataVersion;
};

typedef Serialize<true> SerializeWrite;
typedef Serialize<false> SerializeRead;

void LoadLib(Library* library, const char* szFilename)
{
    SerializeRead loadSer(szFilename);
    loadSer.Ser(library);

    for (auto& material : library->mMaterials)
    {
        material.mThumbnailTextureId = 0;
        material.mRuntimeUniqueId = GetRuntimeId();
        for (auto& node : material.mMaterialNodes)
        {
            node.mRuntimeUniqueId = GetRuntimeId();
            if (loadSer.dataVersion >= v_nodeTypeName)
            {
                node.mType = uint32_t(GetMetaNodeIndex(node.mTypeName));
            }
        }
    }
}

void SaveLib(Library* library, const char* szFilename)
{
    SerializeWrite(szFilename).Ser(library);
}

unsigned int GetRuntimeId()
{
    static unsigned int runtimeId = 0;
    return ++runtimeId;
}

int GetParameterIndex(uint32_t nodeType, const char* parameterName)
{
    const MetaNode& currentMeta = gMetaNodes[nodeType];
    int i = 0;
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (!stricmp(param.mName.c_str(), parameterName))
            return i;
        i++;
    }
    return -1;
}

size_t GetParameterTypeSize(ConTypes paramType)
{
    switch (paramType)
    {
        case Con_Angle:
        case Con_Float:
            return sizeof(float);
        case Con_Angle2:
        case Con_Float2:
            return sizeof(float) * 2;
        case Con_Angle3:
        case Con_Float3:
            return sizeof(float) * 3;
        case Con_Angle4:
        case Con_Color4:
        case Con_Float4:
            return sizeof(float) * 4;
        case Con_Ramp:
            return sizeof(float) * 2 * 8;
        case Con_Ramp4:
            return sizeof(float) * 4 * 8;
        case Con_Enum:
        case Con_Int:
            return sizeof(int);
        case Con_Int2:
            return sizeof(int) * 2;
        case Con_FilenameRead:
        case Con_FilenameWrite:
            return 1024;
        case Con_ForceEvaluate:
            return 0;
        case Con_Bool:
            return sizeof(int);
        case Con_Camera:
            return sizeof(Camera);
        case Con_Multiplexer:
            return sizeof(int);
        default:
            assert(0);
    }
    return -1;
}

static const char* parameterNames[] = {
    "Float",        "Float2",        "Float3",        "Float4", "Color4", "Int",    "Int2",
    "Ramp",         "Angle",         "Angle2",        "Angle3", "Angle4", "Enum",   "Structure",
    "FilenameRead", "FilenameWrite", "ForceEvaluate", "Bool",   "Ramp4",  "Camera", "Multiplexer", "Any",
};

const char* GetParameterTypeName(ConTypes paramType)
{
    return parameterNames[std::min(int(paramType), int(Con_Any) - 1)];
}

ConTypes GetParameterType(const char* parameterName)
{
    for (size_t i = 0; i < Con_Any; i++)
    {
        if (!_stricmp(parameterNames[i], parameterName))
            return ConTypes(i);
    }
    return Con_Any;
}

size_t GetCurveCountPerParameterType(uint32_t paramType)
{
    switch (paramType)
    {
        case Con_Angle:
        case Con_Float:
            return 1;
        case Con_Angle2:
        case Con_Float2:
            return 2;
        case Con_Angle3:
        case Con_Float3:
            return 3;
        case Con_Angle4:
        case Con_Color4:
        case Con_Float4:
            return 4;
        case Con_Ramp:
            return 0; // sizeof(float) * 2 * 8;
        case Con_Ramp4:
            return 0; // sizeof(float) * 4 * 8;
        case Con_Enum:
            return 1;
        case Con_Int:
            return 1;
        case Con_Int2:
            return 2;
        case Con_FilenameRead:
        case Con_FilenameWrite:
            return 0;
        case Con_ForceEvaluate:
            return 0;
        case Con_Bool:
            return 1;
        case Con_Camera:
            return 7;
        case Con_Multiplexer:
            return 1;
        default:
            assert(0);
    }
    return 0;
}

const char* GetCurveParameterSuffix(uint32_t paramType, int suffixIndex)
{
    static const char* suffixes[] = {".x", ".y", ".z", ".w"};
    static const char* cameraSuffixes[] = {"posX", "posY", "posZ", "dirX", "dirY", "dirZ", "FOV"};
    switch (paramType)
    {
        case Con_Angle:
        case Con_Float:
            return "";
        case Con_Angle2:
        case Con_Float2:
            return suffixes[suffixIndex];
        case Con_Angle3:
        case Con_Float3:
            return suffixes[suffixIndex];
        case Con_Angle4:
        case Con_Color4:
        case Con_Float4:
            return suffixes[suffixIndex];
        case Con_Ramp:
            return 0; // sizeof(float) * 2 * 8;
        case Con_Ramp4:
            return 0; // sizeof(float) * 4 * 8;
        case Con_Enum:
            return "";
        case Con_Int:
            return "";
        case Con_Int2:
            return suffixes[suffixIndex];
        case Con_FilenameRead:
        case Con_FilenameWrite:
            return 0;
        case Con_ForceEvaluate:
            return 0;
        case Con_Bool:
            return "";
        case Con_Camera:
            return cameraSuffixes[suffixIndex];
        case Con_Multiplexer:
            return "";
        default:
            assert(0);
    }
    return "";
}

uint32_t GetCurveParameterColor(uint32_t paramType, int suffixIndex)
{
    static const uint32_t colors[] = {0xFF1010F0, 0xFF10F010, 0xFFF01010, 0xFFF0F0F0};
    static const uint32_t cameraColors[] = {
        0xFF1010F0, 0xFF10F010, 0xFFF01010, 0xFF1010F0, 0xFF10F010, 0xFFF01010, 0xFFF0F0F0};
    switch (paramType)
    {
        case Con_Angle:
        case Con_Float:
            return 0xFF1040F0;
        case Con_Angle2:
        case Con_Float2:
            return colors[suffixIndex];
        case Con_Angle3:
        case Con_Float3:
            return colors[suffixIndex];
        case Con_Angle4:
        case Con_Color4:
        case Con_Float4:
            return colors[suffixIndex];
        case Con_Ramp:
            return 0xFFAAAAAA; // sizeof(float) * 2 * 8;
        case Con_Ramp4:
            return 0xFFAAAAAA; // sizeof(float) * 4 * 8;
        case Con_Enum:
            return 0xFFAAAAAA;
        case Con_Int:
            return 0xFFAAAAAA;
        case Con_Int2:
            return colors[suffixIndex];
        case Con_FilenameRead:
        case Con_FilenameWrite:
            return 0;
        case Con_ForceEvaluate:
            return 0;
        case Con_Bool:
            return 0xFFF0F0F0;
        case Con_Camera:
            return cameraColors[suffixIndex];
        case Con_Multiplexer:
            return 0xFFAABBCC;
        default:
            assert(0);
    }
    return 0xFFAAAAAA;
}

size_t GetParameterOffset(uint32_t type, uint32_t parameterIndex)
{
    const MetaNode& currentMeta = gMetaNodes[type];
    size_t ret = 0;
    int i = 0;
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (i == parameterIndex)
            break;
        ret += GetParameterTypeSize(param.mType);
        i++;
    }
    return ret;
}

ConTypes GetParameterType(uint32_t nodeType, uint32_t parameterIndex)
{
    const MetaNode& currentMeta = gMetaNodes[nodeType];
    return currentMeta.mParams[parameterIndex].mType;
}

AnimationBase* AllocateAnimation(uint32_t valueType)
{
    switch (valueType)
    {
        case Con_Float:
            return new Animation<float>;
        case Con_Float2:
            return new Animation<Vec2>;
        case Con_Float3:
            return new Animation<Vec3>;
        case Con_Float4:
            return new Animation<Vec4>;
        case Con_Color4:
            return new Animation<Vec4>;
        case Con_Int:
            return new Animation<int>;
        case Con_Int2:
            return new Animation<iVec2>;
        case Con_Ramp:
            return new Animation<Vec2>;
        case Con_Angle:
            return new Animation<float>;
        case Con_Angle2:
            return new Animation<Vec2>;
        case Con_Angle3:
            return new Animation<Vec3>;
        case Con_Angle4:
            return new Animation<Vec4>;
        case Con_Enum:
            return new Animation<int>;
        case Con_Structure:
        case Con_FilenameRead:
        case Con_FilenameWrite:
        case Con_ForceEvaluate:
            return NULL;
        case Con_Bool:
            return new Animation<unsigned char>;
        case Con_Ramp4:
            return new Animation<Vec4>;
        case Con_Camera:
            return new Animation<Camera>;
        case Con_Multiplexer:
            return new Animation<int>;
    }
    return NULL;
}

CurveType GetCurveTypeForParameterType(ConTypes paramType)
{
    switch (paramType)
    {
        case Con_Float:
            return CurveSmooth;
        case Con_Float2:
            return CurveSmooth;
        case Con_Float3:
            return CurveSmooth;
        case Con_Float4:
            return CurveSmooth;
        case Con_Color4:
            return CurveSmooth;
        case Con_Int:
            return CurveLinear;
        case Con_Int2:
            return CurveLinear;
        case Con_Ramp:
            return CurveNone;
        case Con_Angle:
            return CurveSmooth;
        case Con_Angle2:
            return CurveSmooth;
        case Con_Angle3:
            return CurveSmooth;
        case Con_Angle4:
            return CurveSmooth;
        case Con_Enum:
            return CurveDiscrete;
        case Con_Structure:
        case Con_FilenameRead:
        case Con_FilenameWrite:
        case Con_ForceEvaluate:
            return CurveNone;
        case Con_Bool:
            return CurveDiscrete;
        case Con_Ramp4:
            return CurveNone;
        case Con_Camera:
            return CurveSmooth;
        case Con_Multiplexer:
            return CurveLinear;
    }
    return CurveNone;
}

void Camera::ComputeViewProjectionMatrix(float* viewProj, float* viewInverse)
{
    Mat4x4& vp = *(Mat4x4*)viewProj;
    Mat4x4 view, proj;
    view.lookAtRH(mPosition, mPosition + mDirection, Vec4(0.f, 1.f, 0.f, 0.f));
    proj.glhPerspectivef2(53.f, 1.f, 0.01f, 100.f);
    vp = view * proj;

    Mat4x4& vi = *(Mat4x4*)viewInverse;
    vi.LookAt(mPosition, mPosition + mDirection, Vec4(0.f, 1.f, 0.f, 0.f));
}

std::vector<MetaNode> gMetaNodes;
std::map<std::string, size_t> gMetaNodesIndices;

size_t GetMetaNodeIndex(const std::string& metaNodeName)
{
    auto iter = gMetaNodesIndices.find(metaNodeName.c_str());
    if (iter == gMetaNodesIndices.end())
    {
        Log("Node type %s not find in the library!\n", metaNodeName.c_str());
        return -1;
    }
    return iter->second;
}

size_t ComputeNodeParametersSize(size_t nodeType)
{
    size_t res = 0;
    for (auto& param : gMetaNodes[nodeType].mParams)
    {
        res += GetParameterTypeSize(param.mType);
    }
    return res;
}


void LoadMetaNodes(const std::vector<std::string>& metaNodeFilenames)
{
    static const uint32_t hcTransform = IM_COL32(200, 200, 200, 255);
    static const uint32_t hcGenerator = IM_COL32(150, 200, 150, 255);
    static const uint32_t hcMaterial = IM_COL32(150, 150, 200, 255);
    static const uint32_t hcBlend = IM_COL32(200, 150, 150, 255);
    static const uint32_t hcFilter = IM_COL32(200, 200, 150, 255);
    static const uint32_t hcNoise = IM_COL32(150, 250, 150, 255);
    static const uint32_t hcPaint = IM_COL32(100, 250, 180, 255);

    for (auto& filename : metaNodeFilenames)
    {
        std::vector<MetaNode> metaNodes = ReadMetaNodes(filename.c_str());
        if (metaNodes.empty())
        {
            IMessageBox("Errors while parsing nodes definitions.\nCheck logs.", "Node Parsing Error!");
            exit(-1);
        }
        gMetaNodes.insert(gMetaNodes.end(), metaNodes.begin(), metaNodes.end());
    }


    for (size_t i = 0; i < gMetaNodes.size(); i++)
    {
        gMetaNodesIndices[gMetaNodes[i].mName] = i;
    }
}

void LoadMetaNodes()
{
    std::vector<std::string> metaNodeFilenames;
    DiscoverFiles("json", "Nodes/", metaNodeFilenames);
    LoadMetaNodes(metaNodeFilenames);
}

void SaveMetaNodes(const char* filename)
{
    // write lib to json -----------------------
    rapidjson::Document d;
    d.SetObject();
    rapidjson::Document::AllocatorType& allocator = d.GetAllocator();

    rapidjson::Value nodelist(rapidjson::kArrayType);

    for (auto& node : gMetaNodes)
    {
        rapidjson::Value nodeValue;
        nodeValue.SetObject();
        nodeValue.AddMember("name", rapidjson::Value(node.mName.c_str(), allocator), allocator);
        nodeValue.AddMember("category", rapidjson::Value().SetInt(node.mCategory), allocator);
        {
            ImColor clr(node.mHeaderColor);
            rapidjson::Value colorValue(rapidjson::kArrayType);
            colorValue.PushBack(rapidjson::Value().SetFloat(clr.Value.x), allocator);
            colorValue.PushBack(rapidjson::Value().SetFloat(clr.Value.y), allocator);
            colorValue.PushBack(rapidjson::Value().SetFloat(clr.Value.z), allocator);
            colorValue.PushBack(rapidjson::Value().SetFloat(clr.Value.w), allocator);
            nodeValue.AddMember("color", colorValue, allocator);
        }

        std::vector<MetaCon>* cons[] = {&node.mInputs, &node.mOutputs};
        for (int i = 0; i < 2; i++)
        {
            auto inouts = cons[i];
            if (inouts->empty())
                continue;

            rapidjson::Value ioValue(rapidjson::kArrayType);

            for (auto& con : *inouts)
            {
                rapidjson::Value conValue;
                conValue.SetObject();
                conValue.AddMember("name", rapidjson::Value(con.mName.c_str(), allocator), allocator);
                conValue.AddMember(
                    "type", rapidjson::Value(GetParameterTypeName(ConTypes(con.mType)), allocator), allocator);
                ioValue.PushBack(conValue, allocator);
            }
            if (i)
                nodeValue.AddMember("outputs", ioValue, allocator);
            else
                nodeValue.AddMember("inputs", ioValue, allocator);
        }

        if (!node.mParams.empty())
        {
            rapidjson::Value paramValues(rapidjson::kArrayType);
            for (auto& con : node.mParams)
            {
                rapidjson::Value conValue;
                conValue.SetObject();
                conValue.AddMember("name", rapidjson::Value(con.mName.c_str(), allocator), allocator);
                conValue.AddMember("type", rapidjson::Value(GetParameterTypeName(con.mType), allocator), allocator);
                if (con.mRangeMinX > FLT_EPSILON || con.mRangeMaxX > FLT_EPSILON || con.mRangeMinY > FLT_EPSILON ||
                    con.mRangeMaxY > FLT_EPSILON)
                {
                    conValue.AddMember("rangeMinX", rapidjson::Value().SetFloat(con.mRangeMinX), allocator);
                    conValue.AddMember("rangeMaxX", rapidjson::Value().SetFloat(con.mRangeMaxX), allocator);
                    conValue.AddMember("rangeMinY", rapidjson::Value().SetFloat(con.mRangeMinY), allocator);
                    conValue.AddMember("rangeMaxY", rapidjson::Value().SetFloat(con.mRangeMaxY), allocator);
                }
                if (con.mbRelative)
                    conValue.AddMember("relative", rapidjson::Value().SetBool(con.mbRelative), allocator);
                if (con.mbQuadSelect)
                    conValue.AddMember("quadSelect", rapidjson::Value().SetBool(con.mbQuadSelect), allocator);
                if (con.mEnumList.size())
                {
                    conValue.AddMember("enum", rapidjson::Value(con.mEnumList.c_str(), allocator), allocator);
                }
                paramValues.PushBack(conValue, allocator);
            }
            nodeValue.AddMember("parameters", paramValues, allocator);
        }
        if (node.mbHasUI)
            nodeValue.AddMember("hasUI", rapidjson::Value().SetBool(node.mbHasUI), allocator);
        if (node.mbSaveTexture)
            nodeValue.AddMember("saveTexture", rapidjson::Value().SetBool(node.mbSaveTexture), allocator);

        nodelist.PushBack(nodeValue, allocator);
    }

    d.AddMember("nodes", nodelist, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);

    std::ofstream myfile;
    myfile.open(filename);
    myfile << buffer.GetString();
    myfile.close();
}

void ParseStringToParameter(const std::string& str, uint32_t parameterType, void *parameterPtr)
{
    float* pf = (float*)parameterPtr;
    int* pi = (int*)parameterPtr;
    Camera* cam = (Camera*)parameterPtr;
    ImVec2* iv2 = (ImVec2*)parameterPtr;
    ImVec4* iv4 = (ImVec4*)parameterPtr;
    switch (parameterType)
    {
        case Con_Angle:
        case Con_Float:
            sscanf(str.c_str(), "%f", pf);
            break;
        case Con_Angle2:
        case Con_Float2:
            sscanf(str.c_str(), "%f,%f", &pf[0], &pf[1]);
            break;
        case Con_Angle3:
        case Con_Float3:
            sscanf(str.c_str(), "%f,%f,%f", &pf[0], &pf[1], &pf[2]);
            break;
        case Con_Color4:
        case Con_Float4:
        case Con_Angle4:
            sscanf(str.c_str(), "%f,%f,%f,%f", &pf[0], &pf[1], &pf[2], &pf[3]);
            break;
        case Con_Multiplexer:
        case Con_Enum:
        case Con_Int:
            sscanf(str.c_str(), "%d", &pi[0]);
            break;
        case Con_Int2:
            sscanf(str.c_str(), "%d,%d", &pi[0], &pi[1]);
            break;
        case Con_Ramp:
            iv2[0] = ImVec2(0, 0);
            iv2[1] = ImVec2(1, 1);
            break;
        case Con_Ramp4:
            iv4[0] = ImVec4(0, 0, 0, 0);
            iv4[1] = ImVec4(1, 1, 1, 1);
            break;
        case Con_FilenameRead:
            strcpy((char*)parameterPtr, str.c_str());
            break;
        case Con_Structure:
        case Con_FilenameWrite:
        case Con_ForceEvaluate:
        case Con_Camera:
            cam->mDirection = Vec4(0.f, 0.f, 1.f, 0.f);
            cam->mUp = Vec4(0.f, 1.f, 0.f, 0.f);
            break;
        case Con_Bool:
            pi[0] = (str == "true") ? 1 : 0;
            break;
    }
}

std::vector<MetaNode> ReadMetaNodes(const char* filename)
{
    // read it back
    std::vector<MetaNode> serNodes;
    std::ifstream t(filename);
    if (!t.good())
    {
        Log("%s - Unable to load file.\n", filename);
        return serNodes;
    }
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    rapidjson::Document doc;
    doc.Parse(str.c_str());
    if (doc.HasParseError())
    {
        Log("Parsing error in %s\n", filename);
        return serNodes;
    }
    rapidjson::Value& nodesValue = doc["nodes"];
    for (rapidjson::SizeType i = 0; i < nodesValue.Size(); i++)
    {
        MetaNode curNode;
        rapidjson::Value& node = nodesValue[i];
        if (!node.HasMember("name"))
        {
            // error
            Log("Missing name in node %d definition (%s)\n", i, filename);
            return serNodes;
        }
        curNode.mName = node["name"].GetString();
        if (!node.HasMember("category"))
        {
            // error
            Log("Missing category in node %s definition (%s)\n", curNode.mName.c_str(), filename);
            return serNodes;
        }
        curNode.mCategory = node["category"].GetInt();

        if (node.HasMember("description"))
        {
            curNode.mDescription = node["description"].GetString();
        }

        if (node.HasMember("height"))
            curNode.mHeight = node["height"].GetInt();
        else 
            curNode.mHeight = 100;

        if (node.HasMember("width"))
            curNode.mWidth = node["width"].GetInt();
        else
            curNode.mWidth = 100;
        
		if (node.HasMember("experimental"))
            curNode.mbExperimental = node["experimental"].GetBool();
        else
            curNode.mbExperimental = false;

        if (node.HasMember("saveTexture"))

        if (node.HasMember("hasUI"))
            curNode.mbHasUI = node["hasUI"].GetBool();
        else
            curNode.mbHasUI = false;
        if (node.HasMember("saveTexture"))
            curNode.mbSaveTexture = node["saveTexture"].GetBool();
        else
            curNode.mbSaveTexture = false;

        if (!node.HasMember("color"))
        {
            // error
            Log("Missing color in node %s definition (%s)\n", curNode.mName.c_str(), filename);
            return serNodes;
        }
        rapidjson::Value& color = node["color"];
        float c[4];
        if (color.Size() != 4)
        {
            // error
            Log("wrong color component count in node %s definition (%s)\n", curNode.mName.c_str(), filename);
            return serNodes;
        }
        for (rapidjson::SizeType i = 0; i < color.Size(); i++)
        {
            c[i] = color[i].GetFloat();
        }
        curNode.mHeaderColor = ImColor(c[0], c[1], c[2], c[3]).operator ImU32();
        // inputs/outputs
        if (node.HasMember("inputs"))
        {
            rapidjson::Value& inputs = node["inputs"];
            for (rapidjson::SizeType i = 0; i < inputs.Size(); i++)
            {
                MetaCon metaNode;
                if (!inputs[i].HasMember("name") || !inputs[i].HasMember("type"))
                {
                    // error
                    Log("Missing name or type in inputs for node %s definition (%s)\n",
                        curNode.mName.c_str(),
                        filename);
                    return serNodes;
                }

                metaNode.mName = inputs[i]["name"].GetString();
                metaNode.mType = GetParameterType(inputs[i]["type"].GetString());
                if (metaNode.mType == Con_Any)
                {
                    // error
                    Log("Wrong type for %s in inputs for node %s definition (%s)\n",
                        metaNode.mName.c_str(),
                        curNode.mName.c_str(),
                        filename);
                    return serNodes;
                }
                curNode.mInputs.emplace_back(metaNode);
            }
        }
        if (node.HasMember("outputs"))
        {
            rapidjson::Value& outputs = node["outputs"];
            for (rapidjson::SizeType i = 0; i < outputs.Size(); i++)
            {
                MetaCon metaNode;
                if (!outputs[i].HasMember("name") || !outputs[i].HasMember("type"))
                {
                    // error
                    Log("Missing name or type in outputs for node %s definition (%s)\n",
                        curNode.mName.c_str(),
                        filename);
                    return serNodes;
                }
                metaNode.mName = outputs[i]["name"].GetString();
                metaNode.mType = GetParameterType(outputs[i]["type"].GetString());
                if (metaNode.mType == Con_Any)
                {
                    // error
                    Log("Wrong type for %s in outputs for node %s definition (%s)\n",
                        metaNode.mName.c_str(),
                        curNode.mName.c_str(),
                        filename);
                    return serNodes;
                }
                curNode.mOutputs.emplace_back(metaNode);
            }
        }
        // parameters
        if (node.HasMember("parameters"))
        {
            rapidjson::Value& params = node["parameters"];
            for (rapidjson::SizeType i = 0; i < params.Size(); i++)
            {
                MetaParameter metaParam;
                rapidjson::Value& param = params[i];
                if (!param.HasMember("name") || !param.HasMember("type"))
                {
                    // error
                    Log("Missing name or type in parameters for node %s definition (%s)\n",
                        curNode.mName.c_str(),
                        filename);
                    return serNodes;
                }
                metaParam.mName = param["name"].GetString();
                metaParam.mType = GetParameterType(param["type"].GetString());
                if (metaParam.mType == Con_Any)
                {
                    // error
                    Log("Wrong type for %s in parameters for node %s definition (%s)\n",
                        metaParam.mName.c_str(),
                        curNode.mName.c_str(),
                        filename);
                    return serNodes;
                }

                if (param.HasMember("rangeMinX") && param.HasMember("rangeMaxX") && param.HasMember("rangeMinY") &&
                    param.HasMember("rangeMaxY"))
                {
                    metaParam.mRangeMinX = param["rangeMinX"].GetFloat();
                    metaParam.mRangeMaxX = param["rangeMaxX"].GetFloat();
                    metaParam.mRangeMinY = param["rangeMinY"].GetFloat();
                    metaParam.mRangeMaxY = param["rangeMaxY"].GetFloat();
                }
                else
                {
                    metaParam.mRangeMinX = metaParam.mRangeMinY = metaParam.mRangeMaxX = metaParam.mRangeMaxY = 0.f;
                }
                if (param.HasMember("description"))
                {
                    metaParam.mDescription = param["description"].GetString();
                }

                if (param.HasMember("loop"))
                    metaParam.mbLoop = param["loop"].GetBool();
                else
                    metaParam.mbLoop = true;
                if (param.HasMember("relative"))
                    metaParam.mbRelative = param["relative"].GetBool();
                else
                    metaParam.mbRelative = false;
                if (param.HasMember("quadSelect"))
                    metaParam.mbQuadSelect = param["quadSelect"].GetBool();
                else
                    metaParam.mbQuadSelect = false;
                if (param.HasMember("enum"))
                {
                    if (metaParam.mType != Con_Enum)
                    {
                        // error
                        Log("Mismatch type for enumerator in parameter %s for node %s definition (%s)\n",
                            metaParam.mName.c_str(),
                            curNode.mName.c_str(),
                            filename);
                        return serNodes;
                    }
                    std::string enumStr = param["enum"].GetString();
                    if (enumStr.size())
                    {
                        metaParam.mEnumList = enumStr;
                        if (metaParam.mEnumList.back() != '|')
                            metaParam.mEnumList += '|';
                    }
                    else
                    {
                        // error
                        Log("Empty string for enumerator in parameter %s for node %s definition (%s)\n",
                            metaParam.mName.c_str(),
                            curNode.mName.c_str(),
                            filename);
                        return serNodes;
                    }
                }
                if (param.HasMember("default"))
                {
                    size_t paramSize = GetParameterTypeSize(metaParam.mType);
                    metaParam.mDefaultValue.resize(paramSize);
                    std::string defaultStr = param["default"].GetString();
                    ParseStringToParameter(defaultStr, metaParam.mType, metaParam.mDefaultValue.data());
                }

                curNode.mParams.emplace_back(metaParam);
            }
        }

        serNodes.emplace_back(curNode);
    }
    return serNodes;
}

AnimationBase::AnimationPointer AnimationBase::GetPointer(int32_t frame, bool bSetting) const
{
    if (mFrames.empty())
        return {bSetting ? -1 : 0, 0, 0, 0, 0.f};
    if (frame <= mFrames[0])
        return {bSetting ? -1 : 0, 0, 0, 0, 0.f};
    if (frame > mFrames.back())
    {
        int32_t last = int32_t(mFrames.size() - (bSetting ? 0 : 1));
        return {last, mFrames.back(), last, mFrames.back(), 0.f};
    }
    for (int i = 0; i < int(mFrames.size()) - 1; i++)
    {
        if (mFrames[i] <= frame && mFrames[i + 1] >= frame)
        {
            float ratio = float(frame - mFrames[i]) / float(mFrames[i + 1] - mFrames[i]);
            return {i, mFrames[i], i + 1, mFrames[i + 1], ratio};
        }
    }
    assert(0);
    return {bSetting ? -1 : 0, 0, 0, 0, 0.f};
}

AnimTrack& AnimTrack::operator=(const AnimTrack& other)
{
    mNodeIndex = other.mNodeIndex;
    mParamIndex = other.mParamIndex;
    mValueType = other.mValueType;
    delete mAnimation;
    mAnimation = AllocateAnimation(mValueType);
    mAnimation->Copy(other.mAnimation);
    return *this;
}