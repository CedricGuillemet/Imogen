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
#include "MetaNodes.h"

struct Camera;

struct ParameterBlock
{
    ParameterBlock(uint16_t nodeType) : mNodeType(nodeType)
    {
    }
    ParameterBlock(uint16_t nodeType, const std::vector<uint8_t>& dump) : mDump(dump), mNodeType(nodeType)
    {
    }
    
    operator const std::vector<uint8_t>&() const { return mDump; }

    ParameterBlock& InitDefault();
    float GetParameterComponentValue(int parameterIndex, int componentIndex) const;
    Camera* GetCamera() const;
    int GetIntParameter(const char* parameterName, int defaultValue) const;
    void *Data(size_t parameterInde);
    void* Data() { return mDump.data(); }
    const void* Data() const { return mDump.data(); }

protected:
    std::vector<unsigned char> mDump;
    uint16_t mNodeType;

    template<typename type> type GetParameter(const char* parameterName, type defaultValue, ConTypes parameterType) const;
    template<typename type> type* GetParameterPtr(const char* parameterName, type* defaultValue, ConTypes parameterType) const;

};

