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

#include <vector>
#include <string>


enum ConTypes
{
    Con_Float,
    Con_Float2,
    Con_Float3,
    Con_Float4,
    Con_Color4,
    Con_Int,
    Con_Int2,
    Con_Ramp,
    Con_Angle,
    Con_Angle2,
    Con_Angle3,
    Con_Angle4,
    Con_Enum,
    Con_Structure,
    Con_FilenameRead,
    Con_FilenameWrite,
    Con_ForceEvaluate,
    Con_Bool,
    Con_Ramp4,
    Con_Camera,
    Con_Multiplexer,
    Con_Any,
};

enum ControlTypes
{
    Control_NumericEdit,
    Control_Slider,
};

enum CurveType
{
    CurveNone,
    CurveDiscrete,
    CurveLinear,
    CurveSmooth,
    CurveBezier,
};

struct MetaCon
{
    std::string mName;
    int mType;
    bool operator==(const MetaCon& other) const
    {
        if (mName != other.mName)
            return false;
        if (mType != other.mType)
            return false;
        return true;
    }
};

struct MetaParameter
{
    std::string mName;
    ConTypes mType;
    ControlTypes mControlType;
    float mRangeMinX, mRangeMaxX;
    float mRangeMinY, mRangeMaxY;
    float mSliderMinX, mSliderMaxX;
    bool mbRelative;
    bool mbQuadSelect;
    bool mbLoop;
    bool mbHidden;
    std::string mEnumList;
    std::vector<unsigned char> mDefaultValue;
    std::string mDescription;
    bool operator==(const MetaParameter& other) const
    {
        if (mName != other.mName)
            return false;
        if (mType != other.mType)
            return false;
        if (mRangeMaxX != other.mRangeMaxX)
            return false;
        if (mRangeMinX != other.mRangeMinX)
            return false;
        if (mRangeMaxY != other.mRangeMaxY)
            return false;
        if (mRangeMinY != other.mRangeMinY)
            return false;
        if (mbRelative != other.mbRelative)
            return false;
        if (mbQuadSelect != other.mbQuadSelect)
            return false;
        if (mEnumList != other.mEnumList)
            return false;
        return true;
    }
};

struct MetaNode
{
    std::string mName;
    uint32_t mHeaderColor;
    int mCategory;
    std::string mDescription;
    std::vector<MetaCon> mInputs;
    std::vector<MetaCon> mOutputs;
    std::vector<MetaParameter> mParams;

    int mWidth;
    int mHeight;

    bool mbHasUI;
    bool mbSaveTexture;
    bool mbExperimental;
    bool mbThumbnail;
    bool operator==(const MetaNode& other) const
    {
        if (mName != other.mName)
            return false;
        if (mCategory != other.mCategory)
            return false;
        if (mHeaderColor != other.mHeaderColor)
            return false;
        if (mInputs != other.mInputs)
            return false;
        if (mOutputs != other.mOutputs)
            return false;
        if (mParams != other.mParams)
            return false;
        if (mbHasUI != other.mbHasUI)
            return false;
        if (mbSaveTexture != other.mbSaveTexture)
            return false;
        return true;
    }

    static const std::vector<std::string> mCategories;
};

extern std::vector<MetaNode> gMetaNodes;

size_t GetMetaNodeIndex(const std::string& metaNodeName);
void LoadMetaNodes();
size_t GetParameterTypeSize(ConTypes paramType);
CurveType GetCurveTypeForParameterType(ConTypes paramType);
const char* GetParameterTypeName(ConTypes paramType);
ConTypes GetParameterType(uint32_t nodeType, uint32_t parameterIndex);
void ParseStringToParameter(const std::string& str, ConTypes parameterType, void* parameterPtr);
int GetParameterIndex(uint32_t nodeType, const char* parameterName);
size_t GetParameterOffset(uint32_t type, uint32_t parameterIndex);
ConTypes GetParameterType(uint32_t nodeType, uint32_t parameterIndex);
size_t GetCurveCountPerParameterType(ConTypes paramType);
const char* GetCurveParameterSuffix(ConTypes paramType, int suffixIndex);
uint32_t GetCurveParameterColor(ConTypes paramType, int suffixIndex);
size_t ComputeNodeParametersSize(size_t nodeType);
