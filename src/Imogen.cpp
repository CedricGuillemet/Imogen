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
unsigned char *stbi_write_png_to_mem(unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len);
extern Evaluation gEvaluation;

extern enki::TaskScheduler g_TS;

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

//		if (ScrollToBottom)
//			ImGui::SetScrollHere(1.0f);
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
colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
colors[ImGuiCol_TabActive] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
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
void InitCallbackRects()
{
	mCallbackRects.clear();
}
size_t AddNodeUICallbackRect(const ImRect& rect, size_t nodeIndex)
{
	mCallbackRects.push_back({ rect, nodeIndex });
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
		editor.SetText(evaluation.GetEvaluator(mEvaluatorFiles[currentShaderIndex].mFilename));
	}
	auto cpos = editor.GetCursorPosition();
	ImGui::BeginChild(13, ImVec2(250, 800));
	for (size_t i = 0; i < mEvaluatorFiles.size(); i++)
	{
		bool selected = i == currentShaderIndex;
		if (ImGui::Selectable(mEvaluatorFiles[i].mFilename.c_str(), &selected))
		{
			currentShaderIndex = int(i);
			editor.SetText(evaluation.GetEvaluator(mEvaluatorFiles[currentShaderIndex].mFilename));
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

		// TODO
		evaluation.SetEvaluators(mEvaluatorFiles);
		nodeGraphDelegate.InvalidateParameters();
	}

	ImGui::SameLine();
	ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | F5 to save and update nodes", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
		editor.IsOverwrite() ? "Ovr" : "Ins",
		editor.CanUndo() ? "*" : " ",
		editor.GetLanguageDefinition().mName.c_str());
	editor.Render("TextEditor");
	ImGui::EndChild();

}

void NodeEdit(TileNodeEditGraphDelegate& nodeGraphDelegate, Evaluation& evaluation)
{
	ImGuiIO& io = ImGui::GetIO();

	int selNode = nodeGraphDelegate.mSelectedNodeIndex;
	if (ImGui::CollapsingHeader("Preview", 0, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF000000);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF000000);
		ImGui::PushStyleColor(ImGuiCol_Button, 0xFF000000);
		float w = ImGui::GetWindowContentRegionWidth();
		int imageWidth(1), imageHeight(1);
		Evaluation::GetEvaluationSize(selNode, &imageWidth, &imageHeight);
		float ratio = float(imageHeight)/float(imageWidth);
		float h = w * ratio;
		ImVec2 p = ImGui::GetCursorPos() + ImGui::GetWindowPos();

		ImGui::ImageButton((ImTextureID)(int64_t)((selNode != -1) ? evaluation.GetEvaluationTexture(selNode) : 0), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(1);
		ImRect rc(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		if (selNode != -1 && nodeGraphDelegate.NodeIsCubemap(selNode))
		{
			draw_list->AddCallback((ImDrawCallback)(Evaluation::NodeUICallBack), (void*)(AddNodeUICallbackRect(rc, -2)));
		}
		else if (selNode != -1 && nodeGraphDelegate.NodeHasUI(selNode))
		{
			draw_list->AddCallback((ImDrawCallback)(Evaluation::NodeUICallBack), (void*)(AddNodeUICallbackRect(rc, selNode)));
		}
		if (rc.Contains(io.MousePos))
		{
			ImVec2 ratio((io.MousePos.x - rc.Min.x) / rc.GetSize().x, (io.MousePos.y - rc.Min.y) / rc.GetSize().y);
			ImVec2 deltaRatio((io.MouseDelta.x) / rc.GetSize().x, (io.MouseDelta.y) / rc.GetSize().y);
			nodeGraphDelegate.SetMouse(ratio.x, ratio.y, deltaRatio.x, deltaRatio.y, io.MouseDown[0], io.MouseDown[1]);
		}
		else
		{
			nodeGraphDelegate.SetMouse(-9999.f, -9999.f, -9999.f, -9999.f, false, false);
		}
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
	PinnedTaskUploadImage(Image image, ASyncId identifier, bool isThumbnail)
		: enki::IPinnedTask(0) // set pinned thread to 0
		, mImage(image)
		, mIdentifier(identifier)
		, mbIsThumbnail(isThumbnail)
	{
	}

	virtual void Execute()
	{
		unsigned int textureId = Evaluation::UploadImage(&mImage, 0);
		if (mbIsThumbnail)
		{
			Material* material = library.Get(mIdentifier);
			if (material)
				material->mThumbnailTextureId = textureId;
		}
		else
		{
			TileNodeEditGraphDelegate::ImogenNode *node = TileNodeEditGraphDelegate::GetInstance()->Get(mIdentifier);
			if (node)
			{
				node->mbProcessing = false;
				Evaluation::SetEvaluationImage(int(node->mEvaluationTarget), &mImage);
				gEvaluation.SetEvaluationParameters(node->mEvaluationTarget, node->mParameters, node->mParametersSize);
			}
			Evaluation::FreeImage(&mImage);
		}
	}
	Image mImage;
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
			image.mBits = data;
			PinnedTaskUploadImage uploadTexTask(image, mIdentifier, true);
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
		unsigned char *bits = stbi_write_png_to_mem((unsigned char*)mImage.mBits, mImage.mWidth * components, mImage.mWidth, mImage.mHeight, components, &outlen);
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
			image.mBits = data;
			PinnedTaskUploadImage uploadTexTask(image, mIdentifier, false);
			g_TS.AddPinnedTask(&uploadTexTask);
			g_TS.WaitforTask(&uploadTexTask);
		}
		delete this;
	}
	ASyncId mIdentifier;
	std::vector<uint8_t> *mSrc;
};

template <typename T, typename Ty> bool TVRes(std::vector<T, Ty>& res, const char *szName, int &selection, int index, Evaluation& evaluation)
{
	bool ret = false;
	if (!ImGui::TreeNodeEx(szName, ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_DefaultOpen))
		return ret;

	std::string currentGroup = "";
	bool currentGroupIsSkipped = false;

	std::vector<SortedResource<T, Ty>> sortedResources;
	SortedResource<T, Ty>::ComputeSortedResources(res, sortedResources);
	unsigned int defaultTextureId = evaluation.GetTexture("Stock/thumbnail-icon.png");

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
		ImGui::BeginGroup();

		T& resource = res[indexInRes];
		if (!resource.mThumbnailTextureId)
		{
			resource.mThumbnailTextureId = defaultTextureId;
			g_TS.AddTaskSetToPipe(new DecodeThumbnailTaskSet(&resource.mThumbnail, std::make_pair(indexInRes,resource.mRuntimeUniqueId)));
		}
		ImGui::Image((ImTextureID)(int64_t)(resource.mThumbnailTextureId), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
		bool clicked = ImGui::IsItemClicked();
		ImGui::SameLine();
		ImGui::TreeNodeEx(GetName(resource.mName).c_str(), node_flags);
		clicked |= ImGui::IsItemClicked();
		if (clicked)
		{
			selection = (index << 16) + indexInRes;
			ret = true;
		}
		ImGui::EndGroup();
	}

	if (currentGroup.length() && !currentGroupIsSkipped)
		ImGui::TreePop();

	ImGui::TreePop();
	return ret;
}

inline void GuiString(const char*label, std::string* str, int stringId, bool multi)
{
	ImGui::PushID(stringId);
	char eventStr[2048];
	strcpy(eventStr, str->c_str());
	if (multi)
	{
		if (ImGui::InputTextMultiline(label, eventStr, 2048))
			*str = eventStr;
	}
	else
	{
		if (ImGui::InputText(label, eventStr, 2048))
			*str = eventStr;
	}
	ImGui::PopID();
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

	int metaNodeCount;
	const TileNodeEditGraphDelegate::MetaNode* metaNodes = nodeGraphDelegate.GetMetaNodes(metaNodeCount);

	for (size_t i = 0; i < nodeGraphDelegate.mNodes.size(); i++)
	{
		TileNodeEditGraphDelegate::ImogenNode srcNode = nodeGraphDelegate.mNodes[i];
		MaterialNode &dstNode = material.mMaterialNodes[i];
		dstNode.mRuntimeUniqueId = GetRuntimeId();
		if (metaNodes[srcNode.mType].mbSaveTexture)
		{
			Image image;
			if (Evaluation::GetEvaluationImage(int(i), &image) == EVAL_OK)
			{
				g_TS.AddTaskSetToPipe(new EncodeImageTaskSet(image, std::make_pair(materialIndex, material.mRuntimeUniqueId), std::make_pair(i, dstNode.mRuntimeUniqueId)));
			}
		}

		dstNode.mType = uint32_t(srcNode.mType);
		dstNode.mParameters.resize(srcNode.mParametersSize);
		if (srcNode.mParametersSize)
			memcpy(&dstNode.mParameters[0], srcNode.mParameters, srcNode.mParametersSize);
		dstNode.mInputSamplers = srcNode.mInputSamplers;
		ImVec2 nodePos = NodeGraphGetNodePos(i);
		dstNode.mPosX = int32_t(nodePos.x);
		dstNode.mPosY = int32_t(nodePos.y);
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
}

void LibraryEdit(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
	int previousSelection = selectedMaterial;
	if (ImGui::Button("New Material"))
	{
		library.mMaterials.push_back(Material());
		Material& back = library.mMaterials.back();
		back.mName = "New";
		back.mThumbnailTextureId = 0;
		back.mRuntimeUniqueId = GetRuntimeId();
		
		if (previousSelection != -1)
		{
			ValidateMaterial(library, nodeGraphDelegate, previousSelection);
		}
		selectedMaterial = int(library.mMaterials.size()) - 1;

		nodeGraphDelegate.Clear();
		evaluation.Clear();
		NodeGraphClear();
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
	ImGui::BeginChild("TV", ImVec2(250, -1));
	if (TVRes(library.mMaterials, "Materials", selectedMaterial, 0, evaluation))
	{
		nodeGraphDelegate.mSelectedNodeIndex = -1;
		// save previous
		if (previousSelection != -1)
		{
			ValidateMaterial(library, nodeGraphDelegate, previousSelection);
		}
		// set new
		if (selectedMaterial != -1)
		{
			nodeGraphDelegate.Clear();
			evaluation.Clear();
			NodeGraphClear();

			int metaNodeCount;
			const TileNodeEditGraphDelegate::MetaNode* metaNodes = nodeGraphDelegate.GetMetaNodes(metaNodeCount);

			Material& material = library.mMaterials[selectedMaterial];
			for (size_t i = 0; i < material.mMaterialNodes.size(); i++)
			{
				MaterialNode& node = material.mMaterialNodes[i];
				NodeGraphAddNode(&nodeGraphDelegate, node.mType, node.mParameters.data(), node.mPosX, node.mPosY);
				if (!node.mImage.empty())
				{
					TileNodeEditGraphDelegate::ImogenNode& lastNode = nodeGraphDelegate.mNodes.back();
					lastNode.mbProcessing = true;
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
		}
	}
	ImGui::EndChild();
}

void Imogen::Show(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(io.DisplaySize);
	if (ImGui::Begin("Imogen", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
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
				GuiString("Name", &material.mName, 100, false);
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
				if (ImGui::Button("Delete Material"))
				{
					library.mMaterials.erase(library.mMaterials.begin() + selectedMaterial);
					selectedMaterial = int(library.mMaterials.size()) - 1;
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
			NodeEdit(nodeGraphDelegate, evaluation);
		}
		ImGui::End();

		if (ImGui::Begin("Logs"))
		{
			ImguiAppLog::Log->DrawEmbedded();
		}
		ImGui::End();

		ImGui::End();
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

	DiscoverNodes("glsl", "GLSL/", EVALUATOR_GLSL, mEvaluatorFiles);
	DiscoverNodes("c", "C/", EVALUATOR_C, mEvaluatorFiles);
}

void Imogen::Finish()
{

}