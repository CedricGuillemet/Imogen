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
#include "Nodes.h"
#include <vector>
#include <algorithm>
#include <assert.h>
#include "Evaluation.h"
#include "imgui_stdlib.h"

UndoRedoHandler undoRedoHandler;
int Log(const char *szFormat, ...);
void AddExtractedView(size_t nodeIndex);

static inline float Distance(ImVec2& a, ImVec2& b) { return sqrtf((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y)); }

Node::Node(int type, const ImVec2& pos)
{
	mType = type;
	Pos = pos;
	Size = ImVec2(100, 100);
	InputsCount = gMetaNodes[type].mInputs.size();
	OutputsCount = gMetaNodes[type].mOutputs.size();
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
static std::vector<NodeLink> links;
static std::vector<NodeRug> rugs;
static NodeRug *editRug = NULL;
static NodeRug* movingRug = NULL;
static NodeRug* sizingRug = NULL;

void NodeGraphClear()
{
	nodes.clear();
	links.clear();
	rugs.clear();
	editRug = NULL;
}

const std::vector<NodeLink>& NodeGraphGetLinks()
{
	return links;
}

const std::vector<NodeRug>& NodeGraphRugs()
{
	return rugs;
}

NodeRug* DisplayRugs(NodeRug *editRug, ImDrawList* draw_list, ImVec2 offset, float factor, bool editingNodeAndConnexions)
{
	ImGuiIO& io = ImGui::GetIO();
	NodeRug *ret = editRug;
	int id = 900;
	for (NodeRug& rug : rugs)
	{
		if (&rug == editRug)
			continue;
		ImGui::PushID(id++);
		ImVec2 commentSize = rug.mSize * factor;

		ImVec2 node_rect_min = offset + rug.mPos * factor;

		ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);

		ImVec2 node_rect_max = node_rect_min + commentSize;

		// Display node box
		ImRect rugRect(node_rect_min, node_rect_max);
		ImRect insideSizingRect(node_rect_min + commentSize - ImVec2(30, 30), node_rect_min + commentSize);

		if (rugRect.Contains(io.MousePos) && io.MouseDoubleClicked[0])
		{
			ret = &rug;
		}

		if (!editingNodeAndConnexions && !sizingRug && !movingRug && insideSizingRect.Contains(io.MousePos) && io.MouseDown[0])
			sizingRug = &rug;
		if (sizingRug && !io.MouseDown[0])
			sizingRug = NULL;

		if (!editingNodeAndConnexions && !movingRug && !sizingRug && rugRect.Contains(io.MousePos) && !insideSizingRect.Contains(io.MousePos) && io.MouseDown[0])
			movingRug = &rug;
		if (movingRug && !io.MouseDown[0])
			movingRug = NULL;

		draw_list->AddText(ImGui::GetIO().FontDefault, 14 * ImLerp(1.f, factor, 0.5f), node_rect_min + ImVec2(5, 5), (rug.mColor & 0xFFFFFF) + 0xFF404040, rug.mText.c_str());
		draw_list->AddRectFilled(node_rect_min, node_rect_max, (rug.mColor&0xFFFFFF)+0x60000000, 10.0f, 15);
		draw_list->AddRect(node_rect_min, node_rect_max, (rug.mColor & 0xFFFFFF) + 0x90000000, 10.0f, 15, 2.f);
		draw_list->AddTriangleFilled(node_rect_min + commentSize - ImVec2(25, 8), node_rect_min + commentSize - ImVec2(8, 25), node_rect_min + commentSize - ImVec2(8, 8), (rug.mColor & 0xFFFFFF) + 0x90000000);
		ImGui::PopID();
	}

	if (sizingRug && ImGui::IsMouseDragging(0))
		sizingRug->mSize += ImGui::GetIO().MouseDelta;
	if (movingRug && ImGui::IsMouseDragging(0))
		movingRug->mPos += ImGui::GetIO().MouseDelta;

	return ret;
}

bool EditRug(NodeRug *rug, ImDrawList* draw_list, ImVec2 offset, float factor)
{
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 commentSize = rug->mSize * factor;

	ImVec2 node_rect_min = (offset + rug->mPos) * factor;
	ImVec2 node_rect_max = node_rect_min + commentSize;
	ImRect rugRect(node_rect_min, node_rect_max);

	ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
	

	draw_list->AddRectFilled(node_rect_min, node_rect_max, (rug->mColor & 0xFFFFFF) + 0xE0000000, 10.0f, 15);
	draw_list->AddRect(node_rect_min, node_rect_max, (rug->mColor & 0xFFFFFF) + 0xFF000000, 10.0f, 15, 2.f);

	ImGui::SetCursorScreenPos(node_rect_min + ImVec2(5, 5));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
	ImGui::InputTextMultiline("", &rug->mText, (node_rect_max - node_rect_min) - ImVec2(30, 30));
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
		}
		ImGui::PopStyleColor(3);
		ImGui::PopID();
	}
	ImGui::SameLine(0, 50);

	if (ImGui::Button("Delete"))
	{
		rugs.erase(rugs.begin() + (rug - rugs.data()));
		return true;
	}
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

void NodeGraphUpdateEvaluationOrder(NodeGraphDelegate *delegate)
{
	mOrders = ComputeEvaluationOrder(links, nodes.size());
	std::sort(mOrders.begin(), mOrders.end());
	std::vector<size_t> nodeOrderList(mOrders.size());
	for (size_t i = 0; i < mOrders.size(); i++)
		nodeOrderList[i] = mOrders[i].mNodeIndex;
	delegate->UpdateEvaluationList(nodeOrderList);
}

void NodeGraphAddNode(NodeGraphDelegate *delegate, int type, void *parameters, int posx, int posy, int frameStart, int frameEnd)
{
	size_t index = nodes.size();
	nodes.push_back(Node(type, ImVec2(float(posx), float(posy))));
	delegate->AddNode(type);
	delegate->SetParamBlock(index, (unsigned char*)parameters);
	delegate->SetTimeSlot(index, frameStart, frameEnd);
}

void NodeGraphAddLink(NodeGraphDelegate *delegate, int InputIdx, int InputSlot, int OutputIdx, int OutputSlot)
{
	NodeLink nl;
	nl.InputIdx = InputIdx;
	nl.InputSlot = InputSlot;
	nl.OutputIdx = OutputIdx;
	nl.OutputSlot = OutputSlot;
	links.push_back(nl);
	delegate->AddLink(nl.InputIdx, nl.InputSlot, nl.OutputIdx, nl.OutputSlot);
}

ImVec2 NodeGraphGetNodePos(size_t index)
{
	return nodes[index].Pos;
}

static ImVec2 scrolling = ImVec2(0.0f, 0.0f);

void NodeGraphUpdateScrolling()
{
	if (nodes.empty())
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

void NodeGraph(NodeGraphDelegate *delegate, bool enabled)
{
	size_t metaNodeCount = gMetaNodes.size();
	const MetaNode* metaNodes = gMetaNodes.data();

	static bool editingNode = false;
	static ImVec2 editingNodeSource;
	static bool editingInput = false;
	static int editingNodeIndex;
	static int editingSlotIndex;
	
	static bool show_grid = true;

	int node_selected = delegate->mSelectedNodeIndex;

	int node_hovered_in_list = -1;
	int node_hovered_in_scene = -1;
	bool open_context_menu = false;

	ImGui::BeginGroup();

	ImGuiIO& io = ImGui::GetIO();
	const ImVec2 win_pos = ImGui::GetCursorScreenPos();
	const ImVec2 canvas_sz = ImGui::GetWindowSize();
	ImRect regionRect(win_pos, win_pos + canvas_sz);

	// zoom factor handling
	static float factor = 1.0f;
	static float factorTarget = 1.0f;

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
	scrolling += mouseWPosPost - mouseWPosPre;

	// Create our child canvas
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(30, 30, 30, 200));

	ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::PushItemWidth(120.0f);

	ImVec2 offset = ImGui::GetCursorScreenPos() + scrolling * factor;
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Display grid
	if (show_grid)
	{
		ImU32 GRID_COLOR = IM_COL32(100, 100, 100, 40);
		float GRID_SZ = 64.0f * factor;
		for (float x = fmodf(scrolling.x*factor, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
			draw_list->AddLine(ImVec2(x, 0.0f) + win_pos, ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
		for (float y = fmodf(scrolling.y*factor, GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
			draw_list->AddLine(ImVec2(0.0f, y) + win_pos, ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);
	}

	if (!enabled)
	{
		ImGui::PopItemWidth();
		ImGui::EndChild();
		ImGui::PopStyleColor(1);
		ImGui::PopStyleVar(2);

		ImGui::EndGroup();
		return;
	}

	// Display links
	draw_list->ChannelsSplit(3);
	draw_list->ChannelsSetCurrent(1); // Background
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
		draw_list->AddBezierCurve(p1, p1 + ImVec2(+50, 0) * factor, p2 + ImVec2(-50, 0) * factor, p2, IM_COL32(200, 200, 150, 255), 3.0f * factor);
	}

	// edit node
	if (editingNode)
	{
		ImVec2 p1 = editingNodeSource;
		ImVec2 p2 = ImGui::GetIO().MousePos;
		draw_list->AddBezierCurve(p1, p1 + ImVec2(editingInput ?-50.f:+50.f, 0.f), p2 + ImVec2(editingInput ?50.f:-50.f, 0.f), p2, IM_COL32(200, 200, 100, 255), 3.0f);
	}

	int nodeToDelete = -1;
	bool editingNodeAndConnexions = false;
	static bool isMovingNode = false;

	// Display nodes
	for (int node_idx = 0; node_idx < nodes.size(); node_idx++)
	{
		Node* node = &nodes[node_idx];
		ImVec2 node_rect_min = offset + node->Pos * factor;

		// node view clipping
		ImVec2 p1 = node_rect_min;
		ImVec2 p2 = node_rect_min + ImVec2(100, 100) * factor;
		if ((p1.y < 0.f && p2.y < 0.f) || (p1.y > regionRect.Max.y && p2.y > regionRect.Max.y) ||
			(p1.x < 0.f && p2.x < 0.f) || (p1.x > regionRect.Max.x && p2.x > regionRect.Max.x))
			continue;

		ImGui::PushID(node_idx);

		// Display node contents first
		draw_list->ChannelsSetCurrent(2); // Foreground
		bool old_any_active = ImGui::IsAnyItemActive();
		ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);

		ImGui::InvisibleButton("canvas", ImVec2(100, 100) * factor);
		bool node_moving_active = ImGui::IsItemActive(); // must be called right after creating the control we want to be able to move
		editingNodeAndConnexions |= node_moving_active;

		// Save the size of what we have emitted and whether any of the widgets are being used
		bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
		node->Size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
		ImVec2 node_rect_max = node_rect_min + node->Size;

		draw_list->AddRectFilled(node_rect_min, ImVec2(node_rect_max.x, node_rect_min.y + 20), metaNodes[node->mType].mHeaderColor, 2.0f);
		draw_list->AddText(node_rect_min+ImVec2(2,2), IM_COL32(0, 0, 0, 255), metaNodes[node->mType].mName.c_str());

		// Display node box
		draw_list->ChannelsSetCurrent(1); // Background
		ImGui::SetCursorScreenPos(node_rect_min);
		ImGui::InvisibleButton("node", node->Size);
		if (ImGui::IsItemHovered())
		{
			node_hovered_in_scene = node_idx;
			open_context_menu |= ImGui::IsMouseClicked(1);
		}
		
		if (node_widgets_active || node_moving_active)
			node_selected = node_idx;

		if (node_moving_active && ImGui::IsMouseDragging(0))
		{
			node->Pos += ImGui::GetIO().MouseDelta / factor;
			isMovingNode = true;
		}

		if (!io.MouseDown[0])
			isMovingNode = false;

		bool currentSelectedNode = node_selected == node_idx;

		ImU32 node_bg_color = (node_hovered_in_list == node_idx || node_hovered_in_scene == node_idx || (node_hovered_in_list == -1 && currentSelectedNode)) ? IM_COL32(85, 85, 85, 255) : IM_COL32(60, 60, 60, 255);
		
		if (delegate->mBakeTargetIndex == node_idx)
		{
			//draw_list->AddRect(node_rect_min, node_rect_max, IM_COL32(230, 100, 30, 255) , 2.0f, 15, 4.f);
			draw_list->AddRectFilled(node_rect_min, node_rect_max, IM_COL32(230, 100, 30, 255), 2.0f);
		}
		else
			draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 2.0f);
		draw_list->AddRect(node_rect_min, node_rect_max, currentSelectedNode? IM_COL32(230, 230, 230, 255):IM_COL32(100, 100, 100, 0), 2.0f, 15, 2.f);

		//ImVec2 offsetImg = ImGui::GetCursorScreenPos();

		ImVec2 imgPos = node_rect_min + ImVec2(14, 25);
		ImVec2 imgSize = node_rect_max + ImVec2(-5, -5) - imgPos;
		float imgSizeComp = std::min(imgSize.x, imgSize.y);
		
		ImVec2 imgPosMax = imgPos + ImVec2(imgSizeComp, imgSizeComp);
		draw_list->AddRectFilled(imgPos, imgPosMax, 0xFF000000);

		ImVec2 imageSize = delegate->GetEvaluationSize(node_idx);
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
		
		if (delegate->NodeIsProcesing(node_idx))
			draw_list->AddCallback((ImDrawCallback)(Evaluation::NodeUICallBack), (void*)(AddNodeUICallbackRect(CBUI_Progress, ImRect(imgPos, imgPosMax), node_idx)));
		else if (delegate->NodeIsCubemap(node_idx))
			draw_list->AddCallback((ImDrawCallback)(Evaluation::NodeUICallBack), (void*)(AddNodeUICallbackRect(CBUI_Cubemap, ImRect(imgPos + marge, imgPosMax - marge), node_idx)));
		else
			draw_list->AddImage((ImTextureID)(int64_t)(delegate->GetNodeTexture(size_t(node_idx))), imgPos + marge, imgPosMax - marge, ImVec2(0, 1), ImVec2(1, 0));

		// draw/use inputs/outputs
		bool hoverSlot = false;
		for (int i = 0; i < 2; i++)
		{
			const size_t slotCount[2] = { node->InputsCount, node->OutputsCount };
			const MetaCon *con = i ? metaNodes[node->mType].mOutputs.data() : metaNodes[node->mType].mInputs.data();
			for (int slot_idx = 0; slot_idx < slotCount[i]; slot_idx++)
			{
				ImVec2 p = offset + (i ? node->GetOutputSlotPos(slot_idx, factor) : node->GetInputSlotPos(slot_idx, factor));
				bool overCon = (!isMovingNode) && (movingRug == NULL && sizingRug == NULL) && !hoverSlot && (node_hovered_in_scene == -1 || editingNode) && Distance(p, ImGui::GetIO().MousePos) < NODE_SLOT_RADIUS*2.f;

				const char *conText = con[slot_idx].mName.c_str();
				ImVec2 textSize;
				textSize = ImGui::CalcTextSize(conText);
				ImVec2 textPos = p + ImVec2(-NODE_SLOT_RADIUS*(i ? -1.f : 1.f)*(overCon ? 3.f : 2.f) - (i ? 0:textSize.x ), -textSize.y / 2);

				if (overCon)
				{
					hoverSlot = true;
					draw_list->AddCircleFilled(p, NODE_SLOT_RADIUS*2.f, IM_COL32(0, 0, 0, 200));
					draw_list->AddCircleFilled(p, NODE_SLOT_RADIUS*1.5f, IM_COL32(200, 200, 200, 200));
					draw_list->AddText(io.FontDefault, 16, textPos+ImVec2(1,1), IM_COL32(0, 0, 0, 255), conText);
					draw_list->AddText(io.FontDefault, 16, textPos, IM_COL32(250, 250, 250, 255), conText);
					bool inputToOutput = (!editingInput && !i) || (editingInput&& i);
					if (editingNode && !ImGui::GetIO().MouseDown[0])
					{
						if (inputToOutput)
						{
							editingNode = false;

							// check loopback

							NodeLink nl;
							if (editingInput)
								nl = NodeLink(node_idx, slot_idx, editingNodeIndex, editingSlotIndex);
							else
								nl = NodeLink(editingNodeIndex, editingSlotIndex, node_idx, slot_idx);

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
									delegate->DelLink(link.OutputIdx, link.OutputSlot);
									links.erase(links.begin() + linkIndex);
									NodeGraphUpdateEvaluationOrder(delegate);
									break;
								}
							}

							if (!alreadyExisting)
							{
								links.push_back(nl);
								delegate->AddLink(nl.InputIdx, nl.InputSlot, nl.OutputIdx, nl.OutputSlot);
								NodeGraphUpdateEvaluationOrder(delegate);
							}
						}
					}
					if (!editingNode && ImGui::GetIO().MouseDown[0])
					{
						editingNode = true;
						editingInput = i == 0;
						editingNodeSource = p;
						editingNodeIndex = node_idx;
						editingSlotIndex = slot_idx;
						if (editingInput)
						{
							// remove existing link
							for (int linkIndex = 0; linkIndex < links.size(); linkIndex++)
							{
								NodeLink& link = links[linkIndex];
								if (link.OutputIdx == node_idx && link.OutputSlot == slot_idx)
								{
									delegate->DelLink(link.OutputIdx, link.OutputSlot);
									links.erase(links.begin() + linkIndex);
									NodeGraphUpdateEvaluationOrder(delegate);
									break;
								}
							}
						}
					}
				}
				else
				{
					draw_list->AddCircleFilled(p, NODE_SLOT_RADIUS*1.2f, IM_COL32(0, 0, 0, 200));
					draw_list->AddCircleFilled(p, NODE_SLOT_RADIUS*0.75f*1.2f, IM_COL32(160, 160, 160, 200));
					draw_list->AddText(io.FontDefault, 14, textPos + ImVec2(2, 2), IM_COL32(0, 0, 0, 255), conText);
					draw_list->AddText(io.FontDefault, 14, textPos, IM_COL32(150, 150, 150, 255), conText);
					
				}
			}
		}
		editingNodeAndConnexions |= editingNode || hoverSlot;
		ImGui::PopID();
	}

	// release mouse anywhere but over a slot
	if (editingNode&&!ImGui::GetIO().MouseDown[0])
	{
		editingNode = false;
	}
	// rugs
	draw_list->ChannelsSetCurrent(0);
	editRug = DisplayRugs(editRug, draw_list, offset, factor, editingNodeAndConnexions);

	draw_list->ChannelsMerge();

	if (editRug)
	{
		if (EditRug(editRug, draw_list, offset, factor))
			editRug = NULL;
	}
	// Open context menu
	if (!ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseClicked(1))
	{
		node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
		open_context_menu = true;
	}
	if (open_context_menu)
	{
		ImGui::OpenPopup("context_menu");
		if (node_hovered_in_list != -1)
			node_selected = node_hovered_in_list;
		if (node_hovered_in_scene != -1)
			node_selected = node_hovered_in_scene;
	}

	// Draw context menu
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	if (ImGui::BeginPopup("context_menu"))
	{
		Node* node = node_selected != -1 ? &nodes[node_selected] : NULL;
		ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
		if (node)
		{
			ImGui::Text(metaNodes[node->mType].mName.c_str());
			ImGui::Separator();
			//if (ImGui::MenuItem("Rename..", NULL, false, false)) {}
			if (ImGui::MenuItem("Extract view", NULL, false))
			{
				AddExtractedView(node_selected);
			}
			if (ImGui::MenuItem("Delete", NULL, false)) 
			{
				if (delegate->mBakeTargetIndex == node_selected)
					delegate->mBakeTargetIndex = -1;

				for (int id = 0; id < links.size(); id++)
				{
					if (links[id].InputIdx == node_selected || links[id].OutputIdx == node_selected)
						delegate->DelLink(links[id].OutputIdx, links[id].OutputSlot);
				}
				auto iter = links.begin();
				for (; iter != links.end();)
				{
					if (iter->InputIdx == node_selected || iter->OutputIdx == node_selected)
						iter = links.erase(iter);
					else
						iter++;
				}
				// recompute link indices
				for (int id = 0; id < links.size(); id++)
				{
					if (links[id].InputIdx > node_selected)
						links[id].InputIdx--;
					if (links[id].OutputIdx > node_selected)
						links[id].OutputIdx--;
				}

				// inform delegate
				delegate->DeleteNode(node_selected);

				// delete links
				nodes.erase(nodes.begin() + node_selected);
				NodeGraphUpdateEvaluationOrder(delegate);
				node_selected = -1;
			}
		}
		else
		{
			auto AddNode = [&](int i)
			{
				nodes.push_back(Node(i, scene_pos));
				delegate->AddNode(i);
				NodeGraphUpdateEvaluationOrder(delegate);
				node_selected = int(nodes.size()) - 1;
			};
			if (ImGui::MenuItem("Add rug", NULL, false))
			{
				rugs.push_back({ scene_pos, ImVec2(400,200), 0xFFA0A0A0, "Description\nEdit me with a double click." });
			}
			static char inputText[64] = { 0 };
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

					for (int iCateg = 0; iCateg < delegate->mCategoriesCount; iCateg++)
					{
						if (ImGui::BeginMenu(delegate->mCategories[iCateg]))
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
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();

	// Scrolling
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(2, 0.0f))
		scrolling += ImGui::GetIO().MouseDelta / factor;

	ImGui::PopItemWidth();
	ImGui::EndChild();
	ImGui::PopStyleColor(1);
	ImGui::PopStyleVar(2);

	delegate->mSelectedNodeIndex = node_selected;
	
	ImGui::EndGroup();
}