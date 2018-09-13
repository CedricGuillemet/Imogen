#include "imgui.h"
#include "imgui_internal.h"

#define IMAPP_IMPL
#include "ImApp.h"

#include <math.h>
#include <vector>
#include <fstream>
#include <streambuf>

#include "Nodes.h"
#include "NodesDelegate.h"
#include "Evaluation.h"
#include "TextEditor.h"

std::vector<std::string> shaderFileNames;

void DiscoverShaders()
{
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;

	if ((hFind = FindFirstFile("GLSL/*.glsl", &FindFileData)) != INVALID_HANDLE_VALUE) 
	{
		do 
		{
			//printf("%s\n", FindFileData.cFileName);
			shaderFileNames.push_back(std::string(FindFileData.cFileName));
		} 
		while (FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
	}
}
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

void DebugLogText(const char *szText)
{
	static ImguiAppLog imguiLog;
	imguiLog.AddLog(szText);
}
int Log(const char *szFormat, ...)
{
	va_list ptr_arg;
	va_start(ptr_arg, szFormat);

	char buf[1024];
	vsprintf(buf, szFormat, ptr_arg);

	static FILE *fp = fopen("log.txt", "wt");
	if (fp)
	{
		fprintf(fp, buf);
		fflush(fp);
	}
	DebugLogText(buf);
	va_end(ptr_arg);
	return 0;
}

bool PushTabButton(const char *szText, bool selected)
{
	int i = selected ? 0 : 1;
	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(i / 7.0f, 0.8f, 0.8f));
	bool res = ImGui::Button(szText);
	ImGui::PopStyleColor(3);
	return res;
}

void HandleEditor(TextEditor &editor, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
	static int currentShaderIndex = -1;
	if (currentShaderIndex == -1)
	{
		currentShaderIndex = 0;
		editor.SetText(evaluation.GetEvaluationGLSL(shaderFileNames[currentShaderIndex]));
	}
	auto cpos = editor.GetCursorPosition();
	ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

	// save
	if (ImGui::IsKeyReleased(VK_F5))
	{
		auto textToSave = editor.GetText();

		std::ofstream t("GLSL/"+shaderFileNames[currentShaderIndex], std::ofstream::out);
		t << textToSave;
		t.close();

		evaluation.SetEvaluationGLSL(shaderFileNames);
		nodeGraphDelegate.InvalidateParameters();
	}
	
	ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
		editor.IsOverwrite() ? "Ovr" : "Ins",
		editor.CanUndo() ? "*" : " ",
		editor.GetLanguageDefinition().mName.c_str());

	for (size_t i = 0; i < shaderFileNames.size(); i++)
	{
		if (i)
			ImGui::SameLine();

		if (PushTabButton(shaderFileNames[i].c_str(), currentShaderIndex == int(i)) && currentShaderIndex != int(i))
		{
			currentShaderIndex = int(i);
			editor.SetText(evaluation.GetEvaluationGLSL(shaderFileNames[currentShaderIndex]));
		}
	}
	ImGui::SameLine();
	ImGui::Text("F5 to save and update nodes");

	editor.Render("TextEditor");
}

int main(int, char**)
{
	ImApp::ImApp imApp;

	ImApp::Config config;
	config.mWidth = 1280;
	config.mHeight = 720;
	imApp.Init(config);
	ImguiAppLog logger;

	DiscoverShaders();

	
	Evaluation evaluation;
	
	TextEditor editor;
	editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
	evaluation.SetEvaluationGLSL(shaderFileNames);
	evaluation.LoadEquiRect("studio017PoT.png");

	TileNodeEditGraphDelegate nodeGraphDelegate(evaluation);

	static const char* MaterialFilename = "Materials.txt";
	Log("Loading nodes...\n");
	LoadNodes(MaterialFilename, &nodeGraphDelegate);
	Log("\nDone!\n");

	// Main loop
	while (!imApp.Done())
	{
		
		imApp.NewFrame();
		
		ImGuiIO& io = ImGui::GetIO();
		
		//ImGui::SetNextWindowSize(ImVec2(1100, 900), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Imogen"))
		{
			ImGui::BeginChild("ImogenEdit", ImVec2(256, 0));

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

			ImGui::EndChild();


			ImGui::SameLine();


			ImGui::BeginGroup();
			static int editorMode = 0;
			if (PushTabButton("Nodes", editorMode == 0))
				editorMode = 0;
			ImGui::SameLine();
			if (PushTabButton("Shaders", editorMode == 1))
				editorMode = 1;
			ImGui::SameLine();
			if (PushTabButton("Logs", editorMode == 2))
				editorMode = 2;

			switch (editorMode)
			{
			case 0:
				NodeGraph(&nodeGraphDelegate);
				break;
			case 1:
				HandleEditor(editor, nodeGraphDelegate, evaluation);
				break;
			case 2:
				ImguiAppLog::Log->DrawEmbedded();
				break;
			}
			ImGui::EndGroup();
			
			
			ImGui::End();
		}
		evaluation.RunEvaluation();

		// render everything
		glClearColor(0.45f, 0.4f, 0.4f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();

		imApp.EndFrame();
	}
	SaveNodes(MaterialFilename, &nodeGraphDelegate);
	imApp.Finish();

	return 0;
}