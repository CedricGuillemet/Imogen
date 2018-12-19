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

#include <SDL.h>
#include "imgui.h"
#include "Imogen.h"
#include "TextEditor.h"
#include <fstream>
#include <streambuf>
#include "Evaluation.h"
#include "NodesDelegate.h"
#include "Library.h"
#include "TaskScheduler.h"
#include "stb_image.h"
#include "tinydir.h"
#include "stb_image.h"
#include "imgui_stdlib.h"
#include "ImSequencer.h"
#include "Evaluators.h"
#include "nfd.h"

unsigned char *stbi_write_png_to_mem(unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len);
extern Evaluation gEvaluation;
int gEvaluationTime = 0;
extern enki::TaskScheduler g_TS;
void ClearAll(TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation);

struct ImguiAppLog
{
    ImguiAppLog()
    {
        Log = this;
    }
    static ImguiAppLog *Log;
    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    ImVector<int>       LineOffsets;        // Index to lines offset
    bool                ScrollToBottom;

    void    Clear() { Buf.clear(); LineOffsets.clear(); }

    void    AddLog(const char* fmt, ...)
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

    void DrawEmbedded()
    {
        if (ImGui::Button("Clear")) Clear();
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);
        ImGui::Separator();
        ImGui::BeginChild("scrolling");
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
        if (copy) ImGui::LogToClipboard();

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

//        if (ScrollToBottom)
//            ImGui::SetScrollHere(1.0f);
        ScrollToBottom = false;
        ImGui::PopStyleVar();
        ImGui::EndChild();
    }
};

void SetStyle()
{
    ImGuiStyle &st = ImGui::GetStyle();
    st.FrameBorderSize = 1.0f;
    st.FramePadding = ImVec2(4.0f,2.0f);
    st.ItemSpacing = ImVec2(8.0f,2.0f);
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

std::vector<ImogenDrawCallback> mCallbackRects;
struct ExtractedView
{
    size_t mNodeIndex;
    bool mFirstFrame;
};
std::vector<ExtractedView> mExtratedViews;
void AddExtractedView(size_t nodeIndex)
{
    mExtratedViews.push_back({ nodeIndex, true });
}
void ClearExtractedViews()
{
    mExtratedViews.clear();
}
void InitCallbackRects()
{
    mCallbackRects.clear();
}
size_t AddNodeUICallbackRect(CallbackDisplayType type, const ImRect& rect, size_t nodeIndex)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 mi = draw_list->GetClipRectMin();
    ImVec2 ma = draw_list->GetClipRectMax();

    ImRect rc = rect;
    rc.ClipWith(ImRect(mi, ma));
    mCallbackRects.push_back({ type, rc, rect, nodeIndex });
    return mCallbackRects.size() - 1;
}

ImguiAppLog *ImguiAppLog::Log = NULL;
ImguiAppLog logger;
TextEditor editor;
void DebugLogText(const char *szText)
{
    static ImguiAppLog imguiLog;
    imguiLog.AddLog(szText);
}

void Imogen::HandleEditor(TextEditor &editor, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
    static int currentShaderIndex = -1;

    if (currentShaderIndex == -1)
    {
        currentShaderIndex = 0;
        editor.SetText(gEvaluators.GetEvaluator(mEvaluatorFiles[currentShaderIndex].mFilename));
    }
    auto cpos = editor.GetCursorPosition();
    ImGui::BeginChild(13, ImVec2(250, 800));
    for (size_t i = 0; i < mEvaluatorFiles.size(); i++)
    {
        bool selected = i == currentShaderIndex;
        if (ImGui::Selectable(mEvaluatorFiles[i].mFilename.c_str(), &selected))
        {
            currentShaderIndex = int(i);
            editor.SetText(gEvaluators.GetEvaluator(mEvaluatorFiles[currentShaderIndex].mFilename));
            editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates());
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild(14);
    // save
    if (ImGui::IsKeyReleased(SDL_SCANCODE_F5))
    {
        auto textToSave = editor.GetText();

        std::ofstream t(mEvaluatorFiles[currentShaderIndex].mDirectory + mEvaluatorFiles[currentShaderIndex].mFilename, std::ofstream::out);
        t << textToSave;
        t.close();

        gEvaluators.SetEvaluators(mEvaluatorFiles);
        gCurrentContext->RunAll();
    }

    ImGui::SameLine();
    ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | F5 to save and update nodes", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
        editor.IsOverwrite() ? "Ovr" : "Ins",
        editor.CanUndo() ? "*" : " ",
        editor.GetLanguageDefinition().mName.c_str());
    editor.Render("TextEditor");
    ImGui::EndChild();

}

void RenderPreviewNode(int selNode, TileNodeEditGraphDelegate& nodeGraphDelegate, Evaluation& evaluation, bool forceUI = false)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF000000);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF000000);
    ImGui::PushStyleColor(ImGuiCol_Button, 0xFF000000);
    float w = ImGui::GetWindowContentRegionWidth();
    int imageWidth(1), imageHeight(1);

    // make 2 evaluation for node to get the UI pass image size
    if (selNode != -1 && nodeGraphDelegate.NodeHasUI(selNode))
    {
        gCurrentContext->AllocRenderTargetsForEditingPreview();
        EvaluationInfo evaluationInfo;
        evaluationInfo.forcedDirty = 1;
        evaluationInfo.uiPass = 1;
        gCurrentContext->RunSingle(selNode, evaluationInfo);
    }
    Evaluation::GetEvaluationSize(selNode, &imageWidth, &imageHeight);
    if (selNode != -1 && nodeGraphDelegate.NodeHasUI(selNode))
    {
        EvaluationInfo evaluationInfo;
        evaluationInfo.forcedDirty = 1;
        evaluationInfo.uiPass = 0;
        gCurrentContext->RunSingle(selNode, evaluationInfo);
    }
    ImTextureID displayedTexture = 0;
    ImRect rc;
    ImVec2 displayedTextureSize;
    ImVec2 mouseUVCoord(-FLT_MAX, -FLT_MAX);
    if (imageWidth && imageHeight)
    {
        float ratio = float(imageHeight) / float(imageWidth);
        float h = w * ratio;
        ImVec2 p = ImGui::GetCursorPos() + ImGui::GetWindowPos();

        if (forceUI)
        {
            ImGui::InvisibleButton("ImTheInvisibleMan", ImVec2(w, h));
            rc = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddCallback((ImDrawCallback)(Evaluation::NodeUICallBack), (void*)(AddNodeUICallbackRect(CBUI_Node, rc, selNode)));

            mouseUVCoord = (io.MousePos - rc.Min) / rc.GetSize();
            mouseUVCoord.y = 1.f - mouseUVCoord.y;
        }
        else
        {
            if (selNode != -1 && nodeGraphDelegate.NodeIsCubemap(selNode))
            {
                ImGui::InvisibleButton("ImTheInvisibleMan", ImVec2(w, h));
            }
            else
            {
                displayedTexture = (ImTextureID)(int64_t)((selNode != -1) ? nodeGraphDelegate.mEditingContext.GetEvaluationTexture(selNode) : 0);
                if (displayedTexture)
                {
                    auto tgt = nodeGraphDelegate.mEditingContext.GetRenderTarget(selNode);
                    displayedTextureSize = ImVec2(float(tgt->mImage.mWidth), float(tgt->mImage.mHeight));
                }
                ImVec2 mouseUVPos = (io.MousePos - p) / ImVec2(w, h);
                mouseUVPos.y = 1.f - mouseUVPos.y;
                Vec4 mouseUVPosv(mouseUVPos.x, mouseUVPos.y);

                Vec4 uva(0, 0), uvb(1, 1);
                Mat4x4* viewMatrix = nodeGraphDelegate.GetParameterViewMatrix(selNode);
                Camera *nodeCamera = nodeGraphDelegate.GetCameraParameter(selNode);
                if (viewMatrix && !nodeCamera)
                {
                    Mat4x4& res = *viewMatrix;
                    Mat4x4 tr, trp, sc;
                    static float scale = 1.f;
                    scale = ImLerp(scale, 1.f, 0.15f);
                    if (ImRect(p, p + ImVec2(w, h)).Contains(io.MousePos) && ImGui::IsWindowFocused())
                    {
                        scale -= io.MouseWheel*0.04f;
                        scale -= io.MouseDelta.y * 0.01f * ((io.KeyAlt&&io.MouseDown[1]) ? 1.f : 0.f);

                        ImVec2 pix2uv = ImVec2(1.f, 1.f) / ImVec2(w, h);
                        Vec4 localTranslate;
                        localTranslate = Vec4(-io.MouseDelta.x * pix2uv.x, io.MouseDelta.y * pix2uv.y) * ((io.KeyAlt&&io.MouseDown[2]) ? 1.f : 0.f) * res[0];
                        Mat4x4 localTranslateMat;
                        localTranslateMat.Translation(localTranslate);
                        res *= localTranslateMat;
                    }

                    mouseUVPosv.TransformPoint(res);
                    mouseUVCoord = ImVec2(mouseUVPosv.x, mouseUVPosv.y);
                    sc.Scale(scale, scale, 1.f);
                    tr.Translation(-mouseUVPosv);
                    trp.Translation(mouseUVPosv);
                    res *= tr * sc * trp;
                    res[0] = ImClamp(res[0], 0.05f, 1.f);
                    res[5] = res[0];

                    res[12] = ImClamp(res[12], 0.f, 1.f - res[0]);
                    res[13] = ImClamp(res[13], 0.f, 1.f - res[5]);

                    uva.TransformPoint(res);
                    uvb.TransformPoint(res);
                }

                ImGui::ImageButton(displayedTexture, ImVec2(w, h), ImVec2(uva.x, uvb.y), ImVec2(uvb.x, uva.y));
            }
            rc = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            if (selNode != -1 && nodeGraphDelegate.NodeIsCubemap(selNode))
            {
                draw_list->AddCallback((ImDrawCallback)(Evaluation::NodeUICallBack), (void*)(AddNodeUICallbackRect(CBUI_Cubemap, rc, selNode)));
            }
            else if (selNode != -1 && nodeGraphDelegate.NodeHasUI(selNode))
            {
                draw_list->AddCallback((ImDrawCallback)(Evaluation::NodeUICallBack), (void*)(AddNodeUICallbackRect(CBUI_Node, rc, selNode)));
            }
        }
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(1);

    static int lastSentExit = -1;
    if (rc.Contains(io.MousePos))
    {
        static Image pickerImage;
        if (io.KeyShift && io.MouseDown[0] && displayedTexture && selNode > -1 && mouseUVCoord.x >= 0.f && mouseUVCoord.y >= 0.f)
        {
            if (!pickerImage.GetBits())
            {
                Evaluation::GetEvaluationImage(selNode, &pickerImage);
                Log("Texel view Get image\n");
            }
            int width = pickerImage.mWidth;
            int height = pickerImage.mHeight;
            
            ImVec2 pixSize = ImVec2(1.f, -1.f)/displayedTextureSize;

            ImGui::BeginTooltip();
            ImGui::BeginGroup();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImGui::InvisibleButton("AnotherInvisibleMan", ImVec2(120, 120));
            ImRect pickRc(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            draw_list->AddRectFilled(pickRc.Min, pickRc.Max, 0xFF000000);
            static int zoomSize = 2;
            float quadWidth = 120.f / float(zoomSize * 2 + 1);
            ImVec2 quadSize(quadWidth, quadWidth);
            int basex = ImClamp(int(mouseUVCoord.x * width), zoomSize, width-zoomSize);
            int basey = ImClamp(int(mouseUVCoord.y * height) - zoomSize * 2 - 1, zoomSize, height - zoomSize);
            for (int y = -zoomSize; y <= zoomSize; y++)
            {
                for (int x = -zoomSize; x <= zoomSize; x++)
                {
                    uint32_t texel = ((uint32_t*)pickerImage.GetBits())[(basey + zoomSize * 2 + 1 - y) * width + x + basex];
                    ImVec2 pos = pickRc.Min + ImVec2(float(x + zoomSize), float(y + zoomSize)) * quadSize;
                    draw_list->AddRectFilled(pos, pos + quadSize, texel);
                }
            }
            ImVec2 pos = pickRc.Min + ImVec2(float(zoomSize), float(zoomSize)) * quadSize;
            draw_list->AddRect(pos, pos + quadSize, 0xFF0000FF, 0.f, 15, 2.f);
            
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            uint32_t texel = ((uint32_t*)pickerImage.GetBits())[basey * width + basex];
            ImVec4 color = ImColor(texel);
            ImVec4 colHSV;
            ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, colHSV.x, colHSV.y, colHSV.z);
            ImGui::Text("U %1.3f V %1.3f", mouseUVCoord.x, mouseUVCoord.y);
            ImGui::Text("Coord %d %d", int(mouseUVCoord.x * width), int(mouseUVCoord.y * height));
            ImGui::Separator();
            ImGui::Text("R 0x%02x  G 0x%02x  B 0x%02x", int(color.x * 255.f), int(color.y * 255.f), int(color.z * 255.f));
            ImGui::Text("R %1.3f G %1.3f B %1.3f", color.x, color.y, color.z);
            ImGui::Separator();
            ImGui::Text("H 0x%02x  S 0x%02x  V 0x%02x", int(colHSV.x * 255.f), int(colHSV.y * 255.f), int(colHSV.z * 255.f));
            ImGui::Text("H %1.3f S %1.3f V %1.3f", colHSV.x, colHSV.y, colHSV.z);
            ImGui::Separator();
            ImGui::Text("Alpha 0x%02x", int(color.w * 255.f));
            ImGui::Text("Alpha %1.3f", color.w);
            ImGui::Separator();
            ImGui::Text("Size %d, %d", int(displayedTextureSize.x), int(displayedTextureSize.y));
            ImGui::EndGroup();
            ImGui::EndTooltip();
        }
        else if (ImGui::IsWindowFocused())
        {
            Evaluation::FreeImage(&pickerImage);
            ImVec2 ratio((io.MousePos.x - rc.Min.x) / rc.GetSize().x, (io.MousePos.y - rc.Min.y) / rc.GetSize().y);
            ImVec2 deltaRatio((io.MouseDelta.x) / rc.GetSize().x, (io.MouseDelta.y) / rc.GetSize().y);
            nodeGraphDelegate.SetMouse(ratio.x, ratio.y, deltaRatio.x, deltaRatio.y, io.MouseDown[0], io.MouseDown[1], io.MouseWheel);
        }
        lastSentExit = -1;
    }
    else
    {
        if (lastSentExit != selNode)
        {
            lastSentExit = selNode;
            nodeGraphDelegate.SetMouse(-9999.f, -9999.f, -9999.f, -9999.f, false, false, 0.f);
        }
    }
}

void NodeEdit(TileNodeEditGraphDelegate& nodeGraphDelegate, Evaluation& evaluation)
{
    ImGuiIO& io = ImGui::GetIO();

    int selNode = nodeGraphDelegate.mSelectedNodeIndex;
    if (ImGui::CollapsingHeader("Preview", 0, ImGuiTreeNodeFlags_DefaultOpen))
    {
        RenderPreviewNode(selNode, nodeGraphDelegate, evaluation);
    }

    if (selNode == -1)
        ImGui::CollapsingHeader("No Selection", 0, ImGuiTreeNodeFlags_DefaultOpen);
    else
        nodeGraphDelegate.EditNode();
}

template <typename T, typename Ty> struct SortedResource
{
    SortedResource() {}
    SortedResource(unsigned int index, const std::vector<T, Ty>* res) : mIndex(index), mRes(res) {}
    SortedResource(const SortedResource& other) : mIndex(other.mIndex), mRes(other.mRes) {}
    void operator = (const SortedResource& other) { mIndex = other.mIndex; mRes = other.mRes; }
    unsigned int mIndex;
    const std::vector<T, Ty>* mRes;
    bool operator < (const SortedResource& other) const
    {
        return (*mRes)[mIndex].mName<(*mRes)[other.mIndex].mName;
    }

    static void ComputeSortedResources(const std::vector<T, Ty>& res, std::vector<SortedResource>& sortedResources)
    {
        sortedResources.resize(res.size());
        for (unsigned int i = 0; i < res.size(); i++)
            sortedResources[i] = SortedResource<T, Ty>(i, &res);
        std::sort(sortedResources.begin(), sortedResources.end());
    }
};

std::string GetGroup(const std::string &name)
{
    for (int i = int(name.length()) - 1; i >= 0; i--)
    {
        if (name[i] == '/')
        {
            return name.substr(0, i);
        }
    }
    return "";
}

std::string GetName(const std::string &name)
{
    for (int i = int(name.length()) - 1; i >= 0; i--)
    {
        if (name[i] == '/')
        {
            return name.substr(i+1);
        }
    }
    return name;
}

struct PinnedTaskUploadImage : enki::IPinnedTask
{
    PinnedTaskUploadImage(Image *image, ASyncId identifier, bool isThumbnail)
        : enki::IPinnedTask(0) // set pinned thread to 0
        , mImage(image)
        , mIdentifier(identifier)
        , mbIsThumbnail(isThumbnail)
    {
    }

    virtual void Execute()
    {
        unsigned int textureId = Evaluation::UploadImage(mImage, 0);
        if (mbIsThumbnail)
        {
            Material* material = library.Get(mIdentifier);
            if (material)
                material->mThumbnailTextureId = textureId;
        }
        else
        {
            TileNodeEditGraphDelegate::ImogenNode *node = gNodeDelegate.Get(mIdentifier);
            size_t nodeIndex = node - gNodeDelegate.mNodes.data();
            if (node)
            {
                Evaluation::SetEvaluationImage(int(nodeIndex), mImage);
                gEvaluation.SetEvaluationParameters(nodeIndex, node->mParameters);
                gCurrentContext->StageSetProcessing(nodeIndex, false);
            }
            Evaluation::FreeImage(mImage);
        }
    }
    Image *mImage;
    ASyncId mIdentifier;
    bool mbIsThumbnail;
};

struct DecodeThumbnailTaskSet : enki::ITaskSet
{
    DecodeThumbnailTaskSet(std::vector<uint8_t> *src, ASyncId identifier) : enki::ITaskSet(), mIdentifier(identifier), mSrc(src)
    {
    }
    virtual void    ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum)
    {
        Image image;
        int components;
        unsigned char *data = stbi_load_from_memory(mSrc->data(), int(mSrc->size()), &image.mWidth, &image.mHeight, &components, 0);
        if (data)
        {
            image.SetBits(data);
            image.mNumFaces = 1;
            image.mNumMips = 1;
            image.mFormat = (components == 4) ? TextureFormat::RGBA8 : TextureFormat::RGB8;
            PinnedTaskUploadImage uploadTexTask(&image, mIdentifier, true);
            g_TS.AddPinnedTask(&uploadTexTask);
            g_TS.WaitforTask(&uploadTexTask);
            Evaluation::FreeImage(&image);
        }
        delete this;
    }
    ASyncId mIdentifier;
    std::vector<uint8_t> *mSrc;
};

struct EncodeImageTaskSet : enki::ITaskSet
{
    EncodeImageTaskSet(Image image, ASyncId materialIdentifier, ASyncId nodeIdentifier) : enki::ITaskSet(), mMaterialIdentifier(materialIdentifier), mNodeIdentifier(nodeIdentifier), mImage(image)
    {
    }
    virtual void    ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum)
    {
        int outlen;
        int components = 4;
        unsigned char *bits = stbi_write_png_to_mem((unsigned char*)mImage.GetBits(), mImage.mWidth * components, mImage.mWidth, mImage.mHeight, components, &outlen);
        if (bits)
        {
            Material *material = library.Get(mMaterialIdentifier);
            if (material)
            {
                MaterialNode *node = material->Get(mNodeIdentifier);
                if (node)
                {
                    node->mImage.resize(outlen);
                    memcpy(node->mImage.data(), bits, outlen);
                }
            }
        }
        delete this;
    }
    ASyncId mMaterialIdentifier;
    ASyncId mNodeIdentifier;
    Image mImage;
};

struct DecodeImageTaskSet : enki::ITaskSet
{
    DecodeImageTaskSet(std::vector<uint8_t> *src, ASyncId identifier) : enki::ITaskSet(), mIdentifier(identifier), mSrc(src)
    {
    }
    virtual void    ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum)
    {
        Image image;
        int components;
        unsigned char *data = stbi_load_from_memory(mSrc->data(), int(mSrc->size()), &image.mWidth, &image.mHeight, &components, 0);
        if (data)
        {
            image.SetBits(data);
            image.mNumFaces = 1;
            image.mNumMips = 1;
            image.mFormat = (components == 3) ? TextureFormat::RGB8 : TextureFormat::RGBA8;
            PinnedTaskUploadImage uploadTexTask(&image, mIdentifier, false);
            g_TS.AddPinnedTask(&uploadTexTask);
            g_TS.WaitforTask(&uploadTexTask);
        }
        delete this;
    }
    ASyncId mIdentifier;
    std::vector<uint8_t> *mSrc;
};

template <typename T, typename Ty> bool TVRes(std::vector<T, Ty>& res, const char *szName, int &selection, int index, Evaluation& evaluation, int viewMode)
{
    bool ret = false;
    if (!ImGui::TreeNodeEx(szName, ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_DefaultOpen))
        return ret;

    std::string currentGroup = "";
    bool currentGroupIsSkipped = false;

    std::vector<SortedResource<T, Ty>> sortedResources;
    SortedResource<T, Ty>::ComputeSortedResources(res, sortedResources);
    unsigned int defaultTextureId = evaluation.GetTexture("Stock/thumbnail-icon.png");
    float regionWidth = ImGui::GetWindowContentRegionWidth();
    float currentStep = 0.f;
    float stepSize = (viewMode == 2) ? 64.f : 128.f;
    for (const auto& sortedRes : sortedResources)
    {
        unsigned int indexInRes = sortedRes.mIndex;
        bool selected = ((selection >> 16) == index) && (selection & 0xFFFF) == (int)indexInRes;
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (selected ? ImGuiTreeNodeFlags_Selected : 0);

        std::string grp = GetGroup(res[indexInRes].mName);

        if ( grp != currentGroup)
        {
            if (currentGroup.length() && !currentGroupIsSkipped)
                ImGui::TreePop();

            currentGroup = grp;
            currentStep = 0.f;
            if (currentGroup.length())
            {
                if (!ImGui::TreeNode(currentGroup.c_str()))
                {
                    currentGroupIsSkipped = true;
                    continue;
                }
            }
            currentGroupIsSkipped = false;
        }
        else if (currentGroupIsSkipped)
            continue;

        if ((regionWidth - currentStep) < stepSize)
        {
            currentStep = 0.f;
        }
        if (viewMode == 2 || viewMode == 3)
        {
            if (currentStep > FLT_EPSILON)
            {
                ImGui::SameLine();
            }
        }

        ImGui::BeginGroup();

        T& resource = res[indexInRes];
        if (!resource.mThumbnailTextureId)
        {
            resource.mThumbnailTextureId = defaultTextureId;
            g_TS.AddTaskSetToPipe(new DecodeThumbnailTaskSet(&resource.mThumbnail, std::make_pair(indexInRes,resource.mRuntimeUniqueId)));
        }
        bool clicked = false;
        switch (viewMode)
        {
        case 0:
            ImGui::TreeNodeEx(GetName(resource.mName).c_str(), node_flags);
            clicked |= ImGui::IsItemClicked();
            break;
        case 1:
            ImGui::Image((ImTextureID)(int64_t)(resource.mThumbnailTextureId), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
            clicked = ImGui::IsItemClicked();
            ImGui::SameLine();
            ImGui::TreeNodeEx(GetName(resource.mName).c_str(), node_flags);
            clicked |= ImGui::IsItemClicked();
            break;
        case 2:
            ImGui::Image((ImTextureID)(int64_t)(resource.mThumbnailTextureId), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
            clicked = ImGui::IsItemClicked();
            break;
        case 3:
            ImGui::Image((ImTextureID)(int64_t)(resource.mThumbnailTextureId), ImVec2(128, 128), ImVec2(0, 1), ImVec2(1, 0));
            clicked = ImGui::IsItemClicked();
            break;
        }
        if (clicked)
        {
            selection = (index << 16) + indexInRes;
            ret = true;
        }
        ImGui::EndGroup();
        currentStep += stepSize;
    }

    if (currentGroup.length() && !currentGroupIsSkipped)
        ImGui::TreePop();

    ImGui::TreePop();
    return ret;
}

static int selectedMaterial = -1;
int Imogen::GetCurrentMaterialIndex()
{
    return selectedMaterial;
}
void ValidateMaterial(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate, int materialIndex)
{
    if (materialIndex == -1)
        return;
    Material& material = library.mMaterials[materialIndex];
    material.mMaterialNodes.resize(nodeGraphDelegate.mNodes.size());

    for (size_t i = 0; i < nodeGraphDelegate.mNodes.size(); i++)
    {
        TileNodeEditGraphDelegate::ImogenNode srcNode = nodeGraphDelegate.mNodes[i];
        MaterialNode &dstNode = material.mMaterialNodes[i];
        MetaNode& metaNode = gMetaNodes[srcNode.mType];
        dstNode.mRuntimeUniqueId = GetRuntimeId();
        if (metaNode.mbSaveTexture)
        {
            Image image;
            if (Evaluation::GetEvaluationImage(int(i), &image) == EVAL_OK)
            {
                g_TS.AddTaskSetToPipe(new EncodeImageTaskSet(image, std::make_pair(materialIndex, material.mRuntimeUniqueId), std::make_pair(i, dstNode.mRuntimeUniqueId)));
            }
        }

        dstNode.mType = uint32_t(srcNode.mType);
        dstNode.mTypeName = metaNode.mName;
        dstNode.mParameters = srcNode.mParameters;
        dstNode.mInputSamplers = srcNode.mInputSamplers;
        ImVec2 nodePos = NodeGraphGetNodePos(i);
        dstNode.mPosX = int32_t(nodePos.x);
        dstNode.mPosY = int32_t(nodePos.y);
        dstNode.mFrameStart = srcNode.mStartFrame;
        dstNode.mFrameEnd = srcNode.mEndFrame;
    }
    auto links = NodeGraphGetLinks();
    material.mMaterialConnections.resize(links.size());
    for (size_t i = 0; i < links.size(); i++)
    {
        MaterialConnection& materialConnection = material.mMaterialConnections[i];
        materialConnection.mInputNode = links[i].InputIdx;
        materialConnection.mInputSlot = links[i].InputSlot;
        materialConnection.mOutputNode = links[i].OutputIdx;
        materialConnection.mOutputSlot = links[i].OutputSlot;
    }
    auto rugs = NodeGraphRugs();
    material.mMaterialRugs.resize(rugs.size());
    for (size_t i = 0; i < rugs.size(); i++)
    {
        MaterialNodeRug& rug = material.mMaterialRugs[i];
        rug.mPosX = int32_t(rugs[i].mPos.x);
        rug.mPosY = int32_t(rugs[i].mPos.y);
        rug.mSizeX = int32_t(rugs[i].mSize.x);
        rug.mSizeY = int32_t(rugs[i].mSize.y);
        rug.mColor = rugs[i].mColor;
        rug.mComment = rugs[i].mText;
    }
    material.mAnimTrack = nodeGraphDelegate.GetAnimTrack();
    material.mFrameMin = nodeGraphDelegate.mFrameMin;
    material.mFrameMax = nodeGraphDelegate.mFrameMax;
}
void UpdateNewlySelectedGraph(TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
    // set new
    if (selectedMaterial != -1)
    {
        ClearAll(nodeGraphDelegate, evaluation);

        Material& material = library.mMaterials[selectedMaterial];
        for (size_t i = 0; i < material.mMaterialNodes.size(); i++)
        {
            MaterialNode& node = material.mMaterialNodes[i];
            NodeGraphAddNode(&nodeGraphDelegate, node.mType, node.mParameters, node.mPosX, node.mPosY, node.mFrameStart, node.mFrameEnd);
            if (!node.mImage.empty())
            {
                TileNodeEditGraphDelegate::ImogenNode& lastNode = nodeGraphDelegate.mNodes.back();
                gCurrentContext->StageSetProcessing(i, true);
                g_TS.AddTaskSetToPipe(new DecodeImageTaskSet(&node.mImage, std::make_pair(i, lastNode.mRuntimeUniqueId)));
            }
        }
        for (size_t i = 0; i < material.mMaterialConnections.size(); i++)
        {
            MaterialConnection& materialConnection = material.mMaterialConnections[i];
            NodeGraphAddLink(&nodeGraphDelegate, materialConnection.mInputNode, materialConnection.mInputSlot, materialConnection.mOutputNode, materialConnection.mOutputSlot);
        }
        for (size_t i = 0; i < material.mMaterialRugs.size(); i++)
        {
            MaterialNodeRug& rug = material.mMaterialRugs[i];
            NodeGraphAddRug(rug.mPosX, rug.mPosY, rug.mSizeX, rug.mSizeY, rug.mColor, rug.mComment);
        }
        NodeGraphUpdateEvaluationOrder(&nodeGraphDelegate);
        NodeGraphUpdateScrolling();
        nodeGraphDelegate.SetAnimTrack(material.mAnimTrack);
        nodeGraphDelegate.mFrameMin = material.mFrameMin;
        nodeGraphDelegate.mFrameMax = material.mFrameMax;
        nodeGraphDelegate.mEditingContext.RunAll();
    }
}

void LibraryEdit(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
    int previousSelection = selectedMaterial;
    if (ImGui::Button("New Graph"))
    {
        library.mMaterials.push_back(Material());
        Material& back = library.mMaterials.back();
        back.mName = "Name_Of_New_Graph";
        back.mThumbnailTextureId = 0;
        back.mRuntimeUniqueId = GetRuntimeId();
        
        if (previousSelection != -1)
        {
            ValidateMaterial(library, nodeGraphDelegate, previousSelection);
        }
        selectedMaterial = int(library.mMaterials.size()) - 1;
        ClearAll(nodeGraphDelegate, evaluation);
    }
    ImGui::SameLine();
    if (ImGui::Button("Import"))
    {
        nfdchar_t *outPath = NULL;
        nfdresult_t result = NFD_OpenDialog("imogen", NULL, &outPath);

        if (result == NFD_OKAY)
        {
            Library tempLibrary;
            LoadLib(&tempLibrary, outPath);
            for (auto& material : tempLibrary.mMaterials)
            {
                Log("Importing Graph %s\n", material.mName.c_str());
                library.mMaterials.push_back(material);
            }
            free(outPath);
        }
    }
    ImGui::SameLine();
    static int libraryViewMode = 1;
    unsigned int libraryViewTextureId = evaluation.GetTexture("Stock/library-view.png");
    static const ImVec2 iconSize(16.f, 16.f);
    for (int i = 0; i < 4; i++)
    {
        if (i)
            ImGui::SameLine();
        ImGui::PushID(99 + i);
        if (ImGui::ImageButton((ImTextureID)(int64_t)libraryViewTextureId, iconSize, ImVec2(float(i)*0.25f, 0.f), ImVec2(float(i + 1)*0.25f, 1.f), -1, ImVec4(0.f, 0.f, 0.f, 0.f)
            , (i == libraryViewMode) ? ImVec4(1.f, 0.3f, 0.1f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f)))
            libraryViewMode = i;
        ImGui::PopID();
    }

    ImGui::BeginChild("TV");
    if (TVRes(library.mMaterials, "Graphs", selectedMaterial, 0, evaluation, libraryViewMode))
    {
        nodeGraphDelegate.mSelectedNodeIndex = -1;
        // save previous
        if (previousSelection != -1)
        {
            ValidateMaterial(library, nodeGraphDelegate, previousSelection);
        }
        UpdateNewlySelectedGraph(nodeGraphDelegate, evaluation);
    }
    ImGui::EndChild();
}

struct AnimCurveEdit : public ImCurveEdit::Delegate
{
    AnimCurveEdit(const ImVec2 min, const ImVec2 max, std::vector<AnimTrack>& animTrack, std::vector<bool>& visible, int nodeIndex) : 
        mMin(min), mMax(max), mAnimTrack(animTrack), mbVisible(visible), mNodeIndex(nodeIndex)
    {
        size_t type = gNodeDelegate.mNodes[nodeIndex].mType;
        const MetaNode& metaNode = gMetaNodes[type];
        std::vector<bool> parameterAddressed(metaNode.mParams.size(), false);

        auto curveForParameter = [&](int parameterIndex, ConTypes type, AnimationBase * animation) {
            const std::string & parameterName = metaNode.mParams[parameterIndex].mName;
            size_t curveCountPerParameter = GetCurveCountPerParameterType(type);
            parameterAddressed[parameterIndex] = true;
            for (size_t curveIndex = 0; curveIndex < curveCountPerParameter; curveIndex++)
            {
                std::vector<ImVec2> curvePts;
                if (!animation || animation->mFrames.empty())
                {
                    curvePts.resize(2);
                    auto& node = gNodeDelegate.mNodes[mNodeIndex];
                    float value = gNodeDelegate.GetParameterComponentValue(nodeIndex, parameterIndex, int(curveIndex));
                    curvePts[0] = ImVec2(float(node.mStartFrame) + 0.5f, value);
                    curvePts[1] = ImVec2(float(node.mEndFrame) + 0.5f, value);
                }
                else
                {
                    size_t ptCount = animation->mFrames.size();
                    curvePts.resize(ptCount);
                    for (size_t i = 0; i < ptCount; i++)
                    {
                        curvePts[i] = ImVec2(float(animation->mFrames[i] + 0.5f), animation->GetFloatValue(uint32_t(i), int(curveIndex)));
                    }
                }
                mPts.push_back(curvePts);
                mValueType.push_back(type);
                mParameterIndex.push_back(parameterIndex);
                mComponentIndex.push_back(uint32_t(curveIndex));
                mLabels.push_back(parameterName + GetCurveParameterSuffix(type, int(curveIndex)));
            }
        };

        for (auto& track : mAnimTrack)
        {
            if (track.mNodeIndex != nodeIndex)
                continue;
            curveForParameter(track.mParamIndex, ConTypes(track.mValueType), track.mAnimation);
        }
        // non keyed parameters
        for (size_t param = 0; param < metaNode.mParams.size(); param++)
        {
            if (parameterAddressed[param])
                continue;
            curveForParameter(int(param), metaNode.mParams[param].mType, nullptr);
        }
    }

    void BakeValuesToAnimationTrack()
    {
        size_t type = gNodeDelegate.mNodes[mNodeIndex].mType;
        const MetaNode& metaNode = gMetaNodes[type];

        auto iter = mAnimTrack.begin();
        for (; iter != mAnimTrack.end();)
        {
            if (iter->mNodeIndex == mNodeIndex)
            {
                delete iter->mAnimation;
                iter = mAnimTrack.erase(iter);
            }
            else
                ++iter;
        }


        for (size_t curve = 0; curve < mPts.size(); curve++)
        {
            AnimTrack animTrack;
            animTrack.mNodeIndex = mNodeIndex;
            animTrack.mParamIndex = mParameterIndex[curve];
            animTrack.mValueType = mValueType[curve];
            animTrack.mAnimation = AllocateAnimation(mValueType[curve]);
            size_t keyCount = mPts[curve].size();
            auto anim = animTrack.mAnimation;
            anim->mFrames.resize(keyCount);
            anim->Allocate(keyCount);
            for (size_t key = 0; key < keyCount; key++)
            {
                ImVec2 keyValue = mPts[curve][key];
                anim->mFrames[key] = uint32_t(keyValue.x);
                anim->SetFloatValue(uint32_t(key), mComponentIndex[curve], keyValue.y);
            }
            mAnimTrack.push_back(animTrack);
        }
        gNodeDelegate.ApplyAnimationForNode(mNodeIndex, gEvaluationTime);
    }

    virtual ImVec2 GetRange() { return mMax - mMin; }
    virtual ImVec2 GetMin() { return mMin; }
    virtual unsigned int GetBackgroundColor() { return 0x00202020; }
    virtual bool IsVisible(size_t curveIndex) { return mbVisible[curveIndex]; }
    size_t GetCurveCount() { return mPts.size(); }
    size_t GetPointCount(size_t curveIndex) { return mPts[curveIndex].size(); }
    uint32_t GetCurveColor(size_t curveIndex) { return 0xFFAAAAAA; }
    ImVec2* GetPoints(size_t curveIndex) { return mPts[curveIndex].data(); }

    virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
    {
        mPts[curveIndex][pointIndex] = value;
        SortValues(curveIndex);
        for (size_t i = 0; i < GetPointCount(curveIndex); i++)
        {
            if (mPts[curveIndex][i].x == value.x)
                return int(i);
        }
        return pointIndex;
    }

    virtual void AddPoint(size_t curveIndex, ImVec2 value)
    {
        mPts[curveIndex].push_back(value);
        SortValues(curveIndex);
    }

    virtual void DelPoint(size_t curveIndex, size_t pointIndex)
    {
        mPts[curveIndex].erase(mPts[curveIndex].begin() + pointIndex);
        SortValues(curveIndex);
    }

    virtual void BeginEditing() 
    {
    }

    virtual void EndEditing() 
    {
    }

    std::vector<std::vector<ImVec2>> mPts;
    std::vector<std::string> mLabels;
    std::vector<AnimTrack>& mAnimTrack;
    std::vector<uint32_t> mValueType;
    std::vector<uint32_t> mParameterIndex;
    std::vector<uint32_t> mComponentIndex;
    std::vector<bool>& mbVisible;
    ImVec2 mMin, mMax;
    int mNodeIndex;

private:
    void SortValues(size_t curveIndex)
    {
        auto b = std::begin(mPts[curveIndex]);
        auto e = std::end(mPts[curveIndex]);
        std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });
        BakeValuesToAnimationTrack();
    }
};

struct MySequence : public ImSequencer::SequenceInterface
{
    MySequence(TileNodeEditGraphDelegate &nodeGraphDelegate) : mNodeGraphDelegate(nodeGraphDelegate) {}

    void Clear()
    {
        mbExpansions.clear();
        mbVisible.clear();
        mLastCustomDrawIndex = -1;
    }
    virtual int GetFrameMin() const { return gNodeDelegate.mFrameMin; }
    virtual int GetFrameMax() const { return gNodeDelegate.mFrameMax; }

    virtual int GetItemCount() const { return (int)gEvaluation.GetStagesCount(); }

    virtual int GetItemTypeCount() const { return 0; }
    virtual const char *GetItemTypeName(int typeIndex) const { return NULL; }
    virtual const char *GetItemLabel(int index) const
    {
        size_t nodeType = gEvaluation.GetStageType(index);
        return gMetaNodes[nodeType].mName.c_str();
    }

    virtual void Get(int index, int** start, int** end, int *type, unsigned int *color)
    {
        size_t nodeType = gEvaluation.GetStageType(index);

        if (color)
            *color = gMetaNodes[nodeType].mHeaderColor;
        if (start)
            *start = &mNodeGraphDelegate.mNodes[index].mStartFrame;
        if (end)
            *end = &mNodeGraphDelegate.mNodes[index].mEndFrame;
        if (type)
            *type = int(nodeType);
    }
    virtual void Add(int type) { };
    virtual void Del(int index) { }
    virtual void Duplicate(int index) { }

    virtual size_t GetCustomHeight(int index) 
    { 
        if (index >= mbExpansions.size())
            return false;
        return mbExpansions[index]? 300 : 0;
    }
    virtual void DoubleClick(int index) 
    {
        if (index >= mbExpansions.size())
            mbExpansions.resize(index + 1, false);
        if (mbExpansions[index])
        {
            mbExpansions[index] = false;
            return;
        }
        for (auto& item : mbExpansions)
            item = false;
        mbExpansions[index] = !mbExpansions[index];
    }

    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect)
    {
        if (mLastCustomDrawIndex != index)
        {
            mLastCustomDrawIndex = index;
            mbVisible.clear();
        }

        AnimCurveEdit curveEdit(ImVec2(float(gNodeDelegate.mFrameMin), 0.f), ImVec2(float(gNodeDelegate.mFrameMax), 1.f), gNodeDelegate.mAnimTrack, mbVisible, index);
        mbVisible.resize(curveEdit.GetCurveCount(), true);
        draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);
        for (int i = 0; i < curveEdit.GetCurveCount(); i++)
        {
            ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
            ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i + 1) * 14.f);
            draw_list->AddText(pta, mbVisible[i] ? 0xFFFFFFFF : 0x80FFFFFF, curveEdit.mLabels[i].c_str());
            if (ImRect(pta, ptb).Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(0))
                mbVisible[i] = !mbVisible[i];
        }
        draw_list->PopClipRect();

        ImGui::SetCursorScreenPos(rc.Min);
        ImCurveEdit::Edit(curveEdit, rc.Max - rc.Min, 137 + index, &clippingRect);
    }

    TileNodeEditGraphDelegate &mNodeGraphDelegate;
    std::vector<bool> mbExpansions;
    std::vector<bool> mbVisible;
    int mLastCustomDrawIndex;
};
MySequence mySequence(gNodeDelegate);

void Imogen::Show(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    if (ImGui::Begin("Imogen", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
        ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);

        if (ImGui::Begin("Nodes"))
        {
            if (selectedMaterial != -1)
            {
                Material& material = library.mMaterials[selectedMaterial];
                ImGui::PushItemWidth(150);
                ImGui::InputText("Name", &material.mName);
                ImGui::SameLine();
                ImGui::PopItemWidth();


                ImGui::PushItemWidth(60);
                static int previewSize = 0;
                //ImGui::Combo("Preview size", &previewSize, "  128\0  256\0  512\0 1024\0 2048\0 4096\0");
                //ImGui::SameLine();
                if (ImGui::Button("Do exports"))
                {
                    nodeGraphDelegate.DoForce();
                }
                ImGui::SameLine();
                if (ImGui::Button("Save Graph"))
                {
                    nfdchar_t *outPath = NULL;
                    nfdresult_t result = NFD_SaveDialog("imogen", NULL, &outPath);

                    if (result == NFD_OKAY)
                    {
                        Library tempLibrary;
                        Material& material = library.mMaterials[selectedMaterial];
                        tempLibrary.mMaterials.push_back(material);
                        SaveLib(&tempLibrary, outPath);
                        Log("Graph %s saved at path %s\n", material.mName.c_str(), outPath);
                        free(outPath);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete Graph"))
                {
                    library.mMaterials.erase(library.mMaterials.begin() + selectedMaterial);
                    selectedMaterial = int(library.mMaterials.size()) - 1;
                    UpdateNewlySelectedGraph(nodeGraphDelegate, evaluation);
                }
                ImGui::PopItemWidth();
            }
            NodeGraph(&nodeGraphDelegate, selectedMaterial != -1);    
        }
        ImGui::End();

        if (ImGui::Begin("Shaders"))
        {
            HandleEditor(editor, nodeGraphDelegate, evaluation);
        }
        ImGui::End();

        if (ImGui::Begin("Library"))
        {
            LibraryEdit(library, nodeGraphDelegate, evaluation);
        }
        ImGui::End();

        ImGui::SetWindowSize(ImVec2(300, 300));
        if (ImGui::Begin("Parameters"))
        {
            const ImVec2 windowPos = ImGui::GetCursorScreenPos();
            const ImVec2 canvasSize = ImGui::GetContentRegionAvail();

            NodeEdit(nodeGraphDelegate, evaluation);
            if (ImGui::IsWindowFocused())
            {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddRect(windowPos-ImVec2(4,6), windowPos+canvasSize+ ImVec2(4, 6), 0xCCCC0000);
            }
            
        }
        ImGui::End();

        if (ImGui::Begin("Logs"))
        {
            ImguiAppLog::Log->DrawEmbedded();
        }
        ImGui::End();

        if (ImGui::Begin("Timeline"))
        {
            int selectedEntry = nodeGraphDelegate.mSelectedNodeIndex;
            static int firstFrame = 0;
            int currentTime = gEvaluationTime;

            ImGui::PushItemWidth(80);
            ImGui::PushID(200);
            ImGui::InputInt("", &gNodeDelegate.mFrameMin, 0, 0);
            ImGui::PopID();
            ImGui::SameLine();
            if (ImGui::Button("|<"))
                currentTime = gNodeDelegate.mFrameMin;
            ImGui::SameLine();
            if (ImGui::Button("<"))
                currentTime--;
            ImGui::SameLine();
            
            ImGui::PushID(201);
            if (ImGui::InputInt("", &currentTime, 0, 0, 0))
            {
            }
            ImGui::PopID();
            
            ImGui::SameLine();
            if (ImGui::Button(">"))
                currentTime++;
            ImGui::SameLine();
            if (ImGui::Button(">|"))
                currentTime = gNodeDelegate.mFrameMax;
            ImGui::SameLine();
            extern bool gbIsPlaying;
            if (ImGui::Button(gbIsPlaying ? "Stop" : "Play"))
            {
                if (!gbIsPlaying)
                {
                    currentTime = gNodeDelegate.mFrameMin;
                }
                gbIsPlaying = !gbIsPlaying;
            }
            extern bool gPlayLoop;
            unsigned int playNoLoopTextureId = evaluation.GetTexture("Stock/PlayNoLoop.png");
            unsigned int playLoopTextureId = evaluation.GetTexture("Stock/PlayLoop.png");

            ImGui::SameLine();
            if (ImGui::ImageButton((ImTextureID)(uint64_t)(gPlayLoop ? playLoopTextureId : playNoLoopTextureId), ImVec2(16.f, 16.f)))
                gPlayLoop = !gPlayLoop;

            ImGui::SameLine();
            ImGui::PushID(202);
            ImGui::InputInt("", &gNodeDelegate.mFrameMax, 0, 0);
            ImGui::PopID();
            ImGui::SameLine();
            currentTime = ImMax(currentTime, 0);
            ImGui::SameLine(0, 40.f);
            if (ImGui::Button("Make Key") && selectedEntry != -1)
            {
                nodeGraphDelegate.MakeKey(currentTime, uint32_t(selectedEntry), 0);
            }

            ImGui::SameLine(0, 50.f);
            
            float keyValue[2];
            ImGui::PushID(203);
            if (ImGui::InputFloat("", &keyValue[0]))
            {

            }
            ImGui::SameLine();
            if (ImGui::InputFloat("Key", &keyValue[1]))
            {

            }
            ImGui::PopID();
            ImGui::SameLine();
            int timeMask[2] = { 0,0 };
            if (selectedEntry != -1)
            {
                timeMask[0] = gNodeDelegate.mNodes[selectedEntry].mStartFrame;
                timeMask[1] = gNodeDelegate.mNodes[selectedEntry].mEndFrame;
            }
            ImGui::PushItemWidth(120);
            if (ImGui::InputInt2("Time Mask", timeMask) && selectedEntry != -1)
            {
                gNodeDelegate.SetTimeSlot(selectedEntry, timeMask[0], timeMask[1]);
            }
            ImGui::PopItemWidth();
            ImGui::PopItemWidth();


            Sequencer(&mySequence, &currentTime, NULL, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_CHANGE_FRAME);
            if (selectedEntry != -1)
            {
                nodeGraphDelegate.mSelectedNodeIndex = selectedEntry;
                auto& imoNode = nodeGraphDelegate.mNodes[selectedEntry];
                //nodeGraphDelegate.SetTimeSlot(selectedEntry, imoNode.mStartFrame, imoNode.mEndFrame);
                gEvaluation.SetStageLocalTime(selectedEntry, ImClamp(currentTime - imoNode.mStartFrame, 0, imoNode.mEndFrame - imoNode.mStartFrame), true);
            }
            if (currentTime != gEvaluationTime)
            {
                gEvaluationTime = currentTime;
                nodeGraphDelegate.SetTime(currentTime, true);
                nodeGraphDelegate.ApplyAnimation(currentTime);
            }
        }
        ImGui::End();

        // view extraction
        int index = 0;
        int removeExtractedView = -1;
        for (auto& extraction : mExtratedViews)
        {
            char tmps[512];
            sprintf(tmps, "%s_View_%03d", gMetaNodes[nodeGraphDelegate.mNodes[extraction.mNodeIndex].mType].mName.c_str(), index);
            bool open = true;
            if (extraction.mFirstFrame)
            {
                extraction.mFirstFrame = false;
                ImGui::SetNextWindowSize(ImVec2(256, 256));
            }
            //ImGui::SetNextWindowFocus();
            if (ImGui::Begin(tmps, &open))
            {
                RenderPreviewNode(int(extraction.mNodeIndex), nodeGraphDelegate, evaluation, false);
            }
            ImGui::End();
            if (!open)
            {
                removeExtractedView = index;
            }
            index++;
        }
        if (removeExtractedView != -1)
            mExtratedViews.erase(mExtratedViews.begin() + removeExtractedView);

        ImGui::End();
        // --
    }
}

void Imogen::ValidateCurrentMaterial(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate)
{
    ValidateMaterial(library, nodeGraphDelegate, selectedMaterial);
}

void Imogen::DiscoverNodes(const char *extension, const char *directory, EVALUATOR_TYPE evaluatorType, std::vector<EvaluatorFile>& files)
{
    tinydir_dir dir;
    tinydir_open(&dir, directory);

    while (dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&dir, &file);

        if (!file.is_dir && !strcmp(file.extension, extension))
        {
            files.push_back({ directory, file.name, evaluatorType });
        }

        tinydir_next(&dir);
    }

    tinydir_close(&dir);
}

Imogen::Imogen()
{
}

Imogen::~Imogen()
{

}

void Imogen::Init()
{
    SetStyle();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());

    DiscoverNodes("glsl", "Nodes/GLSL/", EVALUATOR_GLSL, mEvaluatorFiles);
    DiscoverNodes("c", "Nodes/C/", EVALUATOR_C, mEvaluatorFiles);
    DiscoverNodes("py", "Nodes/Python/", EVALUATOR_PYTHON, mEvaluatorFiles);
    DiscoverNodes("glsl", "Nodes/GLSLCompute/", EVALUATOR_GLSLCOMPUTE, mEvaluatorFiles);
    DiscoverNodes("glslc", "Nodes/GLSLCompute/", EVALUATOR_GLSLCOMPUTE, mEvaluatorFiles);
}

void Imogen::Finish()
{

}

void ClearAll(TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
    nodeGraphDelegate.Clear();
    evaluation.Clear();
    NodeGraphClear();
    InitCallbackRects();
    ClearExtractedViews();
    gUndoRedoHandler.Clear();
    mySequence.Clear();
}

UndoRedoHandler gUndoRedoHandler;

