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
#include "imgui.h"
#include "UI.h"
#include "Utils.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"
#include "Evaluators.h"

static inline ImVec4 operator*(const ImVec4& lhs, const float t)
{
    return ImVec4(lhs.x * t, lhs.y * t, lhs.z * t, lhs.w * t);
}

ImVec4 GradientEdit::GetPoint(float t)
{
    if (GetPointCount() == 0)
        return ImVec4(1.f, 1.f, 1.f, 1.f);
    if (GetPointCount() == 1 || t <= mPts[0].w)
        return mPts[0];

    for (size_t i = 0; i < GetPointCount() - 1; i++)
    {
        if (mPts[i].w <= t && mPts[i + 1].w >= t)
        {
            float r = (t - mPts[i].w) / (mPts[i + 1].w - mPts[i].w);
            return Lerp(mPts[i], mPts[i + 1], r);
        }
    }
    return mPts[GetPointCount() - 1];
}

void ImguiAppLog::AddLog(const char* fmt, ...)
{
    int old_size = Buf.size();
    va_list args;
    va_start(args, fmt);
    Buf.appendfv(fmt, args);
    va_end(args);
    for (int new_size = Buf.size(); old_size < new_size; old_size++)
        if (Buf[old_size] == '\n')
            LineOffsets.push_back(old_size);
    ScrollToBottom = true;
}

void ImguiAppLog::DrawEmbedded()
{
    if (ImGui::Button("Clear"))
        Clear();
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy");
    ImGui::SameLine();
    Filter.Draw("Filter", -100.0f);
    ImGui::Separator();
    ImGui::BeginChild("scrolling");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
    if (copy)
        ImGui::LogToClipboard();

    if (Filter.IsActive())
    {
        const char* buf_begin = Buf.begin();
        const char* line = buf_begin;
        for (int line_no = 0; line != NULL; line_no++)
        {
            const char* line_end = (line_no < LineOffsets.Size) ? buf_begin + LineOffsets[line_no] : NULL;
            if (Filter.PassFilter(line, line_end))
                ImGui::TextUnformatted(line, line_end);
            line = line_end && line_end[1] ? line_end + 1 : NULL;
        }
    }
    else
    {
        ImGui::TextUnformatted(Buf.begin());
    }

    if (ScrollToBottom)
        ImGui::SetScrollY(ImGui::GetScrollMaxY());

    ScrollToBottom = false;
    ImGui::PopStyleVar();
    ImGui::EndChild();
}

ImguiAppLog* ImguiAppLog::Log = NULL;
ImguiAppLog logger;

void ImConsoleOutput(const char* szText)
{
    static ImguiAppLog imguiLog;
    imguiLog.AddLog(szText);
}

void SetStyle()
{
    ImGuiStyle& st = ImGui::GetStyle();
    st.FrameBorderSize = 1.0f;
    st.FramePadding = ImVec2(4.0f, 2.0f);
    st.ItemSpacing = ImVec2(8.0f, 2.0f);
    st.WindowBorderSize = 1.0f;
    st.TabBorderSize = 1.0f;
    st.WindowRounding = 1.0f;
    st.ChildRounding = 1.0f;
    st.FrameRounding = 1.0f;
    st.ScrollbarRounding = 1.0f;
    st.GrabRounding = 1.0f;
    st.TabRounding = 1.0f;

    // Setup style
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.53f, 0.53f, 0.53f, 0.46f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.85f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 0.53f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.79f, 0.79f, 0.79f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.48f, 0.47f, 0.47f, 0.91f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.55f, 0.55f, 0.62f);
    colors[ImGuiCol_Button] = ImVec4(0.50f, 0.50f, 0.50f, 0.63f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.67f, 0.67f, 0.68f, 0.63f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.26f, 0.26f, 0.63f);
    colors[ImGuiCol_Header] = ImVec4(0.54f, 0.54f, 0.54f, 0.58f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.64f, 0.65f, 0.65f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.25f, 0.80f);
    colors[ImGuiCol_Separator] = ImVec4(0.58f, 0.58f, 0.58f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.87f, 0.87f, 0.87f, 0.53f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
    colors[ImGuiCol_Tab] = ImVec4(0.01f, 0.01f, 0.01f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.29f, 0.79f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.31f, 0.31f, 0.91f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.38f, 0.48f, 0.60f, 1.00f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.77f, 0.33f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.87f, 0.55f, 0.08f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.47f, 0.60f, 0.76f, 0.47f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

extern ImGui::MarkdownConfig mdConfig;
ImFont *smallAF, *bigAF, *mediumAF;

void InitFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;

    float fontSize_ = 16.f;

    static const char* defaultFontPath = "Stock/Fonts/OpenSans-SemiBold.ttf";
    io.Fonts->AddFontFromFileTTF(defaultFontPath, fontSize_);
    smallAF = io.Fonts->AddFontFromFileTTF("Stock/Fonts/" FONT_ICON_FILE_NAME_FAS, fontSize_, &icons_config, icons_ranges);

    mediumAF = io.Fonts->AddFontFromFileTTF(defaultFontPath, 20.f);
    io.Fonts->AddFontFromFileTTF("Stock/Fonts/" FONT_ICON_FILE_NAME_FAS, 20.f, &icons_config, icons_ranges);

    bigAF = io.Fonts->AddFontFromFileTTF(defaultFontPath, 24.f);
    io.Fonts->AddFontFromFileTTF("Stock/Fonts/" FONT_ICON_FILE_NAME_FAS, 24.f, &icons_config, icons_ranges);


    // Bold headings H2 and H3
    mdConfig.headingFormats[1].font = io.Fonts->AddFontFromFileTTF("Stock/Fonts/OpenSans-ExtraBold.ttf", fontSize_);
    mdConfig.headingFormats[2].font = mdConfig.headingFormats[1].font;
    // bold heading H1
    float fontSizeH1 = fontSize_ * 1.2f;
    mdConfig.headingFormats[0].font = io.Fonts->AddFontFromFileTTF("Stock/Fonts/OpenSans-ExtraBold.ttf", fontSizeH1);
}

struct ImogenDrawCallback
{
    NodeUICallBackFunc mFunc;
    ImRect mClippedRect;
    ImRect mOrginalRect;
    NodeIndex mNodeIndex;
    EvaluationContext* mEvaluationContext;
};

std::vector<ImogenDrawCallback> mCallbackRects;


void UICallbackNodeDeleted(NodeIndex nodeIndex)
{
    auto iter = mCallbackRects.begin();
    for (; iter != mCallbackRects.end(); )
    {
        if (iter->mNodeIndex == nodeIndex)
        {
            iter = mCallbackRects.erase(iter);
            continue;
        }
        if (iter->mNodeIndex > nodeIndex)
        {
            iter->mNodeIndex--;
        }
        ++iter;
    }
}

void UICallbackNodeInserted(NodeIndex nodeIndex)
{
    for (auto& extract : mCallbackRects)
    {
        if (extract.mNodeIndex >= nodeIndex)
        {
            extract.mNodeIndex++;
        }
    }
}
extern Evaluators gEvaluators;
static void NodeUICallBack(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
    ImGuiIO& io = ImGui::GetIO();

    if (!mCallbackRects.empty())
    {
        ImogenDrawCallback& cb = mCallbackRects[intptr_t(cmd->UserCallbackData)];

        ImRect cr = cb.mClippedRect;
        bgfx::setScissor(int(cr.Min.x),
                        int(cr.Min.y),
                        int(cr.Max.x - cr.Min.x),
                        int(cr.Max.y - cr.Min.y));

        cr = cb.mOrginalRect;
        float sx = (cr.Max.x - cr.Min.x) / io.DisplaySize.x * 2.f;
        float sy = (cr.Max.y - cr.Min.y) / io.DisplaySize.y * 2.f;
        float tx = cr.Min.x / io.DisplaySize.x * 2.f - 1.f;
        float ty = -cr.Max.y / io.DisplaySize.y * 2.f + 1.f;
        float uvt[4] = {sx, sy, tx, ty };
        bgfx::setUniform(gEvaluators.u_uvTransform, uvt);

        cb.mFunc(cb.mEvaluationContext, cb.mNodeIndex);
    }
}

void InitCallbackRects()
{
    mCallbackRects.clear();
}

size_t AddNodeUICallbackRect(NodeUICallBackFunc func, const ImRect& rect, NodeIndex nodeIndex, EvaluationContext* context)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 mi = draw_list->GetClipRectMin();
    ImVec2 ma = draw_list->GetClipRectMax();

    ImRect rc = rect;
    rc.ClipWith(ImRect(mi, ma));
    mCallbackRects.push_back({func, rc, rect, nodeIndex, context});
    return mCallbackRects.size() - 1;
}

void AddUICustomDraw(
    ImDrawList* drawList, const ImRect& rc, NodeUICallBackFunc func, NodeIndex nodeIndex, EvaluationContext* context)
{
    drawList->AddCallback((ImDrawCallback)(NodeUICallBack),
                          (void*)(AddNodeUICallbackRect(func, rc, nodeIndex, context)));
}

