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

void AddExtractedView(size_t nodeIndex);

static inline float Distance(ImVec2& a, ImVec2& b) { return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)); }

void LinkCallback(ImGui::MarkdownLinkCallbackData data_)
{
    std::string url(data_.link, data_.linkLength);
    static const std::string graph = "thumbnail:";
    if (url.substr(0, graph.size()) == graph)
    {
        std::string materialName = url.substr(graph.size());
        //SetExistingMaterialActive(materialName.c_str()); TODO
        return;
    }
    OpenShellURL(url);
}

inline ImGui::MarkdownImageData ImageCallback(ImGui::MarkdownLinkCallbackData data_)
{
    std::string url(data_.link, data_.linkLength);
    static const std::string thumbnail = "thumbnail:";
    if (url.substr(0, thumbnail.size()) == thumbnail)
    {
        std::string material = url.substr(thumbnail.size());
        Material* libraryMaterial = library.GetByName(material.c_str());
        if (libraryMaterial)
        {
            //DecodeThumbnailAsync(libraryMaterial); TODO
            return { true, true, (ImTextureID)(uint64_t)libraryMaterial->mThumbnailTextureId, ImVec2(100, 100), ImVec2(0.f, 1.f), ImVec2(1.f, 0.f) };
        }
    }
    else
    {
        int percent = 100;
        size_t sz = url.find('@');
        if (sz != std::string::npos)
        {
            sscanf(url.c_str() + sz + 1, "%d%%", &percent);
            url = url.substr(0, sz);
        }
        unsigned int textureId = gImageCache.GetTexture(url);
        if (textureId)
        {
            int w, h;
            GetTextureDimension(textureId, &w, &h);
            return { true, false, (ImTextureID)(uint64_t)textureId, ImVec2(float(w*percent / 100), float(h*percent / 100)), ImVec2(0.f, 1.f), ImVec2(1.f, 0.f) };
        }
    }

    return { false };
}

ImGui::MarkdownConfig mdConfig = { LinkCallback, ImageCallback, "", { { NULL, true }, { NULL, true }, { NULL, false } } };

Node::Node(int type, const ImVec2& pos)
{
    mType = type;
    Pos = pos;
    Size = ImVec2(100, 100);
    InputsCount = gMetaNodes[type].mInputs.size();
    OutputsCount = gMetaNodes[type].mOutputs.size();
    mbSelected = false;
}

struct NodeOrder
{
    size_t mNodeIndex;
    size_t mNodePriority;
    bool operator < (const NodeOrder& other) const
    {
        return other.mNodePriority < mNodePriority; // reverse order compared to priority value: lower last
    }
};

size_t PickBestNode(const std::vector<NodeOrder>& orders)
{
    for (auto& order : orders)
    {
        if (order.mNodePriority == 0)
            return order.mNodeIndex;
    }
    // issue!
    assert(0);
    return -1;
}

void RecurseSetPriority(std::vector<NodeOrder>& orders, const std::vector<NodeLink> &links, size_t currentIndex, size_t currentPriority, size_t& undeterminedNodeCount)
{
    if (!orders[currentIndex].mNodePriority)
        undeterminedNodeCount--;

    orders[currentIndex].mNodePriority = std::max(orders[currentIndex].mNodePriority, currentPriority+1);
    for (auto & link : links)
    {
        if (link.OutputIdx == currentIndex)
        {
            RecurseSetPriority(orders, links, link.InputIdx, currentPriority + 1, undeterminedNodeCount);
        }
    }
}

std::vector<NodeOrder> ComputeEvaluationOrder(const std::vector<NodeLink> &links, size_t nodeCount)
{
    std::vector<NodeOrder> orders(nodeCount);
    for (size_t i = 0; i < nodeCount; i++)
    {
        orders[i].mNodeIndex = i;
        orders[i].mNodePriority = 0;
    }
    size_t undeterminedNodeCount = nodeCount;
    while (undeterminedNodeCount)
    {
        size_t currentIndex = PickBestNode(orders);
        RecurseSetPriority(orders, links, currentIndex, orders[currentIndex].mNodePriority, undeterminedNodeCount);
    };
    //
    return orders;
}

const float NODE_SLOT_RADIUS = 8.0f;
const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

static std::vector<NodeOrder> mOrders;
static std::vector<Node> nodes;
static std::vector<Node> mNodesClipboard;
static std::vector<NodeLink> links;
static std::vector<NodeRug> rugs;
static NodeRug *editRug = NULL;

static ImVec2 editingNodeSource;
bool editingInput = false;
static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
float factor = 1.0f;
float factorTarget = 1.0f;

enum NodeOperation
{
    NO_None,
    NO_EditingLink,
    NO_QuadSelecting,
    NO_MovingNodes,
    NO_EditInput,
    NO_MovingRug,
    NO_SizingRug,
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
    nodes.clear();
    links.clear();
    rugs.clear();
    editRug = NULL;
    nodeOperation = NO_None;
    factor = 1.0f;
    factorTarget = 1.0f;
}

const std::vector<NodeLink>& NodeGraphGetLinks()
{
    return links;
}

const std::vector<NodeRug>& NodeGraphRugs()
{
    return rugs;
}

NodeRug* DisplayRugs(NodeRug *editRug, ImDrawList* drawList, ImVec2 offset, float factor)
{
    ImGuiIO& io = ImGui::GetIO();
    NodeRug *ret = editRug;
    
    // mouse pointer over any node?
    bool overAnyNode = false;
    for (auto& node : nodes)
    {
        ImVec2 node_rect_min = offset + node.Pos * factor;
        ImVec2 node_rect_max = node_rect_min + node.Size;
        if (ImRect(node_rect_min, node_rect_max).Contains(io.MousePos))
        {
            overAnyNode = true;
            break;
        }
    }

    for(unsigned int rugIndex = 0; rugIndex<rugs.size();rugIndex++)
    {
        auto& rug = rugs[rugIndex];
        if (&rug == editRug)
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
                ret = &rug;
            }
            else if (io.KeyShift && ImGui::IsMouseClicked(0))
            {
                for (auto& node:nodes)
                {
                    ImVec2 node_rect_min = offset + node.Pos * factor;
                    ImVec2 node_rect_max = node_rect_min + node.Size;
                    if (rugRect.Overlaps(ImRect(node_rect_min, node_rect_max)))
                    {
                        node.mbSelected = true;
                    }
                }
            }
        }
        //drawList->AddText(io.FontDefault, 13 * ImLerp(1.f, factor, 0.5f), node_rect_min + ImVec2(5, 5), (rug.mColor & 0xFFFFFF) + 0xFF404040, rug.mText.c_str());
        ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, 0x0);
        ImGui::PushStyleColor(ImGuiCol_Border, 0x0);
        ImGui::BeginChildFrame(88 + rugIndex, commentSize - NODE_WINDOW_PADDING * 2, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav);
        
        ImGui::Markdown(rug.mText.c_str(), rug.mText.length(), mdConfig);
        drawList->AddRectFilled(node_rect_min, node_rect_max, (rug.mColor&0xFFFFFF)+0x60000000, 10.0f, 15);
        drawList->AddRect(node_rect_min, node_rect_max, (rug.mColor & 0xFFFFFF) + 0x90000000, 10.0f, 15, 2.f);
        ImGui::EndChildFrame();
        ImGui::PopStyleColor(2);
        ImGui::PopID();
    }
    return ret;
}

bool EditRug(NodeRug *rug, ImDrawList* drawList, ImVec2 offset, float factor)
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 commentSize = rug->mSize * factor;
    static NodeRug* movingRug = NULL;
    static NodeRug* sizingRug = NULL;

    int rugIndex = int(rug - rugs.data());
    URChange<NodeRug> undoRedoRug(rugIndex, [](int index) {return &rugs[index]; }, [] (int){});
    bool dirtyRug = false;
    ImVec2 node_rect_min = offset + rug->mPos * factor;
    ImVec2 node_rect_max = node_rect_min + commentSize;
    ImRect rugRect(node_rect_min, node_rect_max);
    ImRect insideSizingRect(node_rect_min + commentSize - ImVec2(30, 30), node_rect_min + commentSize);
    ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
    

    drawList->AddRectFilled(node_rect_min, node_rect_max, (rug->mColor & 0xFFFFFF) + 0xE0000000, 10.0f, 15);
    drawList->AddRect(node_rect_min, node_rect_max, (rug->mColor & 0xFFFFFF) + 0xFF000000, 10.0f, 15, 2.f);
    drawList->AddTriangleFilled(node_rect_min + commentSize - ImVec2(25, 8), node_rect_min + commentSize - ImVec2(8, 25), node_rect_min + commentSize - ImVec2(8, 8), (rug->mColor & 0xFFFFFF) + 0x90000000);

    ImGui::SetCursorScreenPos(node_rect_min + ImVec2(5, 5));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
    dirtyRug |= ImGui::InputTextMultiline("", &rug->mText, (node_rect_max - node_rect_min) - ImVec2(30, 30));
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
            rug->mColor = buttonColor;
            dirtyRug = true;
        }
        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }
    ImGui::SameLine(0, 50);

    if (ImGui::Button("Delete"))
    {
        undoRedoRug.Discard();
        int index = int(rug - rugs.data());
        URDel<NodeRug> undoRedoDelRug(index, []() {return &rugs; }, [](int) {}, [](int) {});
        rugs.erase(rugs.begin() + index);
        return true;
    }

    bool createUndo = false;
    bool commitUndo = false;
    bool mouseMoved = fabsf(io.MouseDelta.x) > FLT_EPSILON && fabsf(io.MouseDelta.y) > FLT_EPSILON;
    if (insideSizingRect.Contains(io.MousePos))
    if (nodeOperation == NO_None && io.MouseDown[0] /* && !mouseMoved*/)
    {
        sizingRug = rug;
        createUndo = true;
        nodeOperation = NO_SizingRug;
    }
    if (sizingRug && !io.MouseDown[0])
    {
        sizingRug = NULL;
        commitUndo = true;
        nodeOperation = NO_None;
    }

    if (rugRect.Contains(io.MousePos) && !insideSizingRect.Contains(io.MousePos))
    if (nodeOperation == NO_None && io.MouseDown[0] /*&& !mouseMoved*/)
    {
        movingRug = rug;
        createUndo = true;
        nodeOperation = NO_MovingRug;
    }
    if (movingRug && !io.MouseDown[0])
    {
        commitUndo = true;
        movingRug = NULL;
        nodeOperation = NO_None;
    }

    // undo/redo for sizing/moving
    static URChange<NodeRug> *undoRedoRugcm = NULL;
    if (createUndo)
    {
        undoRedoRugcm = new URChange<NodeRug>(rugIndex, [](int index) {return &rugs[index]; }, [](int) {});
    }
    if (commitUndo)
    {
        delete undoRedoRugcm;
        undoRedoRugcm = NULL;
    }
    if (!dirtyRug)
    {
        undoRedoRug.Discard();
    }

    if (sizingRug && ImGui::IsMouseDragging(0))
        sizingRug->mSize += io.MouseDelta;
    if (movingRug && ImGui::IsMouseDragging(0))
        movingRug->mPos += io.MouseDelta;

    if ((io.MouseClicked[0] || io.MouseClicked[1]) && !rugRect.Contains(io.MousePos))
        return true;
    return false;
}

bool RecurseIsLinked(int from, int to)
{
    for (auto& link : links)
    {
        if (link.InputIdx == from)
        {
            if (link.OutputIdx == to)
                return true;

            if (RecurseIsLinked(link.OutputIdx, to))
                return true;
        }
    }
    return false;
}

void NodeGraphUpdateEvaluationOrder(NodeGraphControlerBase *controler)
{
    mOrders = ComputeEvaluationOrder(links, nodes.size());
    std::sort(mOrders.begin(), mOrders.end());
    std::vector<size_t> nodeOrderList(mOrders.size());
    for (size_t i = 0; i < mOrders.size(); i++)
        nodeOrderList[i] = mOrders[i].mNodeIndex;
    controler->UpdateEvaluationList(nodeOrderList);
}

void NodeGraphAddNode(NodeGraphControlerBase *controler, int type, const std::vector<unsigned char>& parameters, int posx, int posy, int frameStart, int frameEnd)
{
    size_t index = nodes.size();
    nodes.push_back(Node(type, ImVec2(float(posx), float(posy))));
    
    controler->AddSingleNode(type);
    controler->SetParamBlock(index, parameters);
    controler->SetTimeSlot(index, frameStart, frameEnd);
}

void NodeGraphAddLink(NodeGraphControlerBase *controler, int InputIdx, int InputSlot, int OutputIdx, int OutputSlot)
{
    NodeLink nl;
    nl.InputIdx = InputIdx;
    nl.InputSlot = InputSlot;
    nl.OutputIdx = OutputIdx;
    nl.OutputSlot = OutputSlot;
    links.push_back(nl);
    controler->AddLink(nl.InputIdx, nl.InputSlot, nl.OutputIdx, nl.OutputSlot);
}

ImVec2 NodeGraphGetNodePos(size_t index)
{
    return nodes[index].Pos;
}

void NodeGraphUpdateScrolling()
{
    if (nodes.empty() && rugs.empty())
        return;

    scrolling = nodes[0].Pos;
    for (auto& node : nodes)
    {
        scrolling.x = std::min(scrolling.x, node.Pos.x);
        scrolling.y = std::min(scrolling.y, node.Pos.y);
    }
    for (auto& rug : rugs)
    {
        scrolling.x = std::min(scrolling.x, rug.mPos.x);
        scrolling.y = std::min(scrolling.y, rug.mPos.y);
    }

    scrolling = ImVec2(40, 40) - scrolling;
}

void NodeGraphAddRug(int32_t posX, int32_t posY, int32_t sizeX, int32_t sizeY, uint32_t color, const std::string comment)
{
    rugs.push_back({ ImVec2(float(posX), float(posY)), ImVec2(float(sizeX), float(sizeY)), color, comment });
}

static void DeleteSelectedNodes(NodeGraphControlerBase *controler)
{
    URDummy urDummy;
    for (int selection = int(nodes.size()) - 1 ; selection >= 0 ; selection--)
    {
        if (!nodes[selection].mbSelected)
            continue;
        URDel<Node> undoRedoDelNode(int(selection), []() {return &nodes; }
            , [&controler](int index) {
            // recompute link indices
            for (int id = 0; id < links.size(); id++)
            {
                if (links[id].InputIdx > index)
                    links[id].InputIdx--;
                if (links[id].OutputIdx > index)
                    links[id].OutputIdx--;
            }
            NodeGraphUpdateEvaluationOrder(controler);
            controler->mSelectedNodeIndex = -1;
        }
            , [&controler](int index) {
            // recompute link indices
            for (int id = 0; id < links.size(); id++)
            {
                if (links[id].InputIdx >= index)
                    links[id].InputIdx++;
                if (links[id].OutputIdx >= index)
                    links[id].OutputIdx++;
            }

            NodeGraphUpdateEvaluationOrder(controler);
            controler->mSelectedNodeIndex = -1;
        }
        );

        for (int id = 0; id < links.size(); id++)
        {
            if (links[id].InputIdx == selection || links[id].OutputIdx == selection)
                controler->DelLink(links[id].OutputIdx, links[id].OutputSlot);
        }
        //auto iter = links.begin();
        for (size_t i = 0; i < links.size();)
        {
            auto& link = links[i];
            if (link.InputIdx == selection || link.OutputIdx == selection)
            {
                URDel<NodeLink> undoRedoDelNodeLink(int(i), []() {return &links; }
                    , [&controler](int index)
                {
                    NodeLink& link = links[index];
                    controler->DelLink(link.OutputIdx, link.OutputSlot);
                }
                    , [&controler](int index)
                {
                    NodeLink& link = links[index];
                    controler->AddLink(link.InputIdx, link.InputSlot, link.OutputIdx, link.OutputSlot);
                });

                links.erase(links.begin() + i);
            }
            else
            {
                i++;
            }
        }

        // recompute link indices
        for (int id = 0; id < links.size(); id++)
        {
            if (links[id].InputIdx > selection)
                links[id].InputIdx--;
            if (links[id].OutputIdx > selection)
                links[id].OutputIdx--;
        }

        // delete links
        nodes.erase(nodes.begin() + selection);
        NodeGraphUpdateEvaluationOrder(controler);

        // inform delegate
        controler->UserDeleteNode(selection);
    }
}

static void ContextMenu(ImVec2 offset, int nodeHovered, NodeGraphControlerBase *controler)
{
    ImGuiIO& io = ImGui::GetIO();
    size_t metaNodeCount = gMetaNodes.size();
    const MetaNode* metaNodes = gMetaNodes.data();

    bool copySelection = false;
    bool deleteSelection = false;
    bool pasteSelection = false;

    // Draw context menu
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (ImGui::BeginPopup("context_menu"))
    {
        Node* node = nodeHovered != -1 ? &nodes[nodeHovered] : NULL;
        ImVec2 scene_pos = (ImGui::GetMousePosOnOpeningCurrentPopup() - offset)/factor;
        if (node)
        {
            ImGui::Text(metaNodes[node->mType].mName.c_str());
            ImGui::Separator();

            if (ImGui::MenuItem("Extract view", NULL, false))
            {
                AddExtractedView(nodeHovered);
            }
        }
        else
        {
            auto AddNode = [&](int i)
            {
                auto addDelNodeLambda = [&controler](int)
                {
                    NodeGraphUpdateEvaluationOrder(controler);
                    controler->mSelectedNodeIndex = -1;
                };
                URAdd<Node> undoRedoAddRug(int(nodes.size()), []() {return &nodes; }, addDelNodeLambda, addDelNodeLambda);

                nodes.push_back(Node(i, scene_pos));
                controler->UserAddNode(i);
                addDelNodeLambda(0);
            };

            static char inputText[64] = { 0 };
            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();
            ImGui::InputText("", inputText, sizeof(inputText));
            {
                if (strlen(inputText))
                {
                    for (int i = 0; i < metaNodeCount; i++)
                    {
                        const char *nodeName = metaNodes[i].mName.c_str();
                        bool displayNode = !strlen(inputText) || ImStristr(nodeName, nodeName + strlen(nodeName), inputText, inputText + strlen(inputText));
                        if (displayNode && ImGui::MenuItem(nodeName, NULL, false))
                        {
                            AddNode(i);
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < metaNodeCount; i++)
                    {
                        const char *nodeName = metaNodes[i].mName.c_str();
                        if (metaNodes[i].mCategory == -1 && ImGui::MenuItem(nodeName, NULL, false))
                        {
                            AddNode(i);
                        }
                    }

                    for (int iCateg = 0; iCateg < controler->mCategoriesCount; iCateg++)
                    {
                        if (ImGui::BeginMenu(controler->mCategories[iCateg]))
                        {
                            for (int i = 0; i < metaNodeCount; i++)
                            {
                                const char *nodeName = metaNodes[i].mName.c_str();
                                if (metaNodes[i].mCategory == iCateg && ImGui::MenuItem(nodeName, NULL, false))
                                {
                                    AddNode(i);
                                }
                            }
                            ImGui::EndMenu();
                        }
                    }
                }
            }
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Add rug", NULL, false))
        {
            URAdd<NodeRug> undoRedoAddRug(int(rugs.size()), []() {return &rugs; }, [](int) {}, [](int) {});
            rugs.push_back({ scene_pos, ImVec2(400,200), 0xFFA0A0A0, "Description\nEdit me with a double click." });
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete", "Del", false))
        {
            deleteSelection = true;
        }
        if (ImGui::MenuItem("Copy", "CTRL+C"))
        {
            copySelection = true;
        }
        if (ImGui::MenuItem("Paste", "CTRL+V", false, !mNodesClipboard.empty()))
        {
            pasteSelection = true;
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    if (copySelection || (ImGui::IsWindowFocused() && io.KeyCtrl && ImGui::IsKeyPressedMap(ImGuiKey_C)))
    {
        mNodesClipboard.clear();
        std::vector<size_t> selection;
        for (size_t i = 0; i < nodes.size(); i++)
        {
            if (!nodes[i].mbSelected)
                continue;
            mNodesClipboard.push_back(nodes[i]);
            selection.push_back(i);
        }
        controler->CopyNodes(selection);
    }

    if (deleteSelection || (ImGui::IsWindowFocused() && ImGui::IsKeyPressedMap(ImGuiKey_Delete)))
    {
        DeleteSelectedNodes(controler);
    }

    if (pasteSelection || (ImGui::IsWindowFocused() && io.KeyCtrl && ImGui::IsKeyPressedMap(ImGuiKey_V)))
    {
        URDummy undoRedoDummy;
        ImVec2 min(FLT_MAX, FLT_MAX);
        for (auto& clipboardNode : mNodesClipboard)
        {
            min.x = ImMin(clipboardNode.Pos.x, min.x);
            min.y = ImMin(clipboardNode.Pos.y, min.y);
        }
        for (auto& selnode : nodes)
            selnode.mbSelected = false;
        for (auto& clipboardNode : mNodesClipboard)
        {
            URAdd<Node> undoRedoAddRug(int(nodes.size()), []() {return &nodes; }, [](int index) {}, [](int index) {});
            nodes.push_back(clipboardNode);
            nodes.back().Pos += (io.MousePos/factor - offset) - min;
            nodes.back().mbSelected = true;
        }
        controler->PasteNodes();
        NodeGraphUpdateEvaluationOrder(controler);
    }
}

static void DisplayLinks(ImDrawList* drawList, const ImVec2 offset, const float factor, const ImRect regionRect)
{
    for (int link_idx = 0; link_idx < links.size(); link_idx++)
    {
        NodeLink* link = &links[link_idx];
        Node* node_inp = &nodes[link->InputIdx];
        Node* node_out = &nodes[link->OutputIdx];
        ImVec2 p1 = offset + node_inp->GetOutputSlotPos(link->InputSlot, factor);
        ImVec2 p2 = offset + node_out->GetInputSlotPos(link->OutputSlot, factor);

        // con. view clipping
        if ((p1.y < 0.f && p2.y < 0.f) || (p1.y > regionRect.Max.y && p2.y > regionRect.Max.y) ||
            (p1.x < 0.f && p2.x < 0.f) || (p1.x > regionRect.Max.x && p2.x > regionRect.Max.x))
            continue;
        
        uint32_t col = gMetaNodes[node_inp->mType].mHeaderColor;
        // curves
        //drawList->AddBezierCurve(p1, p1 + ImVec2(+50, 0) * factor, p2 + ImVec2(-50, 0) * factor, p2, 0xFF000000, 4.f * factor);
        //drawList->AddBezierCurve(p1, p1 + ImVec2(+50, 0) * factor, p2 + ImVec2(-50, 0) * factor, p2, col, 3.0f * factor);

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
        float overdrawFactor = 1.4f;
        const float limitx = 12.f * factor;
        if (dif.x < limitx)
        {
            ImVec2 p10 = p1 + ImVec2(limitx, 0.f);
            ImVec2 p20 = p2 - ImVec2(limitx, 0.f);

            dif = p20 - p10;
            p1a = p10 + ImVec2(0.f, dif.y * 0.5f);
            p1b = p1a + ImVec2(dif.x, 0.f);

            pts = { p1, p10, p1a, p1b, p20, p2 };
            ptCount = 6;
            overdrawFactor = 2.6f;
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
                    p1b = p1a + ImVec2(fabsf(fabsf(dif.x) - fabsf(d)*2.f) * sign(dif.x), 0.f);
                }
                else
                {

                    float d = fabsf(dif.x) * sign(dif.y) * 0.5f;
                    p1a = p1 + ImVec2(dif.x * 0.5f, d);
                    p1b = p1a + ImVec2(0.f, fabsf(fabsf(dif.y) - fabsf(d)*2.f) * sign(dif.y));
                }
            }
            pts = { p1, p1a, p1b, p2 };
            ptCount = 4;
        }
        for (int pass = 0; pass < 2; pass++)
        {
            for (int i = 0; i < ptCount - 1; i++)
            {
                // make sure each segment overdraw a bit so you can't see cracks in joints
                ImVec2 p1 = pts[i];
                ImVec2 p2 = pts[i + 1];
                ImVec2 dif = p2 - p1;
                float diflen = sqrtf(dif.x*dif.x + dif.y*dif.y);
                ImVec2 difNorm = dif / ImVec2(diflen, diflen);
                p1 -= difNorm * overdrawFactor * factor;
                p2 += difNorm * overdrawFactor * factor;
                drawList->AddLine(p1, p2, pass?col:0xFF000000, (pass?5.f:7.5f) * factor);
            }
        }
    }
}

static void HandleQuadSelection(ImDrawList* drawList, const ImVec2 offset, const float factor, ImRect contentRect)
{
    ImGuiIO& io = ImGui::GetIO();
    static ImVec2 quadSelectPos;

    ImRect editingRugRect(ImVec2(FLT_MAX, FLT_MAX), ImVec2(FLT_MAX, FLT_MAX));
    if (editRug)
    {
        ImVec2 commentSize = editRug->mSize * factor;
        ImVec2 node_rect_min = (offset + editRug->mPos) * factor;
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
                    Node* node = &nodes[nodeIndex];
                    node->mbSelected = false;
                }
            }

            nodeOperation = NO_None;
            ImRect selectionRect(bmin, bmax);
            for (int nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
            {
                Node* node = &nodes[nodeIndex];
                ImVec2 node_rect_min = offset + node->Pos * factor;
                ImVec2 node_rect_max = node_rect_min + node->Size;
                if (selectionRect.Overlaps(ImRect(node_rect_min, node_rect_max)))
                {
                    if (io.KeyCtrl)
                    {
                        node->mbSelected = false;
                    }
                    else
                    {
                        node->mbSelected = true;
                    }
                }
                else
                {
                    if (!io.KeyShift)
                    {
                        node->mbSelected = false;
                    }
                }
            }
        }
    }
    else if (nodeOperation == NO_None && io.MouseDown[0] && ImGui::IsWindowFocused() && contentRect.Contains(io.MousePos) && !editingRugRect.Contains(io.MousePos))
    {
        nodeOperation = NO_QuadSelecting;
        quadSelectPos = io.MousePos;
    }
}


void HandleConnections(ImDrawList* drawList, int nodeIndex, const ImVec2 offset, const float factor, NodeGraphControlerBase *controler)
{
    static int editingNodeIndex;
    static int editingSlotIndex;

    auto deleteLink = [&controler](int index)
    {
        NodeLink& link = links[index];
        controler->DelLink(link.OutputIdx, link.OutputSlot);
        NodeGraphUpdateEvaluationOrder(controler);
    };
    auto addLink = [&controler](int index)
    {
        NodeLink& link = links[index];
        controler->AddLink(link.InputIdx, link.InputSlot, link.OutputIdx, link.OutputSlot);
        NodeGraphUpdateEvaluationOrder(controler);
    };

    size_t metaNodeCount = gMetaNodes.size();
    const MetaNode* metaNodes = gMetaNodes.data();
    ImGuiIO& io = ImGui::GetIO();
    Node *node = &nodes[nodeIndex];

    // draw/use inputs/outputs
    bool hoverSlot = false;
    for (int i = 0; i < 2; i++)
    {
        float closestDistance = FLT_MAX;
        int closestConn = -1;
        ImVec2 closestTextPos;
        ImVec2 closestPos;
        const size_t slotCount[2] = { node->InputsCount, node->OutputsCount };
        const MetaCon *con = i ? metaNodes[node->mType].mOutputs.data() : metaNodes[node->mType].mInputs.data();
        for (int slot_idx = 0; slot_idx < slotCount[i]; slot_idx++)
        {
            ImVec2 p = offset + (i ? node->GetOutputSlotPos(slot_idx, factor) : node->GetInputSlotPos(slot_idx, factor));
            float distance = Distance(p, io.MousePos);
            bool overCon = (nodeOperation == NO_None || nodeOperation == NO_EditingLink) && (distance < NODE_SLOT_RADIUS * 2.f) && (distance < closestDistance);

            const char *conText = con[slot_idx].mName.c_str();
            ImVec2 textSize;
            textSize = ImGui::CalcTextSize(conText);
            ImVec2 textPos = p + ImVec2(-NODE_SLOT_RADIUS * (i ? -1.f : 1.f)*(overCon ? 3.f : 2.f) - (i ? 0 : textSize.x), -textSize.y / 2);

            ImRect nodeRect(offset + node->Pos, offset + node->Pos + node->Size);
            if (overCon || (nodeRect.Contains(io.MousePos) && slot_idx == 0 && i == 0 && nodeOperation == NO_EditingLink))
            {
                closestDistance = distance;
                closestConn = slot_idx;
                closestTextPos = textPos;
                closestPos = p;
            }

            drawList->AddCircleFilled(p, NODE_SLOT_RADIUS*1.2f, IM_COL32(0, 0, 0, 200));
            drawList->AddCircleFilled(p, NODE_SLOT_RADIUS*0.75f*1.2f, IM_COL32(160, 160, 160, 200));
            drawList->AddText(io.FontDefault, 14, textPos + ImVec2(2, 2), IM_COL32(0, 0, 0, 255), conText);
            drawList->AddText(io.FontDefault, 14, textPos, IM_COL32(150, 150, 150, 255), conText);

        }
        if (closestConn != -1)
        {
            const char *conText = con[closestConn].mName.c_str();

            hoverSlot = true;
            drawList->AddCircleFilled(closestPos, NODE_SLOT_RADIUS*2.f, IM_COL32(0, 0, 0, 200));
            drawList->AddCircleFilled(closestPos, NODE_SLOT_RADIUS*1.5f, IM_COL32(200, 200, 200, 200));
            drawList->AddText(io.FontDefault, 16, closestTextPos + ImVec2(1, 1), IM_COL32(0, 0, 0, 255), conText);
            drawList->AddText(io.FontDefault, 16, closestTextPos, IM_COL32(250, 250, 250, 255), conText);
            bool inputToOutput = (!editingInput && !i) || (editingInput && i);
            if (nodeOperation == NO_EditingLink && !io.MouseDown[0])
            {
                if (inputToOutput)
                {
                    // check loopback
                    NodeLink nl;
                    if (editingInput)
                        nl = NodeLink(nodeIndex, closestConn, editingNodeIndex, editingSlotIndex);
                    else
                        nl = NodeLink(editingNodeIndex, editingSlotIndex, nodeIndex, closestConn);

                    if (RecurseIsLinked(nl.OutputIdx, nl.InputIdx))
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
                        NodeLink& link = links[linkIndex];
                        if (link.OutputIdx == nl.OutputIdx && link.OutputSlot == nl.OutputSlot)
                        {
                            URDel<NodeLink> undoRedoDel(linkIndex, []() { return &links; }, deleteLink, addLink);
                            controler->DelLink(link.OutputIdx, link.OutputSlot);
                            links.erase(links.begin() + linkIndex);
                            NodeGraphUpdateEvaluationOrder(controler);
                            break;
                        }
                    }

                    if (!alreadyExisting)
                    {
                        URAdd<NodeLink> undoRedoAdd(int(links.size()), []() { return &links; }, deleteLink, addLink);

                        links.push_back(nl);
                        controler->AddLink(nl.InputIdx, nl.InputSlot, nl.OutputIdx, nl.OutputSlot);
                        NodeGraphUpdateEvaluationOrder(controler);
                    }
                }
            }
            if (nodeOperation == NO_None && io.MouseDown[0])
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
                        NodeLink& link = links[linkIndex];
                        if (link.OutputIdx == nodeIndex && link.OutputSlot == closestConn)
                        {
                            URDel<NodeLink> undoRedoDel(linkIndex, []() { return &links; }, deleteLink, addLink);
                            controler->DelLink(link.OutputIdx, link.OutputSlot);
                            links.erase(links.begin() + linkIndex);
                            NodeGraphUpdateEvaluationOrder(controler);
                            break;
                        }
                    }
                }
            }
        }
    }
}

static void DrawGrid(ImDrawList* drawList, ImVec2 windowPos, const ImVec2 canvasSize, const float factor)
{
    ImU32 GRID_COLOR = IM_COL32(100, 100, 100, 40);
    float GRID_SZ = 64.0f * factor;
    for (float x = fmodf(scrolling.x*factor, GRID_SZ); x < canvasSize.x; x += GRID_SZ)
        drawList->AddLine(ImVec2(x, 0.0f) + windowPos, ImVec2(x, canvasSize.y) + windowPos, GRID_COLOR);
    for (float y = fmodf(scrolling.y*factor, GRID_SZ); y < canvasSize.y; y += GRID_SZ)
        drawList->AddLine(ImVec2(0.0f, y) + windowPos, ImVec2(canvasSize.x, y) + windowPos, GRID_COLOR);
}

// return true if node is hovered
static bool DrawNode(ImDrawList* drawList, int nodeIndex, const ImVec2 offset, const float factor, NodeGraphControlerBase *controler)
{
    ImGuiIO& io = ImGui::GetIO();
    const MetaNode* metaNodes = gMetaNodes.data();
    Node *node = &nodes[nodeIndex];
    ImVec2 node_rect_min = offset + node->Pos * factor;

    bool old_any_active = ImGui::IsAnyItemActive();
    ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);

    const bool nodeIsCompute = controler->NodeIsCompute(nodeIndex);
    if (nodeIsCompute)
        ImGui::InvisibleButton("canvas", ImVec2(100, 50) * factor);
    else
        ImGui::InvisibleButton("canvas", ImVec2(100, 100) * factor);
    bool node_moving_active = ImGui::IsItemActive(); // must be called right after creating the control we want to be able to move

    // Save the size of what we have emitted and whether any of the widgets are being used
    bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
    node->Size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
    ImVec2 node_rect_max = node_rect_min + node->Size;

    // Display node box
    drawList->ChannelsSetCurrent(1); // Background

    ImGui::SetCursorScreenPos(node_rect_min);
    ImGui::InvisibleButton("node", node->Size);
    bool nodeHovered = false;
    if (ImGui::IsItemHovered() && nodeOperation == NO_None)
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
                    for (auto& selnode : nodes)
                        selnode.mbSelected = false;
                }
                node->mbSelected = true;
            }
        }
    }
    if (node_moving_active && io.MouseDown[0])
    {
        nodeOperation = NO_MovingNodes;
    }

    bool currentSelectedNode = node->mbSelected;

    ImU32 node_bg_color = nodeHovered ? IM_COL32(85, 85, 85, 255) : IM_COL32(60, 60, 60, 255);

    drawList->AddRect(node_rect_min, node_rect_max, currentSelectedNode ? IM_COL32(255, 130, 30, 255) : IM_COL32(100, 100, 100, 0), 2.0f, 15, currentSelectedNode ? 6.f : 2.f);

    ImVec2 imgPos = node_rect_min + ImVec2(14, 25);
    ImVec2 imgSize = node_rect_max + ImVec2(-5, -5) - imgPos;
    float imgSizeComp = std::min(imgSize.x, imgSize.y);

    drawList->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 2.0f);
    float progress = controler->NodeProgress(nodeIndex);
    if (progress > FLT_EPSILON && progress < 1.f - FLT_EPSILON)
    {
        ImVec2 progressLineA = node_rect_max - ImVec2(node->Size.x - 2.f, 3.f);
        ImVec2 progressLineB = progressLineA + ImVec2(node->Size.x - 4.f, 0.f);
        drawList->AddLine(progressLineA, progressLineB, 0xFF400000, 3.f);
        drawList->AddLine(progressLineA, ImLerp(progressLineA, progressLineB, progress), 0xFFFF0000, 3.f);
    }
    ImVec2 imgPosMax = imgPos + ImVec2(imgSizeComp, imgSizeComp);
    if (!nodeIsCompute)
        drawList->AddRectFilled(imgPos, imgPosMax, 0xFF000000);

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

    if (controler->NodeIsProcesing(nodeIndex) == 1)
    {
        AddUICustomDraw(drawList, ImRect(imgPos, imgPosMax), EvaluationStages::DrawUIProgress, nodeIndex);
        //drawList->AddCallback((ImDrawCallback)(Evaluation::NodeUICallBack), (void*)(AddNodeUICallbackRect(CBUI_Progress, ImRect(imgPos, imgPosMax), nodeIndex)));
    }
    else if (controler->NodeIsCubemap(nodeIndex))
    {
        AddUICustomDraw(drawList, ImRect(imgPos, imgPosMax), EvaluationStages::DrawUICubemap, nodeIndex);
        //drawList->AddCallback((ImDrawCallback)(Evaluation::NodeUICallBack), (void*)(AddNodeUICallbackRect(CBUI_Cubemap, ImRect(imgPos + marge, imgPosMax - marge), nodeIndex)));
    }
    else if (nodeIsCompute)
    {
    }
    else
    {
        drawList->AddImage((ImTextureID)(int64_t)(controler->GetNodeTexture(size_t(nodeIndex))), imgPos + marge, imgPosMax - marge, ImVec2(0, 1), ImVec2(1, 0));
    }

    drawList->AddRectFilled(node_rect_min, ImVec2(node_rect_max.x, node_rect_min.y + 20), metaNodes[node->mType].mHeaderColor, 2.0f);
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
        drawList->AddImageQuad((ImTextureID)(uint64_t)stageCompute, bmpInfoPos, bmpInfoPos + ImVec2(bmpInfoSize.x, 0.f), bmpInfoPos + bmpInfoSize, bmpInfoPos + ImVec2(0., bmpInfoSize.y));
    }
    else if (controler->NodeIs2D(nodeIndex))
    {
        drawList->AddImageQuad((ImTextureID)(uint64_t)stage2D, bmpInfoPos, bmpInfoPos + ImVec2(bmpInfoSize.x, 0.f), bmpInfoPos + bmpInfoSize, bmpInfoPos + ImVec2(0., bmpInfoSize.y));
    }
    else if (controler->NodeIsCubemap(nodeIndex))
    {
        drawList->AddImageQuad((ImTextureID)(uint64_t)stagecubemap, bmpInfoPos + ImVec2(0., bmpInfoSize.y), bmpInfoPos + bmpInfoSize, bmpInfoPos + ImVec2(bmpInfoSize.x, 0.f), bmpInfoPos);
    }
    return nodeHovered;
}

void ComputeDelegateSelection(NodeGraphControlerBase *controler)
{
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

void NodeGraphSelectNode(int selectedNodeIndex)
{
    for (size_t i = 0; i < nodes.size(); i++)
    {
        nodes[i].mbSelected = selectedNodeIndex == i;
    }
}

void NodeGraph(NodeGraphControlerBase *controler, bool enabled)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.f);// ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);

    const ImVec2 windowPos = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetWindowSize();

    ImRect regionRect(windowPos, windowPos + canvasSize);

    HandleZoomScroll(regionRect);
    ImVec2 offset = ImGui::GetCursorScreenPos() + scrolling * factor;

    {
        ImGui::BeginChild("rugs_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        editRug = DisplayRugs(editRug, drawList, offset, factor);

        ImGui::EndChild();
    }
    
    ImGui::SetCursorPos(windowPos);
    ImGui::BeginGroup();

    

    ImGuiIO& io = ImGui::GetIO();

    // Create our child canvas
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(30, 30, 30, 200));

    ImGui::SetCursorPos(ImVec2(0, 50));
    ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
    ImGui::PushItemWidth(120.0f);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    //editRug = DisplayRugs(editRug, drawList, offset, factor);

    // Display grid
    DrawGrid(drawList, windowPos, canvasSize, factor);

    if (!enabled)
        goto nodeGraphExit;

    // Display links
    drawList->ChannelsSplit(3);
    drawList->ChannelsSetCurrent(1); // Background
    DisplayLinks(drawList, offset, factor, regionRect);

    // edit node link
    if (nodeOperation == NO_EditingLink)
    {
        ImVec2 p1 = editingNodeSource;
        ImVec2 p2 = io.MousePos;
        //drawList->AddBezierCurve(p1, p1 + ImVec2(editingInput ?-50.f:+50.f, 0.f), p2 + ImVec2(editingInput ?50.f:-50.f, 0.f), p2, IM_COL32(200, 200, 100, 255), 3.0f);
        drawList->AddLine(p1, p2, IM_COL32(200, 200, 200, 255), 3.0f);
    }

    // Display nodes
    int hoveredNode = -1;
    for (int i = 0; i < 2; i++)
    {
        for (int nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
        {
            Node* node = &nodes[nodeIndex];
            if (node->mbSelected != (i != 0))
                continue;
            ImVec2 node_rect_min = offset + node->Pos * factor;

            // node view clipping
            ImVec2 p1 = node_rect_min;
            ImVec2 p2 = node_rect_min + ImVec2(100, 100) * factor;
            if (!regionRect.Overlaps(ImRect(p1, p2)))
                continue;

            ImGui::PushID(nodeIndex);

            // Display node contents first
            drawList->ChannelsSetCurrent(2); // Foreground

            if (DrawNode(drawList, nodeIndex, offset, factor, controler))
                hoveredNode = nodeIndex;

            HandleConnections(drawList, nodeIndex, offset, factor, controler);
            ImGui::PopID();
        }
    }

    if (nodeOperation == NO_MovingNodes)
    {
        for (auto& node : nodes)
        {
            if (!node.mbSelected)
                continue;
            node.Pos += io.MouseDelta / factor; 
        }
    }

    // rugs
    drawList->ChannelsSetCurrent(0);
    

    // quad selection
    HandleQuadSelection(drawList, offset, factor, regionRect);

    drawList->ChannelsMerge();

    if (editRug)
    {
        if (EditRug(editRug, drawList, offset, factor))
            editRug = NULL;
    }
    
    // releasing mouse button means it's done in any operation
    if (nodeOperation != NO_None && !io.MouseDown[0])
        nodeOperation = NO_None;

    // Open context menu
    bool openContextMenu = false;
    static int contextMenuHoverNode = -1;
    if (nodeOperation == NO_None && regionRect.Contains(io.MousePos) && (ImGui::IsMouseClicked(1) || (ImGui::IsWindowFocused() && ImGui::IsKeyPressedMap(ImGuiKey_Tab))))
    {
        openContextMenu = true;
        contextMenuHoverNode = hoveredNode;
    }
    
    if (openContextMenu)
        ImGui::OpenPopup("context_menu");
    ContextMenu(offset, contextMenuHoverNode, controler);
    
    // Scrolling
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(2, 0.0f))//&& ImGui::IsWindowFocused())
    {
        scrolling += io.MouseDelta / factor;
    }

nodeGraphExit:;
    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(2);

    ComputeDelegateSelection(controler);
    
    ImGui::EndGroup();
    ImGui::PopStyleVar(3);
}