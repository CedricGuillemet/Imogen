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

#include <map>
#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"

#include "MetaNodes.h"
#include "Utils.h"
#include "Cam.h"

std::vector<MetaNode> gMetaNodes;
std::map<std::string, size_t> gMetaNodesIndices;

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
        if (!strcmp(parameterNames[i], parameterName))
            return ConTypes(i);
    }
    return Con_Any;
}


ConTypes GetParameterType(uint32_t nodeType, uint32_t parameterIndex)
{
    const MetaNode& currentMeta = gMetaNodes[nodeType];
    return currentMeta.mParams[parameterIndex].mType;
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

        curNode.mbHasUI = false;
        if (node.HasMember("hasUI"))
        {
            curNode.mbHasUI = node["hasUI"].GetBool();
        }

        curNode.mbThumbnail = true;
        if (node.HasMember("thumbnail"))
        {
            curNode.mbThumbnail = node["thumbnail"].GetBool();
        }

        if (node.HasMember("saveTexture"))
        {
            curNode.mbSaveTexture = node["saveTexture"].GetBool();
        }
        else
        {
            curNode.mbSaveTexture = false;
        }

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
        curNode.mHeaderColor = ColorF32(c[0], c[1], c[2], c[3]);
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
                metaParam.mControlType = Control_NumericEdit;
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
                if (param.HasMember("control"))
                {
                    const char* control = param["control"].GetString();
                    if (!strcmp(control, "Slider"))
                    {
                        metaParam.mControlType = Control_Slider;
                        metaParam.mSliderMinX = 0.f;
                        metaParam.mSliderMaxX = 1.f;
                        if (param.HasMember("sliderMinX"))
                        {
                            metaParam.mSliderMinX = param["sliderMinX"].GetFloat();
                        }
                        if (param.HasMember("sliderMaxX"))
                        {
                            metaParam.mSliderMaxX = param["sliderMaxX"].GetFloat();
                        }
                    }
                }

                if (param.HasMember("loop"))
                {
                    metaParam.mbLoop = param["loop"].GetBool();
                }
                else
                {
                    metaParam.mbLoop = true;
                }

                metaParam.mbHidden = false;
                if (param.HasMember("hidden"))
                {
                    metaParam.mbHidden = param["hidden"].GetBool();
                }

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

void LoadMetaNodes(const std::vector<std::string>& metaNodeFilenames)
{
    static const uint32_t hcTransform = ColorU8(200, 200, 200, 255);
    static const uint32_t hcGenerator = ColorU8(150, 200, 150, 255);
    static const uint32_t hcMaterial = ColorU8(150, 150, 200, 255);
    static const uint32_t hcBlend = ColorU8(200, 150, 150, 255);
    static const uint32_t hcFilter = ColorU8(200, 200, 150, 255);
    static const uint32_t hcNoise = ColorU8(150, 250, 150, 255);
    static const uint32_t hcPaint = ColorU8(100, 250, 180, 255);

    for (auto& filename : metaNodeFilenames)
    {
        std::vector<MetaNode> metaNodes = ReadMetaNodes(filename.c_str());
        if (metaNodes.empty())
        {
            IMessageBox("Errors while parsing nodes definitions.\nCheck logs.", "Node Parsing Error!");
            //exit(-1);
            continue;
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

#if 0
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

        std::vector<MetaCon>* cons[] = { &node.mInputs, &node.mOutputs };
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
#endif


void ParseStringToParameter(const std::string& str, ConTypes parameterType, void* parameterPtr)
{
    float* pf = (float*)parameterPtr;
    int* pi = (int*)parameterPtr;
    Camera* cam = (Camera*)parameterPtr;
    iVec2* iv2 = (iVec2*)parameterPtr;
    iVec4* iv4 = (iVec4*)parameterPtr;
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
        iv2[0] = {0, 0};
        iv2[1] = {1, 1};
        break;
    case Con_Ramp4:
        iv4[0] = {0, 0, 0, 0};
        iv4[1] = {1, 1, 1, 1};
        break;
    case Con_FilenameWrite:
    case Con_FilenameRead:
        strcpy((char*)parameterPtr, str.c_str());
        break;
    case Con_Structure:
    case Con_ForceEvaluate:
        break;
    case Con_Camera:
        cam->mDirection = Vec4(0.f, 0.f, 1.f, 0.f);
        cam->mUp = Vec4(0.f, 1.f, 0.f, 0.f);
        break;
    case Con_Bool:
        pi[0] = (str == "true") ? 1 : 0;
        break;
    }
    if (parameterType >= Con_Angle && parameterType <= Con_Angle4)
    {
        for (int i = 0; i <= (parameterType - Con_Angle); i++)
        {
            pf[i] = DegToRad(pf[i]);
        }
    }
}


int GetParameterIndex(uint32_t nodeType, const char* parameterName)
{
    const MetaNode& currentMeta = gMetaNodes[nodeType];
    int i = 0;
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (!strcmp(param.mName.c_str(), parameterName))
            return i;
        i++;
    }
    return -1;
}


size_t GetCurveCountPerParameterType(ConTypes paramType)
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

const char* GetCurveParameterSuffix(ConTypes paramType, int suffixIndex)
{
    static const char* suffixes[] = { ".x", ".y", ".z", ".w" };
    static const char* cameraSuffixes[] = { "posX", "posY", "posZ", "dirX", "dirY", "dirZ", "FOV" };
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

uint32_t GetCurveParameterColor(ConTypes paramType, int suffixIndex)
{
    static const uint32_t colors[] = { 0xFF1010F0, 0xFF10F010, 0xFFF01010, 0xFFF0F0F0 };
    static const uint32_t cameraColors[] = {
        0xFF1010F0, 0xFF10F010, 0xFFF01010, 0xFF1010F0, 0xFF10F010, 0xFFF01010, 0xFFF0F0F0 };
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
    default:
        return CurveNone;
    }
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