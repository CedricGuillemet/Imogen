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

#include "Library.h"
#include "imgui.h"

int Log(const char *szFormat, ...);

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
    v_lastVersion
};
#define ADD(_fieldAdded, _fieldName) if (dataVersion >= _fieldAdded){ Ser(_fieldName); }
#define ADD_LOCAL(_localAdded, _type, _localName, _defaultValue) \
    _type _localName = (_defaultValue); \
    if (dataVersion >= (_localAdded)) { Ser(_localName)); }
#define REM(_fieldAdded, _fieldRemoved, _type, _fieldName, _defaultValue) \
    _type _fieldName = (_defaultValue); \
    if (dataVersion >= (_fieldAdded) && dataVersion < (_fieldRemoved)) { Ser(_fieldName); }
#define VERSION_IN_RANGE(_from, _to) \
    (dataVersion >= (_from) && dataVersion < (_to))

template<bool doWrite> struct Serialize
{
    Serialize(const char *szFilename)
    {
        fp = fopen(szFilename, doWrite ? "wb" : "rb");
    }
    ~Serialize()
    {
        if (fp)
            fclose(fp);
    }
    template<typename T> void Ser(T& data)
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
            uint32_t len = uint32_t(data.length() + 1);
            fwrite(&len, sizeof(uint32_t), 1, fp);
            fwrite(data.c_str(), len, 1, fp);
        }
        else
        {
            uint32_t len;
            fread(&len, sizeof(uint32_t), 1, fp);
            data.resize(len);
            fread(&data[0], len, 1, fp);
        }
    }
    template<typename T> void Ser(std::vector<T>& data)
    {
        uint32_t count = uint32_t(data.size());
        Ser(count);
        data.resize(count);
        for (auto& item : data)
            Ser(&item);
    }
    template<typename T> void SerArray(std::vector<T>& data)
    {
        uint32_t count = uint32_t(data.size());
        Ser(count);
        if (!count)
            return;
        if (doWrite)
        {
            fwrite(data.data(), count*sizeof(T), 1, fp);
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

    void Ser(AnimationBase *animBase)
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

    void Ser(AnimTrack *animTrack)
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

    void Ser(InputSampler *inputSampler)
    {
        ADD(v_initial, inputSampler->mWrapU);
        ADD(v_initial, inputSampler->mWrapV);
        ADD(v_initial, inputSampler->mFilterMin);
        ADD(v_initial, inputSampler->mFilterMag);
    }
    void Ser(MaterialNode *materialNode)
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
    void Ser(MaterialNodeRug *materialNodeRug)
    {
        ADD(v_rugs, materialNodeRug->mPosX);
        ADD(v_rugs, materialNodeRug->mPosY);
        ADD(v_rugs, materialNodeRug->mSizeX);
        ADD(v_rugs, materialNodeRug->mSizeY);
        ADD(v_rugs, materialNodeRug->mColor);
        ADD(v_rugs, materialNodeRug->mComment);
    }
    void Ser(MaterialConnection *materialConnection)
    {
        ADD(v_initial, materialConnection->mInputNode);
        ADD(v_initial, materialConnection->mOutputNode);
        ADD(v_initial, materialConnection->mInputSlot);
        ADD(v_initial, materialConnection->mOutputSlot);
    }
    void Ser(Material *material)
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
    }
    bool Ser(Library *library)
    {
        if (!fp)
            return false;
        if (doWrite)
            dataVersion = v_lastVersion-1;
        Ser(dataVersion);
        if (dataVersion > v_lastVersion)
            return false; // no forward compatibility
        ADD(v_initial, library->mMaterials);
        return true;
    }
    FILE *fp;
    uint32_t dataVersion;
};

typedef Serialize<true> SerializeWrite;
typedef Serialize<false> SerializeRead;

void LoadLib(Library *library, const char *szFilename)
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

void SaveLib(Library *library, const char *szFilename)
{
    SerializeWrite(szFilename).Ser(library);
}

unsigned int GetRuntimeId()
{
    static unsigned int runtimeId = 0;
    return ++runtimeId;
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
    default:
        assert(0);
    }
    return -1;
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
        return 0;// sizeof(float) * 2 * 8;
    case Con_Ramp4:
        return 0;// sizeof(float) * 4 * 8;
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
    default:
        assert(0);
    }
    return 0;
}

const char* GetCurveParameterSuffix(uint32_t paramType, int suffixIndex)
{
    static const char* suffixes[] = { ".x",".y",".z",".w" };
    static const char* cameraSuffixes[] = { "posX","posY","posZ", "dirX", "dirY", "dirZ", "FOV" };
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
        return 0;// sizeof(float) * 2 * 8;
    case Con_Ramp4:
        return 0;// sizeof(float) * 4 * 8;
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
    default:
        assert(0);
    }
    return "";
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

AnimationBase *AllocateAnimation(uint32_t valueType)
{
    switch (valueType)
    {
    case Con_Float: return new Animation<float>;
    case Con_Float2: return new Animation<Vec2>;
    case Con_Float3: return new Animation<Vec3>;
    case Con_Float4: return new Animation<Vec4>;
    case Con_Color4: return new Animation<Vec4>;
    case Con_Int: return new Animation<int>;
    case Con_Int2: return new Animation<iVec2>;
    case Con_Ramp: return new Animation<Vec2>;
    case Con_Angle: return new Animation<float>;
    case Con_Angle2: return new Animation<Vec2>;
    case Con_Angle3: return new Animation<Vec3>;
    case Con_Angle4: return new Animation<Vec4>;
    case Con_Enum: return new Animation<int>;
    case Con_Structure:
    case Con_FilenameRead:
    case Con_FilenameWrite:
    case Con_ForceEvaluate:
        return NULL;
    case Con_Bool: return new Animation<unsigned char>;
    case Con_Ramp4: return new Animation<Vec4>;
    case Con_Camera: return new Animation<Camera>;
    }
    return NULL;
}

CurveType GetCurveTypeForParameterType(ConTypes paramType)
{
    switch (paramType)
    {
    case Con_Float: return CurveSmooth;
    case Con_Float2: return CurveSmooth;
    case Con_Float3: return CurveSmooth;
    case Con_Float4: return CurveSmooth;
    case Con_Color4: return CurveSmooth;
    case Con_Int: return CurveLinear;
    case Con_Int2: return CurveLinear;
    case Con_Ramp: return CurveNone;
    case Con_Angle: return CurveSmooth;
    case Con_Angle2: return CurveSmooth;
    case Con_Angle3: return CurveSmooth;
    case Con_Angle4: return CurveSmooth;
    case Con_Enum: return CurveDiscrete;
    case Con_Structure:
    case Con_FilenameRead:
    case Con_FilenameWrite:
    case Con_ForceEvaluate:
        return CurveNone;
    case Con_Bool: return CurveDiscrete;
    case Con_Ramp4: return CurveNone;
    case Con_Camera: return CurveSmooth;
    }
    return CurveNone;
}

void Camera::ComputeViewProjectionMatrix(float *viewProj, float *viewInverse)
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
void LoadMetaNodes()
{
    static const uint32_t hcTransform = IM_COL32(200, 200, 200, 255);
    static const uint32_t hcGenerator = IM_COL32(150, 200, 150, 255);
    static const uint32_t hcMaterial = IM_COL32(150, 150, 200, 255);
    static const uint32_t hcBlend = IM_COL32(200, 150, 150, 255);
    static const uint32_t hcFilter = IM_COL32(200, 200, 150, 255);
    static const uint32_t hcNoise = IM_COL32(150, 250, 150, 255);
    static const uint32_t hcPaint = IM_COL32(100, 250, 180, 255);


    gMetaNodes = {

        {
            "Circle", hcGenerator, 1
            ,{ {} }
        ,{ { "", Con_Float4 } }
        ,{ { "Radius", Con_Float, -.5f,0.5f,0.f,0.f },{ "T", Con_Float } }
        }
        ,
        {
            "Transform", hcTransform, 0
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "Translate", Con_Float2, 1.f,0.f,1.f,0.f, true },{ "Scale", Con_Float2 },{ "Rotation", Con_Angle } }
        }
        ,
        {
            "Square", hcGenerator, 1
            ,{ {} }
        ,{ { "", Con_Float4 } }
        ,{ { "Width", Con_Float, -.5f,0.5f,0.f,0.f } }
        }
        ,
        {
            "Checker", hcGenerator, 1
            ,{ {} }
        ,{ { "", Con_Float4 } }
        ,{}
        }
        ,
        {
            "Sine", hcGenerator, 1
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "Frequency", Con_Float },{ "Angle", Con_Angle } }
        }

        ,
        {
            "SmoothStep", hcFilter, 4
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "Low", Con_Float },{ "High", Con_Float } }
        }

        ,
        {
            "Pixelize", hcTransform, 0
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "scale", Con_Float } }
        }

        ,
        {
            "Blur", hcFilter, 4
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "angle", Con_Float },{ "strength", Con_Float } }
        }

        ,
        {
            "NormalMap", hcFilter, 4
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "spread", Con_Float } }
        }

        ,
        {
            "LambertMaterial", hcMaterial, 2
            ,{ { "Diffuse", Con_Float4 },{ "Equirect sky", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "view", Con_Float2, 1.f,0.f,0.f,1.f } }
        }

        ,
        {
            "MADD", hcBlend, 3
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "Mul Color", Con_Color4 },{ "Add Color", Con_Color4 } }
        }

        ,
        {
            "Hexagon", hcGenerator, 1
            ,{}
        ,{ { "", Con_Float4 } }
        ,{}
        }

        ,
        {
            "Blend", hcBlend, 3
            ,{ { "", Con_Float4 },{ "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "A", Con_Float4 },{ "B", Con_Float4 },{ "Operation", Con_Enum, 0.f,0.f,0.f,0.f, false, false, "Add\0Multiply\0Darken\0Lighten\0Average\0Screen\0Color Burn\0Color Dodge\0Soft Light\0Subtract\0Difference\0Inverse Difference\0Exclusion\0" } }
        }

        ,
        {
            "Invert", hcFilter, 4
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{}
        }

        ,
        {
            "CircleSplatter", hcGenerator, 1
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "Distance", Con_Float2 },{ "Radius", Con_Float2 },{ "Angle", Con_Angle2 },{ "Count", Con_Float } }
        }

        ,
        {
            "Ramp", hcFilter, 4
            ,{ { "", Con_Float4 },{ "Gradient", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "Ramp", Con_Ramp } }
        }

        ,
        {
            "Tile", hcTransform, 0
            ,{ { "", Con_Float4 }, { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "Offset 0", Con_Float2 },{ "Offset 1", Con_Float2 },{ "Overlap", Con_Float2 },{ "Scale", Con_Float } }
        }

        ,
        {
            "Color", hcGenerator, -1
            ,{}
        ,{ { "", Con_Float4 } }
        ,{ { "Color", Con_Color4 } }
        }


        ,
        {
            "NormalMapBlending", hcBlend, 3
            ,{ { "", Con_Float4 },{ "", Con_Float4 } }
        ,{ { "Out", Con_Float4 } }
        ,{ { "Technique", Con_Enum, 0.f,0.f,0.f,0.f, false, false, "RNM\0Partial Derivatives\0Whiteout\0UDN\0Unity\0Linear\0Overlay\0" } }
        }

        ,
        {
            "iqnoise", hcNoise, 5
            ,{}
        ,{ { "", Con_Float4 } }
        ,{ { "Size", Con_Float },{ "U", Con_Float},{ "V", Con_Float} }
        }

        ,
        {
            "PBR", hcMaterial, 2
            ,{ { "Diffuse", Con_Float4 },{ "Normal", Con_Float4 },{ "Roughness", Con_Float4 },{ "Displacement", Con_Float4 },{ "Cubemap", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "View", Con_Float2, 1.f,0.f,0.f,1.f, true }, { "Displacement Factor", Con_Float },{ "Geometry", Con_Enum, 0.f,0.f,0.f,0.f, false, false, "Door knob\0Sphere\0Cube\0Plane\0Cylinder\0" } }
        }

        ,

        {
            "PolarCoords", hcTransform, 0
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "Type", Con_Enum, 0.f,0.f,0.f,0.f,false, false, "Linear to polar\0Polar to linear\0" } }
        }

        ,
        {
            "Clamp", hcFilter, 4
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "Min", Con_Float4 },{ "Max", Con_Float4 } }
        }

        ,
        {
            "ImageRead", hcFilter, 6
            ,{}
        ,{ { "", Con_Float4 } }
        ,{ { "File name", Con_FilenameRead }
        ,{ "+X File name", Con_FilenameRead }
        ,{ "-X File name", Con_FilenameRead }
        ,{ "+Y File name", Con_FilenameRead }
        ,{ "-Y File name", Con_FilenameRead }
        ,{ "+Z File name", Con_FilenameRead }
        ,{ "-Z File name", Con_FilenameRead } }
        }

        ,
        {
            "ImageWrite", hcFilter, 6
            ,{ { "", Con_Float4 } }
        ,{}
        ,{ { "File name", Con_FilenameWrite },{ "Format", Con_Enum, 0.f,0.f,0.f,0.f, false, false, "JPEG\0PNG\0TGA\0BMP\0HDR\0DDS\0KTX\0MP4\0" }
        ,{ "Quality", Con_Enum, 0.f,0.f,0.f,0.f, false, false, " 0 .. Best\0 1\0 2\0 3\0 4\0 5 .. Medium\0 6\0 7\0 8\0 9 .. Lowest\0" }
        ,{ "Width", Con_Int }
        ,{ "Height", Con_Int }
        ,{ "Mode", Con_Enum, 0.f,0.f,0.f,0.f, false, false, "Free\0Keep ratio on Y\0Keep ratio on X\0"}
        ,{ "Export", Con_ForceEvaluate } }
        }

        ,
        {
            "Thumbnail", hcFilter, 6
            ,{ { "", Con_Float4 } }
        ,{}
        ,{ { "Make", Con_ForceEvaluate } }
        }

        ,
        {
            "Paint2D", hcPaint, 7
            ,{ { "Brush", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "Size", Con_Enum, 0.f,0.f,0.f,0.f, false, false, "  256\0  512\0 1024\0 2048\0 4096\0" } }
        , true
        , true
        }
        ,
        {
            "Swirl", hcTransform, 0
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "Angles", Con_Angle2 } }
        }
        ,
        {
            "Crop", hcTransform, 0
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "Quad", Con_Float4, 0.f,1.f,0.f,1.f, false, true } }
        , true
        }

        ,
        {
            "CubemapFilter", hcFilter, 8
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "Lighting Model", Con_Enum, 0.f,0.f,0.f,0.f, false, false, "Phong\0Phong BRDF\0Blinn\0Blinn BRDF\0" }
        ,{ "Exclude Base", Con_Bool }
        ,{ "Gloss scale", Con_Int }
        ,{ "Gloss bias", Con_Int }
        ,{ "Face size", Con_Enum, 0.f,0.f,0.f,0.f, false, false, "   32\0   64\0  128\0  256\0  512\0 1024\0" }
        }
        }

        ,
        {
            "PhysicalSky", hcGenerator, 8
            ,{}
        ,{ { "", Con_Float4 } }
        ,{ { "ambient", Con_Float4 }
, { "lightdir", Con_Float4 }
, { "Kr", Con_Float4 }
, { "rayleigh brightness", Con_Float }
, { "mie brightness", Con_Float }
, { "spot brightness", Con_Float }
, { "scatter strength", Con_Float }
, { "rayleigh strength", Con_Float }
, { "mie strength" , Con_Float }
, { "rayleigh collection power", Con_Float }
, { "mie collection power", Con_Float }
, { "mie distribution", Con_Float }
, { "Size", Con_Enum, 0.f,0.f,0.f,0.f, false, false, "  256\0  512\0 1024\0 2048\0 4096\0" }
            }
        }


        ,
        {
            "CubemapView", hcGenerator, 8
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ { "view", Con_Float2, 1.f,0.f,0.f,1.f, true },{ "Mode", Con_Enum, 0.f,0.f,0.f,0.f, false, false, "Projection\0Isometric\0Cross\0Camera\0" } }
        }

            ,
            {
                "EquirectConverter", hcGenerator, 8
                ,{ { "", Con_Float4 } }
            ,{ { "", Con_Float4 } }
            ,{ { "Mode", Con_Enum, 0.f,0.f,0.f,0.f, false, false, "Equirect To Cubemap\0Cubemap To Equirect\0" },
                { "Size", Con_Enum, 0.f,0.f,0.f,0.f, false, false, "  256\0  512\0 1024\0 2048\0 4096\0" } }
            }
            ,
            {
                "NGon", hcGenerator, 1
                ,{  }
            ,{ { "", Con_Float4 } }
            ,{ {"Sides", Con_Int}, { "Radius", Con_Float, -.5f,0.5f,0.f,0.f },{ "T", Con_Float } }
            }

            ,
            {
                "GradientBuilder", hcGenerator, 1
                ,{  }
            ,{ { "", Con_Float4 } }
            ,{ { "Gradient", Con_Ramp4 } }
            }

            ,
            {
                "Warp", hcTransform, 0
                ,{ { "", Con_Float4 }, { "Warp", Con_Float4 } }
            ,{ { "", Con_Float4 } }
            ,{ { "Strength", Con_Float },{ "Mode", Con_Enum, 0.f,0.f,0.f,0.f, false, false, "XY Offset\0Rotation-Distance\0" } }
            }

            ,
            {
                "TerrainPreview", hcMaterial, 2
                ,{ { "Height", Con_Float4 }, { "Diffuse", Con_Float4 }, { "AO", Con_Float4 }, { "Cubemap", Con_Float4 } }
            ,{ { "", Con_Float4 } }
            ,{ { "Camera", Con_Camera } }
            }

            ,
            {
                "AO", hcFilter, 4
                ,{ { "", Con_Float4 } }
            ,{ { "", Con_Float4 } }
        ,{  { "strength", Con_Float }, { "area", Con_Float }, { "falloff", Con_Float }, { "radius", Con_Float }}
        }


            ,
        {
            "FurGenerator", hcFilter, 9
            ,{ { "Color", Con_Float4 }, { "Length", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{  { "Hair count", Con_Int }, { "Length factor", Con_Float }}
        }

            ,
        {
            "FurDisplay", hcFilter, 9
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{  { "Camera", Con_Camera }}
        }
            ,
        {
            "FurIntegrator", hcFilter, 9
            ,{ { "", Con_Float4 } }
        ,{ { "", Con_Float4 } }
        ,{ }
        }

    };


    for (size_t i = 0; i < gMetaNodes.size(); i++)
    {
        gMetaNodesIndices[gMetaNodes[i].mName] = i;
    }
}

struct AnimationPointer
{
    uint32_t mPreviousIndex;
    uint32_t mNextIndex;
    float mRatio;
};

AnimationBase::AnimationPointer AnimationBase::GetPointer(uint32_t frame, bool bSetting) const
{
    if (mFrames.empty())
        return { 0, 0, 0, 0, 0.f };
    if (frame <= mFrames[0])
        return { 0, 0, 0, 0, 0.f };
    if (frame >= mFrames.back())
    {
        uint32_t last = uint32_t(mFrames.size() - (bSetting ? 0 : 1));
        return {last, mFrames.back(), last, mFrames.back(), 0.f };
    }
    for (uint32_t i = 0; i < mFrames.size() - 1; i++)
    {
        if (mFrames[i] <= frame && mFrames[i + 1] >= frame)
        {
            float ratio = float(frame - mFrames[i]) / float(mFrames[i+1] - mFrames[i]);
            return { i, mFrames[i], i + 1, mFrames[i+1], ratio };
        }
    }
    assert(0);
    return { 0, 0, 0, 0, 0.f };
}

AnimTrack& AnimTrack::operator = (const AnimTrack& other)
{
    mNodeIndex = other.mNodeIndex;
    mParamIndex = other.mParamIndex;
    mValueType = other.mValueType;
    mAnimation = AllocateAnimation(mValueType);
    mAnimation->Copy(other.mAnimation);
    return *this;
}