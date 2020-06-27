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

#include "ImCurveEdit.h"
#include "imgui_markdown/imgui_markdown.h"
#include <algorithm>
#include "Types.h"

void SetStyle();
void InitFonts();

struct ImguiAppLog
{
    ImguiAppLog()
    {
        Log = this;
    }
    static ImguiAppLog* Log;
    ImGuiTextBuffer Buf;
    ImGuiTextFilter Filter;
    ImVector<int> LineOffsets; // Index to lines offset
    bool ScrollToBottom;

    void Clear()
    {
        Buf.clear();
        LineOffsets.clear();
    }
    void AddLog(const char* fmt, ...);
    void DrawEmbedded();
};

void ImConsoleOutput(const char* szText);

struct RampEdit : public ImCurveEdit::Delegate
{
    RampEdit()
    {
        mMin = ImVec2(0.f, 0.f);
        mMax = ImVec2(1.f, 1.f);
    }
    size_t GetCurveCount()
    {
        return 1;
    }

    size_t GetPointCount(size_t curveIndex)
    {
        return mPointCount;
    }

    uint32_t GetCurveColor(size_t curveIndex)
    {
        return 0xFFAAAAAA;
    }
    ImVec2* GetPoints(size_t curveIndex)
    {
        return mPts;
    }
    virtual ImVec2& GetMin()
    {
        return mMin;
    }
    virtual ImVec2& GetMax()
    {
        return mMax;
    }

    virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
    {
        mPts[pointIndex] = value;
        SortValues(curveIndex);
        for (size_t i = 0; i < GetPointCount(curveIndex); i++)
        {
            if (mPts[i].x == value.x)
                return int(i);
        }
        return pointIndex;
    }
    virtual void AddPoint(size_t curveIndex, ImVec2 value)
    {
        if (mPointCount >= 8)
            return;
        mPts[mPointCount++] = value;
        SortValues(curveIndex);
    }

    virtual void DelPoint(size_t curveIndex, size_t pointIndex)
    {
        mPts[pointIndex].x = FLT_MAX;
        SortValues(curveIndex);
        mPointCount--;
        mPts[mPointCount].x = -1.f; // end marker
    }
    ImVec2 mPts[8];
    size_t mPointCount;

private:
    void SortValues(size_t curveIndex)
    {
        auto b = std::begin(mPts);
        auto e = std::begin(mPts) + GetPointCount(curveIndex);
        std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });
    }
    ImVec2 mMin, mMax;
};

// draw callbacks
struct ImRect;
struct ImDrawList;
struct EvaluationContext;
typedef void (*NodeUICallBackFunc)(EvaluationContext* context, NodeIndex nodeIndex);
void AddUICustomDraw(
    ImDrawList* drawList, const ImRect& rc, NodeUICallBackFunc func, NodeIndex nodeIndex, EvaluationContext* context);
void InitCallbackRects();

void UICallbackNodeDeleted(NodeIndex nodeIndex);
void UICallbackNodeInserted(NodeIndex nodeIndex);

extern ImFont *smallAF, *bigAF, *mediumAF;