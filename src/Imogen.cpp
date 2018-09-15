#include "imgui.h"
#include "Imogen.h"
#include "TextEditor.h"
#include "imgui_dock.h"
#include <fstream>
#include <streambuf>
#include "Evaluation.h"
#include "NodesDelegate.h"
#include "ImApp.h"

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

void Imogen::Show(TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
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
			NodeGraph(&nodeGraphDelegate);
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