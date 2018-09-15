#include "imgui.h"
#include "Imogen.h"
#include "TextEditor.h"
#include "imgui_dock.h"
#include <fstream>
#include <streambuf>
#include "Evaluation.h"
#include "NodesDelegate.h"
#include "ImApp.h"
#include "Library.h"

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

		if (ScrollToBottom)
			ImGui::SetScrollHere(1.0f);
		ScrollToBottom = false;
		ImGui::PopStyleVar();
		ImGui::EndChild();
	}
};

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
		editor.SetText(evaluation.GetEvaluationGLSL(shaderFileNames[currentShaderIndex]));
	}
	auto cpos = editor.GetCursorPosition();
	ImGui::BeginChild(13, ImVec2(250, 800));
	for (size_t i = 0; i < shaderFileNames.size(); i++)
	{
		bool selected = i == currentShaderIndex;
		if (ImGui::Selectable(shaderFileNames[i].c_str(), &selected))
		{
			currentShaderIndex = int(i);
			editor.SetText(evaluation.GetEvaluationGLSL(shaderFileNames[currentShaderIndex]));
		}
	}
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild(14);
	// save
	if (ImGui::IsKeyReleased(VK_F5))
	{
		auto textToSave = editor.GetText();

		std::ofstream t("GLSL/" + shaderFileNames[currentShaderIndex], std::ofstream::out);
		t << textToSave;
		t.close();

		evaluation.SetEvaluationGLSL(shaderFileNames);
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
		ImGui::ImageButton(ImTextureID((selNode != -1) ? evaluation.GetEvaluationTexture(selNode) : 0), ImVec2(256, 256));
		ImGui::PopStyleVar(1);
		ImRect rc(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
		if (rc.Contains(io.MousePos))
		{
			if (io.MouseDown[0])
			{
				ImVec2 ratio((io.MousePos.x - rc.Min.x) / rc.GetSize().x, (io.MousePos.y - rc.Min.y) / rc.GetSize().y);
				ImVec2 deltaRatio((io.MouseDelta.x) / rc.GetSize().x, (io.MouseDelta.y) / rc.GetSize().y);
				nodeGraphDelegate.SetMouseRatios(ratio.x, ratio.y, deltaRatio.x, deltaRatio.y);
			}
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

template <typename T, typename Ty> bool TVRes(std::vector<T, Ty>& res, const char *szName, int &selection, int index, int *deletedItem = NULL)
{
	bool ret = false;
	if (!ImGui::TreeNodeEx(szName, ImGuiTreeNodeFlags_FramePadding /*| ImGuiTreeNodeFlags_DefaultOpen*/))
		return ret;

	ImGui::SameLine(0.0f, 30);
	if (ImGui::Button("New"))
	{
		res.push_back(T());
		selection = (int(res.size()) - 1) + (index << 16);
		res.back().mName = "New";
		ret = true;
	}
	int duplicate = -1;
	int del = -1;
	std::string currentGroup = "";
	bool currentGroupIsSkipped = false;

	std::vector<SortedResource<T, Ty>> sortedResources;
	SortedResource<T, Ty>::ComputeSortedResources(res, sortedResources);

	for (const auto& sortedRes : sortedResources) //unsigned int i = 0; i < res.size(); i++)
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

		ImGui::TreeNodeEx(GetName(res[indexInRes].mName).c_str(), node_flags);

		if (ImGui::IsItemClicked())
		{
			selection = (index << 16) + indexInRes;
			ret = true;
		}
		if (((int)indexInRes == (selection & 0xFFFF)) && (selection >> 16) == (int)index)
		{
			ImGui::SameLine(-1);
			if (ImGui::Button("X"))
				del = indexInRes;
			ImGui::SameLine(0, 3);
			if (ImGui::Button("Dup"))
				duplicate = indexInRes;
		}
	}

	if (currentGroup.length() && !currentGroupIsSkipped)
		ImGui::TreePop();

	ImGui::TreePop();
	if (del != -1)
	{
		if (deletedItem)
			*deletedItem = del;
		res.erase(res.begin() + del);
		selection = (min(del, (int)res.size() - 1)) + (index << 16);
		ret = true;
	}
	if (duplicate != -1)
	{
		res.push_back(res[duplicate]);
		res.back().mName += "_Copy";
		selection = (int(res.size()) - 1) + (index << 16);
		ret = true;
	}
	return ret;
}

inline void GuiString(const char*label, std::string* str, int stringId)
{
	//static int guiStringId = 47414;
	ImGui::PushID(stringId);
	char eventStr[512];
	strcpy(eventStr, str->c_str());
	if (ImGui::InputText(label, eventStr, 512))
		*str = eventStr;
	ImGui::PopID();
}

static int selectedMaterial = -1;

void LibraryEdit(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
	int deletedItem = -1;
	ImGui::BeginChild("TV", ImVec2(250, -1));
	int previousSelection = selectedMaterial;
	if (TVRes(library.mMaterials, "Materials", selectedMaterial, 0, &deletedItem))
	{
		nodeGraphDelegate.mSelectedNodeIndex = -1;
		// save previous
		if (previousSelection != -1)
		{
			Material& material = library.mMaterials[previousSelection];
			material.mMaterialNodes.resize(nodeGraphDelegate.mNodes.size());
			for (size_t i = 0; i < nodeGraphDelegate.mNodes.size(); i++)
			{
				TileNodeEditGraphDelegate::ImogenNode srcNode = nodeGraphDelegate.mNodes[i];
				MaterialNode &dstNode = material.mMaterialNodes[i];

				dstNode.mType = uint32_t(srcNode.mType);
				dstNode.mParameters.resize(srcNode.mParametersSize);
				if (srcNode.mParametersSize)
					memcpy(&dstNode.mParameters[0], srcNode.mParameters, srcNode.mParametersSize);
				dstNode.mInputSamplers = srcNode.mInputSamplers;
				ImVec2 nodePos = NodeGraphGetNodePos(i);
				dstNode.mPosX = uint32_t(nodePos.x);
				dstNode.mPosY = uint32_t(nodePos.y);
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
		}
		// set new
		if (selectedMaterial != -1)
		{
			nodeGraphDelegate.Clear();
			evaluation.Clear();
			NodeGraphClear();
			Material& material = library.mMaterials[selectedMaterial];
			for (size_t i = 0; i < material.mMaterialNodes.size(); i++)
			{
				MaterialNode& node = material.mMaterialNodes[i];
				NodeGraphAddNode(&nodeGraphDelegate, node.mType, node.mParameters.data(), node.mPosX, node.mPosY);
			}
			for (size_t i = 0; i < material.mMaterialConnections.size(); i++)
			{
				MaterialConnection& materialConnection = material.mMaterialConnections[i];
				NodeGraphAddLink(&nodeGraphDelegate, materialConnection.mInputNode, materialConnection.mInputSlot, materialConnection.mOutputNode, materialConnection.mOutputSlot);
			}
			NodeGraphUpdateEvaluationOrder(&nodeGraphDelegate);
		}
	}
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("Mat");
	if (selectedMaterial != -1)
	{
		Material& material = library.mMaterials[selectedMaterial];
		GuiString("Name", &material.mName, 100);
		/* to add:
		- bake dir
		- bake size
		- preview size
		- load equirect ibl
		*/
	}
	ImGui::EndChild();
}

void Imogen::Show(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(io.DisplaySize);
	if (ImGui::Begin("Imogen", 0, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::BeginDockspace();

		ImGui::SetNextDock("Imogen", ImGuiDockSlot_Tab);
		if (ImGui::BeginDock("Shaders"))
		{
			HandleEditor(editor, nodeGraphDelegate, evaluation);
		}
		ImGui::EndDock();
		if (ImGui::BeginDock("Nodes"))
		{
			NodeGraph(&nodeGraphDelegate, selectedMaterial!= -1);
		}
		ImGui::EndDock();

		ImGui::SetNextDock("Imogen", ImGuiDockSlot_Left);
		if (ImGui::BeginDock("Library"))
		{
			LibraryEdit(library, nodeGraphDelegate, evaluation);
		}
		ImGui::EndDock();

		ImGui::SetWindowSize(ImVec2(300, 300));
		ImGui::SetNextDock("Imogen", ImGuiDockSlot_Left);
		if (ImGui::BeginDock("Parameters"))
		{
			NodeEdit(nodeGraphDelegate, evaluation);
		}
		ImGui::EndDock();

		ImGui::SetNextDock("Imogen", ImGuiDockSlot_Bottom);
		if (ImGui::BeginDock("Logs"))
		{
			ImguiAppLog::Log->DrawEmbedded();
		}
		ImGui::EndDock();


		ImGui::EndDockspace();
		ImGui::End();
	}
}

void Imogen::DiscoverShaders()
{
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;

	if ((hFind = FindFirstFile("GLSL/*.glsl", &FindFileData)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			//printf("%s\n", FindFileData.cFileName);
			shaderFileNames.push_back(std::string(FindFileData.cFileName));
		} while (FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
	}
}

Imogen::Imogen()
{
	ImGui::InitDock();
	editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
	DiscoverShaders();
}

Imogen::~Imogen()
{
	ImGui::ShutdownDock();
}