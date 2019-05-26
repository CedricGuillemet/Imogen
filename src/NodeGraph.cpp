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

#include "imgui.h"
#include "imgui_internal.h"
#include <math.h>
#include "NodeGraph.h"
#include <vector>
#include <algorithm>
#include <assert.h>
#include "EvaluationStages.h"
#include "imgui_stdlib.h"
#include "NodeGraphControler.h"
#include <array>
#include "imgui_markdown/imgui_markdown.h"
#include "UI.h"
#include "GraphModel.h"


extern ImGui::MarkdownConfig mdConfig;

static inline float Distance(ImVec2& a, ImVec2& b)
{
    return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

ImVec2 GetInputSlotPos(const GraphModel::Node& node, int slot_no, float factor)
{
    const auto& metaNode = gMetaNodes[node.mType];
    ImVec2 Size = ImVec2(metaNode.mWidth, metaNode.mHeight) * factor;
    size_t InputsCount = gMetaNodes[node.mType].mInputs.size();
    return ImVec2(node.mPos.x * factor,
                  node.mPos.y * factor + Size.y * ((float)slot_no + 1) / ((float)InputsCount + 1) + 8.f);
}
ImVec2 GetOutputSlotPos(const GraphModel::Node& node, int slot_no, float factor)
{
    const auto& metaNode = gMetaNodes[node.mType];
    size_t OutputsCount = gMetaNodes[node.mType].mOutputs.size();
    ImVec2 Size = ImVec2(metaNode.mWidth, metaNode.mHeight) * factor;
    return ImVec2(node.mPos.x * factor + Size.x,
                  node.mPos.y * factor + Size.y * ((float)slot_no + 1) / ((float)OutputsCount + 1) + 8.f);
}
ImRect GetNodeRect(const GraphModel::Node& node, float factor)
{
    const auto& metaNode = gMetaNodes[node.mType];
    ImVec2 Size = ImVec2(metaNode.mWidth, metaNode.mHeight) * factor;
    return ImRect(node.mPos * factor, node.mPos * factor + Size);
}

const float NODE_SLOT_RADIUS = 8.0f;
const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

static int editRug = -1;

static ImVec2 editingNodeSource;
bool editingInput = false;
static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
float factor = 1.0f;
float factorTarget = 1.0f;
ImVec2 captureOffset;

enum NodeOperation
{
    NO_None,
    NO_EditingLink,
    NO_QuadSelecting,
    NO_MovingNodes,
    NO_EditInput,
    NO_MovingRug,
    NO_SizingRug,
    NO_PanView,
};
NodeOperation nodeOperation = NO_None;

void HandleZoomScroll(ImRect regionRect)
{
    ImGuiIO& io = ImGui::GetIO();

    if (regionRect.Contains(io.MousePos))
    {
        if (io.MouseWheel < -FLT_EPSILON)
            factorTarget *= 0.9f;

        if (io.MouseWheel > FLT_EPSILON)
            factorTarget *= 1.1f;
    }

    ImVec2 mouseWPosPre = (io.MousePos - ImGui::GetCursorScreenPos()) / factor;
    factorTarget = ImClamp(factorTarget, 0.2f, 3.f);
    factor = ImLerp(factor, factorTarget, 0.15f);
    ImVec2 mouseWPosPost = (io.MousePos - ImGui::GetCursorScreenPos()) / factor;
    if (ImGui::IsMousePosValid())
    {
        scrolling += mouseWPosPost - mouseWPosPre;
    }
}

void NodeGraphClear()
{
    editRug = -1;
    nodeOperation = NO_None;
    factor = 1.0f;
    factorTarget = 1.0f;
}

int DisplayRugs(GraphModel* model, int editRug, ImDrawList* drawList, ImVec2 offset, float factor)
{
    ImGuiIO& io = ImGui::GetIO();
    int ret = editRug;

    const auto& nodes = model->GetNodes();
    const auto& rugs = model->GetRugs();

    // mouse pointer over any node?
    bool overAnyNode = false;
    for (auto& node : nodes)
    {
        const auto& metaNode = gMetaNodes[node.mType];
        ImVec2 node_rect_min = offset + node.mPos * factor;
        ImVec2 node_rect_max = node_rect_min + ImVec2(metaNode.mWidth, metaNode.mHeight) * factor;
        if (ImRect(node_rect_min, node_rect_max).Contains(io.MousePos))
        {
            overAnyNode = true;
            break;
        }
    }

    for (unsigned int rugIndex = 0; rugIndex < rugs.size(); rugIndex++)
    {
        auto& rug = rugs[rugIndex];
        if (rugIndex == editRug)
            continue;
        ImGui::PushID(900 + rugIndex);
        ImVec2 commentSize = rug.mSize * factor;

        ImVec2 node_rect_min = offset + rug.mPos * factor;

        ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);

        ImVec2 node_rect_max = node_rect_min + commentSize;

        ImRect rugRect(node_rect_min, node_rect_max);
        if (rugRect.Contains(io.MousePos) && !overAnyNode)
        {
            if (io.MouseDoubleClicked[0])
            {
                ret = rugIndex;
            }
            else if (io.KeyShift && ImGui::IsMouseClicked(0))
            {
                for (auto i = 0; i < nodes.size(); i++)
                {
                    auto& node = nodes[i];
                    ImVec2 node_rect_min = offset + node.mPos * factor;
                    ImVec2 node_rect_max = node_rect_min + ImVec2(gMetaNodes[node.mType].mWidth, gMetaNodes[node.mType].mHeight) * factor;
                    if (rugRect.Overlaps(ImRect(node_rect_min, node_rect_max)))
                    {
                        model->SelectNode(i);
                    }
                }
            }
        }
        // drawList->AddText(io.FontDefault, 13 * ImLerp(1.f, factor, 0.5f), node_rect_min + ImVec2(5, 5), (rug.mColor &
        // 0xFFFFFF) + 0xFF404040, rug.mText.c_str());
        ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, 0x0);
        ImGui::PushStyleColor(ImGuiCol_Border, 0x0);
        ImGui::BeginChildFrame(
            88 + rugIndex, commentSize - NODE_WINDOW_PADDING * 2, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav);

        ImGui::Markdown(rug.mText.c_str(), rug.mText.length(), mdConfig);
        drawList->AddRectFilled(node_rect_min, node_rect_max, (rug.mColor & 0xFFFFFF) + 0x60000000, 10.0f, 15);
        drawList->AddRect(node_rect_min, node_rect_max, (rug.mColor & 0xFFFFFF) + 0x90000000, 10.0f, 15, 2.f);
        ImGui::EndChildFrame();
        ImGui::PopStyleColor(2);
        ImGui::PopID();
    }
    return ret;
}

bool EditRug(GraphModel* model, int rugIndex, ImDrawList* drawList, ImVec2 offset, float factor)
{
    ImGuiIO& io = ImGui::GetIO();
    const auto& rugs = model->GetRugs();
    ImVec2 commentSize = rugs[rugIndex].mSize * factor;
    static int movingSizingRug = -1;
    GraphModel::Rug rug = rugs[rugIndex];
    static GraphModel::Rug editingRug;
    static GraphModel::Rug editingRugSource;

    bool dirtyRug = false;
    ImVec2 node_rect_min = offset + rug.mPos * factor;
    ImVec2 node_rect_max = node_rect_min + commentSize;
    ImRect rugRect(node_rect_min, node_rect_max);
    ImRect insideSizingRect(node_rect_min + commentSize - ImVec2(30, 30), node_rect_min + commentSize);
    ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);


    drawList->AddRectFilled(node_rect_min, node_rect_max, (rug.mColor & 0xFFFFFF) + 0xE0000000, 10.0f, 15);
    drawList->AddRect(node_rect_min, node_rect_max, (rug.mColor & 0xFFFFFF) + 0xFF000000, 10.0f, 15, 2.f);
    drawList->AddTriangleFilled(node_rect_min + commentSize - ImVec2(25, 8),
                                node_rect_min + commentSize - ImVec2(8, 25),
                                node_rect_min + commentSize - ImVec2(8, 8),
                                (rug.mColor & 0xFFFFFF) + 0x90000000);

    ImGui::SetCursorScreenPos(node_rect_min + ImVec2(5, 5));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
    dirtyRug |= ImGui::InputTextMultiline("", &rug.mText, (node_rect_max - node_rect_min) - ImVec2(30, 30));
    ImGui::PopStyleColor(2);

    ImGui::SetCursorScreenPos(node_rect_min + ImVec2(10, commentSize.y - 30));
    for (int i = 0; i < 7; i++)
    {
        if (i > 0)
            ImGui::SameLine();
        ImGui::PushID(i);
        ImColor buttonColor = ImColor::HSV(i / 7.0f, 0.6f, 0.6f);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)buttonColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(i / 7.0f, 0.8f, 0.8f));
        if (ImGui::Button("   "))
        {
            rug.mColor = buttonColor;
            dirtyRug = true;
        }
        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }
    ImGui::SameLine(0, 50);

    if (ImGui::Button("Delete"))
    {
        model->BeginTransaction(true);
        model->DelRug(rugIndex);
        model->EndTransaction();
        return true;
    }

    bool createUndo = false;
    bool commitUndo = false;

    if (insideSizingRect.Contains(io.MousePos))
	{
        if (nodeOperation == NO_None && ImGui::IsMouseDragging(0, 1))
        {
            movingSizingRug = rugIndex;
            createUndo = true;
            nodeOperation = NO_SizingRug;
            editingRugSource = editingRug = rug;
        }
	}

    if (rugRect.Contains(io.MousePos) && !insideSizingRect.Contains(io.MousePos))
    {
        if (nodeOperation == NO_None && ImGui::IsMouseDragging(0, 1))
        {
            movingSizingRug = rugIndex;
            createUndo = true;
            nodeOperation = NO_MovingRug;
            editingRugSource = editingRug = rug;
        }
	}

    if (movingSizingRug != -1 && !io.MouseDown[0])
    {
        commitUndo = true;
        nodeOperation = NO_None;
    }

    // undo/redo for sizing/moving
    if (commitUndo)
    {
		// set back value 
        model->BeginTransaction(false);
        model->SetRug(movingSizingRug, editingRugSource);
        model->EndTransaction();

		// add undo
        model->BeginTransaction(true);
        model->SetRug(movingSizingRug, editingRug);
        model->EndTransaction();
        movingSizingRug = -1;
    }

    if (dirtyRug)
    {
        model->BeginTransaction(true);
        model->SetRug(rugIndex, rug);
        model->EndTransaction();
    }

    if (movingSizingRug != -1 && ImGui::IsMouseDragging(0))
    {
        if (nodeOperation == NO_MovingRug)
		{
			editingRug.mPos += io.MouseDelta * factor;
		}
		else if (nodeOperation == NO_SizingRug)
		{
            editingRug.mSize += io.MouseDelta * factor;
		}
        model->BeginTransaction(false);
        model->SetRug(movingSizingRug, editingRug);
        model->EndTransaction();
    }

    if ((io.MouseClicked[0] || io.MouseClicked[1]) && !rugRect.Contains(io.MousePos))
        return true;
    return false;
}

void NodeGraphUpdateScrolling(GraphModel* model)
{
    const auto& nodes = model->GetNodes();
    const auto& rugs = model->GetRugs();

    if (nodes.empty() && rugs.empty())
        return;

	if (!nodes.empty())
	{
		scrolling = nodes[0].mPos;
	}
	else if (!rugs.empty())
	{
        scrolling = rugs[0].mPos;
	}
	else
	{
        scrolling = ImVec2(0, 0);
	}
    for (auto& node : nodes)
    {
        scrolling.x = std::min(scrolling.x, node.mPos.x);
        scrolling.y = std::min(scrolling.y, node.mPos.y);
    }
    for (auto& rug : rugs)
    {
        scrolling.x = std::min(scrolling.x, rug.mPos.x);
        scrolling.y = std::min(scrolling.y, rug.mPos.y);
    }

    scrolling = ImVec2(40, 40) - scrolling;
}

static void DisplayLinks(GraphModel* model,
                         ImDrawList* drawList,
                         const ImVec2 offset,
                         const float factor,
                         const ImRect regionRect,
                         int hoveredNode)
{
    const auto& links = model->GetLinks();
    const auto& nodes = model->GetNodes();
    for (int link_idx = 0; link_idx < links.size(); link_idx++)
    {
        const auto* link = &links[link_idx];
        const auto* node_inp = &nodes[link->mInputNodeIndex];
        const auto* node_out = &nodes[link->mOutputNodeIndex];
        ImVec2 p1 = offset + GetOutputSlotPos(*node_inp, link->mInputSlotIndex, factor);
        ImVec2 p2 = offset + GetInputSlotPos(*node_out, link->mOutputSlotIndex, factor);

        // con. view clipping
        if ((p1.y < 0.f && p2.y < 0.f) || (p1.y > regionRect.Max.y && p2.y > regionRect.Max.y) ||
            (p1.x < 0.f && p2.x < 0.f) || (p1.x > regionRect.Max.x && p2.x > regionRect.Max.x))
            continue;

        bool highlightCons = hoveredNode == link->mInputNodeIndex || hoveredNode == link->mOutputNodeIndex;
        uint32_t col = gMetaNodes[node_inp->mType].mHeaderColor | (highlightCons ? 0xF0F0F0 : 0);
        ;
        // curves
        // drawList->AddBezierCurve(p1, p1 + ImVec2(+50, 0) * factor, p2 + ImVec2(-50, 0) * factor, p2, 0xFF000000, 4.f
        // * factor); drawList->AddBezierCurve(p1, p1 + ImVec2(+50, 0) * factor, p2 + ImVec2(-50, 0) * factor, p2,
        // col, 3.0f * factor);

        // -/-
        /*
        ImVec2 p10 = p1 + ImVec2(20.f * factor, 0.f);
        ImVec2 p20 = p2 - ImVec2(20.f * factor, 0.f);

        ImVec2 dif = p20 - p10;
        ImVec2 p1a, p1b;
        if (fabsf(dif.x) > fabsf(dif.y))
        {
            p1a = p10 + ImVec2(fabsf(fabsf(dif.x) - fabsf(dif.y)) * 0.5 * sign(dif.x), 0.f);
            p1b = p1a + ImVec2(fabsf(dif.y) * sign(dif.x) , dif.y);
        }
        else
        {
            p1a = p10 + ImVec2(0.f, fabsf(fabsf(dif.y) - fabsf(dif.x)) * 0.5 * sign(dif.y));
            p1b = p1a + ImVec2(dif.x, fabsf(dif.x) * sign(dif.y));
        }
        drawList->AddLine(p1,  p10, col, 3.f * factor);
        drawList->AddLine(p10, p1a, col, 3.f * factor);
        drawList->AddLine(p1a, p1b, col, 3.f * factor);
        drawList->AddLine(p1b, p20, col, 3.f * factor);
        drawList->AddLine(p20,  p2, col, 3.f * factor);
        */
        std::array<ImVec2, 6> pts;
        int ptCount = 0;
        ImVec2 dif = p2 - p1;

        ImVec2 p1a, p1b;
        const float limitx = 12.f * factor;
        if (dif.x < limitx)
        {
            ImVec2 p10 = p1 + ImVec2(limitx, 0.f);
            ImVec2 p20 = p2 - ImVec2(limitx, 0.f);

            dif = p20 - p10;
            p1a = p10 + ImVec2(0.f, dif.y * 0.5f);
            p1b = p1a + ImVec2(dif.x, 0.f);

            pts = {p1, p10, p1a, p1b, p20, p2};
            ptCount = 6;
        }
        else
        {
            if (fabsf(dif.y) < 1.f)
            {
                pts = {p1, (p1 + p2) * 0.5f, p2};
                ptCount = 3;
            }
            else
            {
                if (fabsf(dif.y) < 10.f)
                {
                    if (fabsf(dif.x) > fabsf(dif.y))
                    {
                        p1a = p1 + ImVec2(fabsf(fabsf(dif.x) - fabsf(dif.y)) * 0.5f * sign(dif.x), 0.f);
                        p1b = p1a + ImVec2(fabsf(dif.y) * sign(dif.x), dif.y);
                    }
                    else
                    {
                        p1a = p1 + ImVec2(0.f, fabsf(fabsf(dif.y) - fabsf(dif.x)) * 0.5f * sign(dif.y));
                        p1b = p1a + ImVec2(dif.x, fabsf(dif.x) * sign(dif.y));
                    }
                }
                else
                {
                    if (fabsf(dif.x) > fabsf(dif.y))
                    {
                        float d = fabsf(dif.y) * sign(dif.x) * 0.5f;
                        p1a = p1 + ImVec2(d, dif.y * 0.5f);
                        p1b = p1a + ImVec2(fabsf(fabsf(dif.x) - fabsf(d) * 2.f) * sign(dif.x), 0.f);
                    }
                    else
                    {
                        float d = fabsf(dif.x) * sign(dif.y) * 0.5f;
                        p1a = p1 + ImVec2(dif.x * 0.5f, d);
                        p1b = p1a + ImVec2(0.f, fabsf(fabsf(dif.y) - fabsf(d) * 2.f) * sign(dif.y));
                    }
                }
                pts = {p1, p1a, p1b, p2};
                ptCount = 4;
            }
        }
        float highLightFactor = factor * highlightCons ? 2.0f : 1.f;
        for (int pass = 0; pass < 2; pass++)
        {
            drawList->AddPolyline(
                pts.data(), ptCount, pass ? col : 0xFF000000, false, (pass ? 5.f : 7.5f) * highLightFactor);
        }
    }
}

static void HandleQuadSelection(
    GraphModel* model, ImDrawList* drawList, const ImVec2 offset, const float factor, ImRect contentRect)
{
    ImGuiIO& io = ImGui::GetIO();
    static ImVec2 quadSelectPos;
    auto& nodes = model->GetNodes();

    ImRect editingRugRect(ImVec2(FLT_MAX, FLT_MAX), ImVec2(FLT_MAX, FLT_MAX));
    if (editRug != -1)
    {
        const auto& rug = model->GetRugs()[editRug];
        ImVec2 commentSize = rug.mSize * factor;
        ImVec2 node_rect_min = (offset + rug.mPos) * factor;
        ImVec2 node_rect_max = node_rect_min + commentSize;
        editingRugRect = ImRect(node_rect_min, node_rect_max);
    }

    if (nodeOperation == NO_QuadSelecting && ImGui::IsWindowFocused())
    {
        const ImVec2 bmin = ImMin(quadSelectPos, io.MousePos);
        const ImVec2 bmax = ImMax(quadSelectPos, io.MousePos);
        drawList->AddRectFilled(bmin, bmax, 0x40FF2020, 1.f);
        drawList->AddRect(bmin, bmax, 0xFFFF2020, 1.f);
        if (!io.MouseDown[0])
        {
            if (!io.KeyCtrl && !io.KeyShift)
            {
                for (int nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
                {
                    model->SelectNode(nodeIndex, false);
                }
            }

            nodeOperation = NO_None;
            ImRect selectionRect(bmin, bmax);
            for (int nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
            {
                const auto* node = &nodes[nodeIndex];
                ImVec2 node_rect_min = offset + node->mPos * factor;
                ImVec2 node_rect_max = node_rect_min + ImVec2(gMetaNodes[node->mType].mWidth, gMetaNodes[node->mType].mHeight) * factor;
                if (selectionRect.Overlaps(ImRect(node_rect_min, node_rect_max)))
                {
                    if (io.KeyCtrl)
                    {
                        model->SelectNode(nodeIndex, false);
                    }
                    else
                    {
                        model->SelectNode(nodeIndex, true);
                    }
                }
                else
                {
                    if (!io.KeyShift)
                    {
                        model->SelectNode(nodeIndex, false);
                    }
                }
            }
        }
    }
    else if (nodeOperation == NO_None && io.MouseDown[0] && ImGui::IsWindowFocused() &&
             contentRect.Contains(io.MousePos) && !editingRugRect.Contains(io.MousePos))
    {
        nodeOperation = NO_QuadSelecting;
        quadSelectPos = io.MousePos;
    }
}


bool HandleConnections(GraphModel* model,
                       ImDrawList* drawList,
                       int nodeIndex,
                       const ImVec2 offset,
                       const float factor,
                       NodeGraphControlerBase* controler,
                       bool bDrawOnly)
{
    static int editingNodeIndex;
    static int editingSlotIndex;
    const auto& links = model->GetLinks();
    const auto& nodes = model->GetNodes();

    size_t metaNodeCount = gMetaNodes.size();
    const MetaNode* metaNodes = gMetaNodes.data();
    ImGuiIO& io = ImGui::GetIO();
    const GraphModel::Node* node = &nodes[nodeIndex];

    size_t InputsCount = gMetaNodes[node->mType].mInputs.size();
    size_t OutputsCount = gMetaNodes[node->mType].mOutputs.size();

    // draw/use inputs/outputs
    bool hoverSlot = false;
    for (int i = 0; i < 2; i++)
    {
        float closestDistance = FLT_MAX;
        int closestConn = -1;
        ImVec2 closestTextPos;
        ImVec2 closestPos;
        const size_t slotCount[2] = {InputsCount, OutputsCount};
        const MetaCon* con = i ? metaNodes[node->mType].mOutputs.data() : metaNodes[node->mType].mInputs.data();
        for (int slot_idx = 0; slot_idx < slotCount[i]; slot_idx++)
        {
            ImVec2 p =
                offset + (i ? GetOutputSlotPos(*node, slot_idx, factor) : GetInputSlotPos(*node, slot_idx, factor));
            float distance = Distance(p, io.MousePos);
            bool overCon = (nodeOperation == NO_None || nodeOperation == NO_EditingLink) &&
                           (distance < NODE_SLOT_RADIUS * 2.f) && (distance < closestDistance);

            const char* conText = con[slot_idx].mName.c_str();
            ImVec2 textSize;
            textSize = ImGui::CalcTextSize(conText);
            ImVec2 textPos =
                p + ImVec2(-NODE_SLOT_RADIUS * (i ? -1.f : 1.f) * (overCon ? 3.f : 2.f) - (i ? 0 : textSize.x),
                           -textSize.y / 2);

            ImRect nodeRect = GetNodeRect(*node, factor);
            if (overCon || (nodeRect.Contains(io.MousePos - offset) && closestConn == -1 &&
                            (editingInput == (i != 0)) && nodeOperation == NO_EditingLink))
            {
                closestDistance = distance;
                closestConn = slot_idx;
                closestTextPos = textPos;
                closestPos = p;
            }

            drawList->AddCircleFilled(p, NODE_SLOT_RADIUS * 1.2f, IM_COL32(0, 0, 0, 200));
            drawList->AddCircleFilled(p, NODE_SLOT_RADIUS * 0.75f * 1.2f, IM_COL32(160, 160, 160, 200));
            drawList->AddText(io.FontDefault, 14, textPos + ImVec2(2, 2), IM_COL32(0, 0, 0, 255), conText);
            drawList->AddText(io.FontDefault, 14, textPos, IM_COL32(150, 150, 150, 255), conText);
        }

        if (closestConn != -1)
        {
            const char* conText = con[closestConn].mName.c_str();

            hoverSlot = true;
            drawList->AddCircleFilled(closestPos, NODE_SLOT_RADIUS * 2.f, IM_COL32(0, 0, 0, 200));
            drawList->AddCircleFilled(closestPos, NODE_SLOT_RADIUS * 1.5f, IM_COL32(200, 200, 200, 200));
            drawList->AddText(io.FontDefault, 16, closestTextPos + ImVec2(1, 1), IM_COL32(0, 0, 0, 255), conText);
            drawList->AddText(io.FontDefault, 16, closestTextPos, IM_COL32(250, 250, 250, 255), conText);
            bool inputToOutput = (!editingInput && !i) || (editingInput && i);
            if (nodeOperation == NO_EditingLink && !io.MouseDown[0] && !bDrawOnly)
            {
                if (inputToOutput)
                {
                    // check loopback
                    GraphModel::Link nl;
                    if (editingInput)
                        nl = GraphModel::Link{nodeIndex, closestConn, editingNodeIndex, editingSlotIndex};
                    else
                        nl = GraphModel::Link{editingNodeIndex, editingSlotIndex, nodeIndex, closestConn};

                    if (model->RecurseIsLinked(nl.mOutputNodeIndex, nl.mInputNodeIndex))
                    {
                        Log("Acyclic graph. Loop is not allowed.\n");
                        break;
                    }
                    bool alreadyExisting = false;
                    for (int linkIndex = 0; linkIndex < links.size(); linkIndex++)
                    {
                        if (links[linkIndex] == nl)
                        {
                            alreadyExisting = true;
                            break;
                        }
                    }

                    // check already connected output
                    for (int linkIndex = 0; linkIndex < links.size(); linkIndex++)
                    {
                        auto& link = links[linkIndex];
                        if (link.mOutputNodeIndex == nl.mOutputNodeIndex && link.mOutputSlotIndex == nl.mOutputSlotIndex)
                        {
                            model->BeginTransaction(true);
                            model->DelLink(linkIndex);
                            model->EndTransaction();
                            break;
                        }
                    }

                    if (!alreadyExisting)
                    {
                        model->BeginTransaction(true);
                        model->AddLink(nl.mInputNodeIndex, nl.mInputSlotIndex, nl.mOutputNodeIndex, nl.mOutputSlotIndex);
                        model->EndTransaction();
                    }
                }
            }
            // when ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() is uncommented, one can't click the node
            // input/output when mouse is over the node itself.
            if (nodeOperation == NO_None &&
                /*ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() &&*/ io.MouseClicked[0] && !bDrawOnly)
            {
                nodeOperation = NO_EditingLink;
                editingInput = i == 0;
                editingNodeSource = closestPos;
                editingNodeIndex = nodeIndex;
                editingSlotIndex = closestConn;
                if (editingInput)
                {
                    // remove existing link
                    for (int linkIndex = 0; linkIndex < links.size(); linkIndex++)
                    {
                        auto& link = links[linkIndex];
                        if (link.mOutputNodeIndex == nodeIndex && link.mOutputSlotIndex == closestConn)
                        {
                            model->BeginTransaction(true);
                            model->DelLink(linkIndex);
                            model->EndTransaction();
                            break;
                        }
                    }
                }
            }
        }
    }
    return hoverSlot;
}

static void DrawGrid(ImDrawList* drawList, ImVec2 windowPos, const ImVec2 canvasSize, const float factor)
{
    ImU32 GRID_COLOR = IM_COL32(100, 100, 100, 40);
    float GRID_SZ = 64.0f * factor;
    for (float x = fmodf(scrolling.x * factor, GRID_SZ); x < canvasSize.x; x += GRID_SZ)
        drawList->AddLine(ImVec2(x, 0.0f) + windowPos, ImVec2(x, canvasSize.y) + windowPos, GRID_COLOR);
    for (float y = fmodf(scrolling.y * factor, GRID_SZ); y < canvasSize.y; y += GRID_SZ)
        drawList->AddLine(ImVec2(0.0f, y) + windowPos, ImVec2(canvasSize.x, y) + windowPos, GRID_COLOR);
}

// return true if node is hovered
static bool DrawNode(GraphModel* model,
                     ImDrawList* drawList,
                     int nodeIndex,
                     const ImVec2 offset,
                     const float factor,
                     NodeGraphControlerBase* controler,
                     bool overInput)
{
    ImGuiIO& io = ImGui::GetIO();
    const MetaNode* metaNodes = gMetaNodes.data();
    const auto& nodes = model->GetNodes();
    const auto* node = &nodes[nodeIndex];
    const auto& metaNode = metaNodes[node->mType];

    ImVec2 node_rect_min = offset + node->mPos * factor;

    bool old_any_active = ImGui::IsAnyItemActive();
    ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
    ImVec2 nodeSize = ImVec2(metaNode.mWidth, metaNode.mHeight) * factor;
    const bool nodeIsCompute = controler->NodeIsCompute(nodeIndex);
    ImGui::InvisibleButton("canvas", nodeSize);

    // must be called right after creating the control we want to be able to move
    bool node_moving_active = ImGui::IsItemActive(); 

    // Save the size of what we have emitted and whether any of the widgets are being used
    bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
    ImVec2 node_rect_max = node_rect_min + nodeSize;

    // test nested IO
    drawList->ChannelsSetCurrent(1); // Background
    size_t InputsCount = metaNode.mInputs.size();
    size_t OutputsCount = metaNode.mOutputs.size();

    for (int i = 0; i < 2; i++)
    {
        const size_t slotCount[2] = {InputsCount, OutputsCount};
        const MetaCon* con = i ? metaNode.mOutputs.data() : metaNode.mInputs.data();
        for (int slot_idx = 0; slot_idx < slotCount[i]; slot_idx++)
        {
            if (!model->IsIOPinned(nodeIndex, slot_idx, i == 1))
            {
                continue;
            }
            ImVec2 p =
                offset + (i ? GetOutputSlotPos(*node, slot_idx, factor) : GetInputSlotPos(*node, slot_idx, factor));
            const float arc = 28.f * (float(i) * 0.3f + 1.0f) * (i ? 1.f : -1.f);
            const float ofs = 0.f;

            ImVec2 pts[3] = {p + ImVec2(arc + ofs, 0.f), p + ImVec2(0.f + ofs, -arc), p + ImVec2(0.f + ofs, arc)};
            drawList->AddTriangleFilled(pts[0], pts[1], pts[2], i ? 0xFFAA5030 : 0xFF30AA50);
            drawList->AddTriangle(pts[0], pts[1], pts[2], 0xFF000000, 2.f);
        }
    }


    ImGui::SetCursorScreenPos(node_rect_min);
    ImGui::InvisibleButton("node", nodeSize);
    bool nodeHovered = false;
    if (ImGui::IsItemHovered() && nodeOperation == NO_None && !overInput)
    {
        nodeHovered = true;
    }

    if (ImGui::IsWindowFocused())
    {
        if (node_widgets_active || node_moving_active)
        {
            if (!node->mbSelected)
            {
                if (!io.KeyShift)
                {
                    for (auto i = 0; i < nodes.size(); i++)
                    {
                        model->SelectNode(i, false);
                    }
                }
                model->SelectNode(nodeIndex);
            }
        }
    }
    if (node_moving_active && io.MouseDown[0] && nodeHovered)
    {
        if (nodeOperation != NO_MovingNodes)
        {
            nodeOperation = NO_MovingNodes;
        }
    }

    bool currentSelectedNode = node->mbSelected;

    // [experimental][hovered]
    static const uint32_t nodeBGColors[2][2] = {{IM_COL32(60, 60, 60, 255), IM_COL32(85, 85, 85, 255)},
                                                {IM_COL32(80, 50, 20, 255), IM_COL32(105, 75, 45, 255)}};

	bool experimental = metaNode.mbExperimental;
	 ImU32 node_bg_color = nodeBGColors[experimental?1:0][nodeHovered?1:0];

	drawList->AddRect(node_rect_min,
					  node_rect_max,
					  currentSelectedNode ? IM_COL32(255, 130, 30, 255) : IM_COL32(100, 100, 100, 0),
					  2.0f,
					  15,
					  currentSelectedNode ? 6.f : 2.f);

	ImVec2 imgPos = node_rect_min + ImVec2(14, 25);
	ImVec2 imgSize = node_rect_max + ImVec2(-5, -5) - imgPos;
	float imgSizeComp = std::min(imgSize.x, imgSize.y);

	drawList->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 2.0f);
	float progress = controler->NodeProgress(nodeIndex);
	if (progress > FLT_EPSILON && progress < 1.f - FLT_EPSILON)
	{
		ImVec2 progressLineA = node_rect_max - ImVec2(nodeSize.x - 2.f, 3.f);
		ImVec2 progressLineB = progressLineA + ImVec2(nodeSize.x * factor - 4.f, 0.f);
		drawList->AddLine(progressLineA, progressLineB, 0xFF400000, 3.f);
		drawList->AddLine(progressLineA, ImLerp(progressLineA, progressLineB, progress), 0xFFFF0000, 3.f);
	}
	ImVec2 imgPosMax = imgPos + ImVec2(imgSizeComp, imgSizeComp);

	ImVec2 imageSize = controler->GetEvaluationSize(nodeIndex);
	float imageRatio = 1.f;
	if (imageSize.x > 0.f && imageSize.y > 0.f)
		imageRatio = imageSize.y / imageSize.x;
	ImVec2 quadSize = imgPosMax - imgPos;
	ImVec2 marge(0.f, 0.f);
	if (imageRatio > 1.f)
	{
		marge.x = (quadSize.x - quadSize.y / imageRatio) * 0.5f;
	}
	else
	{
		marge.y = (quadSize.y - quadSize.y * imageRatio) * 0.5f;
	}

	controler->DrawNodeImage(drawList, ImRect(imgPos, imgPosMax), marge, nodeIndex);

	drawList->AddRectFilled(node_rect_min,
							ImVec2(node_rect_max.x, node_rect_min.y + 20),
							metaNodes[node->mType].mHeaderColor,
							2.0f);
	drawList->PushClipRect(node_rect_min, ImVec2(node_rect_max.x, node_rect_min.y + 20), true);
	drawList->AddText(node_rect_min + ImVec2(2, 2), IM_COL32(0, 0, 0, 255), metaNodes[node->mType].mName.c_str());
	drawList->PopClipRect();


	unsigned int stage2D = gImageCache.GetTexture("Stock/Stage2D.png");
	unsigned int stagecubemap = gImageCache.GetTexture("Stock/StageCubemap.png");
	unsigned int stageCompute = gImageCache.GetTexture("Stock/StageCompute.png");

	ImVec2 bmpInfoPos(node_rect_max - ImVec2(26, 12));
	ImVec2 bmpInfoSize(20, 20);
	if (controler->NodeIsCompute(nodeIndex))
	{
		drawList->AddImageQuad((ImTextureID)(uint64_t)stageCompute,
							   bmpInfoPos,
							   bmpInfoPos + ImVec2(bmpInfoSize.x, 0.f),
							   bmpInfoPos + bmpInfoSize,
							   bmpInfoPos + ImVec2(0., bmpInfoSize.y));
	}
	else if (controler->NodeIs2D(nodeIndex))
	{
		drawList->AddImageQuad((ImTextureID)(uint64_t)stage2D,
							   bmpInfoPos,
							   bmpInfoPos + ImVec2(bmpInfoSize.x, 0.f),
							   bmpInfoPos + bmpInfoSize,
							   bmpInfoPos + ImVec2(0., bmpInfoSize.y));
	}
	else if (controler->NodeIsCubemap(nodeIndex))
	{
		drawList->AddImageQuad((ImTextureID)(uint64_t)stagecubemap,
							   bmpInfoPos + ImVec2(0., bmpInfoSize.y),
							   bmpInfoPos + bmpInfoSize,
							   bmpInfoPos + ImVec2(bmpInfoSize.x, 0.f),
							   bmpInfoPos);
	}
	return nodeHovered;
}

void ComputeDelegateSelection(GraphModel* model, NodeGraphControlerBase* controler)
{
    const auto& nodes = model->GetNodes();
    // only one selection allowed for delegate
    controler->mSelectedNodeIndex = -1;
    for (auto& node : nodes)
    {
        if (node.mbSelected)
        {
            if (controler->mSelectedNodeIndex == -1)
            {
                controler->mSelectedNodeIndex = int(&node - nodes.data());
            }
            else
            {
                controler->mSelectedNodeIndex = -1;
                return;
            }
        }
    }
}

void NodeGraph(GraphModel* model, NodeGraphControlerBase* controler, bool enabled)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);

    const ImVec2 windowPos = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetWindowSize();
    const ImVec2 scrollRegionLocalPos(0, 50);
    const auto& nodes = model->GetNodes();

    static ImVec2 scenePos;

    ImRect regionRect(windowPos, windowPos + canvasSize);

    HandleZoomScroll(regionRect);
    ImVec2 offset = ImGui::GetCursorScreenPos() + scrolling * factor;
    captureOffset = scrollRegionLocalPos + scrolling * factor + ImVec2(10.f, 0.f);

    {
        ImGui::BeginChild("rugs_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        editRug = DisplayRugs(model, editRug, drawList, offset, factor);

        ImGui::EndChild();
    }

    ImGui::SetCursorPos(windowPos);
    ImGui::BeginGroup();


    ImGuiIO& io = ImGui::GetIO();

    // Create our child canvas
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(30, 30, 30, 200));

    ImGui::SetCursorPos(scrollRegionLocalPos);
    ImGui::BeginChild("scrolling_region",
                      ImVec2(0, 0),
                      true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
    ImGui::PushItemWidth(120.0f);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background or Display grid
    if (!controler->RenderBackground())
    {
        DrawGrid(drawList, windowPos, canvasSize, factor);
    }

    if (!enabled)
        goto nodeGraphExit;

    static int hoveredNode = -1;
    // Display links
    drawList->ChannelsSplit(3);
    drawList->ChannelsSetCurrent(1); // Background
    DisplayLinks(model, drawList, offset, factor, regionRect, hoveredNode);

    // edit node link
    if (nodeOperation == NO_EditingLink)
    {
        ImVec2 p1 = editingNodeSource;
        ImVec2 p2 = io.MousePos;
        drawList->AddLine(p1, p2, IM_COL32(200, 200, 200, 255), 3.0f);
    }

    // Display nodes
    drawList->PushClipRect(regionRect.Min, regionRect.Max, true);
    hoveredNode = -1;
    for (int i = 0; i < 2; i++)
    {
        for (int nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
        {
            const auto* node = &nodes[nodeIndex];
            if (node->mbSelected != (i != 0))
                continue;

            // node view clipping
            ImRect nodeRect = GetNodeRect(*node, factor);
            nodeRect.Min += offset;
            nodeRect.Max += offset;
            if (!regionRect.Overlaps(nodeRect))
                continue;

            ImGui::PushID(nodeIndex);

            // Display node contents first
            // drawList->ChannelsSetCurrent(i+1); // channel 2 = Foreground channel 1 = background

            bool overInput = HandleConnections(model, drawList, nodeIndex, offset, factor, controler, false);

            if (DrawNode(model, drawList, nodeIndex, offset, factor, controler, overInput))
                hoveredNode = nodeIndex;

            HandleConnections(model, drawList, nodeIndex, offset, factor, controler, true);

            ImGui::PopID();
        }
    }
    drawList->PopClipRect();

    if (nodeOperation == NO_MovingNodes)
    {
        if (ImGui::IsMouseDragging(0, 1))
        {
            if (!model->InTransaction())
            {
                model->BeginTransaction(true);
            }
            model->MoveSelectedNodes(io.MouseDelta / factor);
        }
    }

    // rugs
    drawList->ChannelsSetCurrent(0);

    // quad selection
    HandleQuadSelection(model, drawList, offset, factor, regionRect);

    drawList->ChannelsMerge();

    if (editRug != -1)
    {
        if (EditRug(model, editRug, drawList, offset, factor))
		{
            editRug = -1;
		}
    }

    // releasing mouse button means it's done in any operation
    if (nodeOperation == NO_PanView)
    {
        if (!io.MouseDown[2])
        {
            nodeOperation = NO_None;
        }
    }
    else if (nodeOperation != NO_None && !io.MouseDown[0])
    {
        nodeOperation = NO_None;
        if (model->InTransaction())
        {
            model->EndTransaction();
        }
    }

    // Open context menu
    bool openContextMenu = false;
    static int contextMenuHoverNode = -1;
    if (nodeOperation == NO_None && regionRect.Contains(io.MousePos) &&
        (ImGui::IsMouseClicked(1) || (ImGui::IsWindowFocused() && ImGui::IsKeyPressedMap(ImGuiKey_Tab))))
    {
        openContextMenu = true;
        contextMenuHoverNode = hoveredNode;
    }

    if (openContextMenu)
    {
        scenePos = (ImGui::GetMousePosOnOpeningCurrentPopup() - offset) / factor;
        ImGui::OpenPopup("context_menu");
    }
    
    controler->ContextMenu(scenePos, contextMenuHoverNode);

    // Scrolling
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && io.MouseClicked[2] && nodeOperation == NO_None)
    {
        nodeOperation = NO_PanView;
    }
    if (nodeOperation == NO_PanView)
    {
        scrolling += io.MouseDelta / factor;
    }

nodeGraphExit:;
    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(2);

    ComputeDelegateSelection(model, controler);

    ImGui::EndGroup();
    ImGui::PopStyleVar(3);
}

