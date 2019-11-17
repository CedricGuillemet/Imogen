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
#include <float.h>
#include <vector>
#include <math.h>
#include "Platform.h"

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);


typedef void (*LogOutput)(const char* szText);
void AddLogOutput(LogOutput output);
int Log(const char* szFormat, ...);
template<typename T>
T Lerp(T a, T b, float t)
{
    return T(a + (b - a) * t);
}

inline int align(int value, int alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

void IMessageBox(const char* text, const char* title);
void DiscoverFiles(const char* extension, const char* directory, std::vector<std::string>& files);

void OpenShellURL(const std::string& url);

std::string GetName(const std::string& name);
std::string GetGroup(const std::string& name);

template<typename T>
void Swap(T& a, T& b)
{
    T temp = a;
    a = b;
    b = temp;
}

template<typename T>
T Min(const T& a, const T& b)
{
    return (a < b) ? a : b;
}

template<typename T>
T Max(const T& a, const T& b)
{
    return (a > b) ? a : b;
}

enum EvaluationStatus
{
    EVAL_OK,
    EVAL_ERR,
    EVAL_DIRTY,
};

std::string GetBasePath(const char* path);

static const float PI = 3.141592f;
inline float RadToDeg(float a)
{
    return a * 180.f / PI;
}
inline float DegToRad(float a)
{
    return a / 180.f * PI;
}

void Splitpath(const char* completePath, char* drive, char* dir, char* filename, char* ext);

inline std::vector<char> ReadFile(const char* szFileName)
{
    FILE* fp = fopen(szFileName, "rb");
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        auto bufSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        //char* buf = new char[bufSize];
        std::vector<char> buf(bufSize);
        fread(buf.data(), bufSize, 1, fp);
        fclose(fp);
        return buf;
    }
    return {};
}

inline float Saturate(float v)
{
    return Min(Max(v, 0.f), 1.f);
}

inline uint32_t ColorU8(uint8_t R, uint8_t G, uint8_t B, uint8_t A)
{
    return (((uint32_t)(A)<<24) | ((uint32_t)(B)<<16) | ((uint32_t)(G)<<8) | ((uint32_t)(R)));
}

inline uint32_t ColorF32(float R, float G, float B, float A)
{
    uint32_t out;
    out = (uint32_t)(Saturate(R) * 255.f + 0.5f) << 0;
    out |= (uint32_t)(Saturate(G) * 255.f + 0.5f) << 8;
    out |= (uint32_t)(Saturate(B) * 255.f + 0.5f) << 16;
    out |= (uint32_t)(Saturate(A) * 255.f + 0.5f) << 24;
    return out;
}