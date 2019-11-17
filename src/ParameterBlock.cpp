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

#include <assert.h>
#include <algorithm>
#include "ParameterBlock.h"
#include "Cam.h"
#include "Utils.h"


ParameterBlock& ParameterBlock::InitDefault()
{
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[mNodeType];
    const size_t paramsSize = ComputeNodeParametersSize(mNodeType);
    mDump.resize(paramsSize, 0);
    unsigned char* paramBuffer = mDump.data();
    int i = 0;
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (!param.mDefaultValue.empty())
        {
            memcpy(paramBuffer, param.mDefaultValue.data(), param.mDefaultValue.size());
        }

        paramBuffer += GetParameterTypeSize(param.mType);
    }
    return *this;
}

float ParameterBlock::GetParameterComponentValue(int parameterIndex, int componentIndex) const
{
    size_t paramOffset = GetParameterOffset(uint32_t(mNodeType), parameterIndex);
    const unsigned char* ptr = &mDump.data()[paramOffset];
    const MetaNode* metaNodes = gMetaNodes.data();
    const MetaNode& currentMeta = metaNodes[mNodeType];
    switch (currentMeta.mParams[parameterIndex].mType)
    {
    case Con_Angle:
    case Con_Float:
        return ((float*)ptr)[componentIndex];
    case Con_Angle2:
    case Con_Float2:
        return ((float*)ptr)[componentIndex];
    case Con_Angle3:
    case Con_Float3:
        return ((float*)ptr)[componentIndex];
    case Con_Angle4:
    case Con_Color4:
    case Con_Float4:
        return ((float*)ptr)[componentIndex];
    case Con_Ramp:
        return 0;
    case Con_Ramp4:
        return 0;
    case Con_Enum:
    case Con_Int:
        return float(((int*)ptr)[componentIndex]);
    case Con_Int2:
        return float(((int*)ptr)[componentIndex]);
    case Con_FilenameRead:
    case Con_FilenameWrite:
        return 0;
    case Con_ForceEvaluate:
        return 0;
    case Con_Bool:
        return float(((bool*)ptr)[componentIndex]);
    case Con_Camera:
        return float((*(Camera*)ptr)[componentIndex]);
    default:
        return 0.f;
    }
}

template<typename type> type ParameterBlock::GetParameter(const char* parameterName, type defaultValue, ConTypes parameterType) const
{
    const MetaNode& currentMeta = gMetaNodes[mNodeType];
    const size_t paramsSize = ComputeNodeParametersSize(mNodeType);
    const unsigned char* paramBuffer = mDump.data();
    for (const MetaParameter& param : currentMeta.mParams)
    {
        if (param.mType == parameterType)
        {
            if (parameterName == nullptr || (!strcmp(param.mName.c_str(), parameterName)))
            {
                return *(type*)paramBuffer;
            }
        }
        paramBuffer += GetParameterTypeSize(param.mType);
    }
    return defaultValue;
}

int ParameterBlock::GetIntParameter(const char* parameterName, int defaultValue) const
{
    return GetParameter(parameterName, defaultValue, Con_Int);
}

Camera* ParameterBlock::GetCamera() const
{
    return GetParameter(nullptr, nullptr, Con_Camera);
}

void* ParameterBlock::Data(size_t parameterIndex)
{
    assert(parameterIndex< gMetaNodes[mNodeType].mParams.size());
    size_t parameterOffset = GetParameterOffset(mNodeType, parameterIndex);
    uint8_t* data = (uint8_t*)Data();
    data += parameterOffset;
    return data;
}