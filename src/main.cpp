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

void HandleEditor(TextEditor &editor, TileNodeEditGraphDelegate &nodeGraphDelegate)
{
	auto cpos = editor.GetCursorPosition();
	ImGui::Begin("Text Editor Demo", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
	ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Save", "Ctrl-S", nullptr, true))
			{
				auto textToSave = editor.GetText();
				UpdateEvaluationShader(textToSave);
				nodeGraphDelegate.UpdateAllFunctionCalls();
				std::ofstream t("Shader.glsl", std::ofstream::out);
				t << textToSave;
				t.close();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			bool ro = editor.IsReadOnly();
			if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
				editor.SetReadOnly(ro);
			ImGui::Separator();

			if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
				editor.Undo();
			if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
				editor.Redo();

			ImGui::Separator();

			if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
				editor.Copy();
			if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
				editor.Cut();
			if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
				editor.Delete();
			if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
				editor.Paste();

			ImGui::Separator();

			if (ImGui::MenuItem("Select all", nullptr, nullptr))
				editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Dark palette"))
				editor.SetPalette(TextEditor::GetDarkPalette());
			if (ImGui::MenuItem("Light palette"))
				editor.SetPalette(TextEditor::GetLightPalette());
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
		editor.IsOverwrite() ? "Ovr" : "Ins",
		editor.CanUndo() ? "*" : " ",
		editor.GetLanguageDefinition().mName.c_str());

	editor.Render("TextEditor");
	ImGui::End();
}

int main(int, char**)
{
	ImApp::ImApp imApp;

	ImApp::Config config;
	config.mWidth = 1280;
	config.mHeight = 720;
	//config.mFullscreen = true;
	imApp.Init(config);
	ImguiAppLog logger;

	
	
	TextEditor editor;
	editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
	static const char* fileToEdit = "Shader.glsl";
	std::ifstream t(fileToEdit);
	if (t.good())
	{
		std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
		editor.SetText(str);
		InitEvaluation(str);
	}
	
	LoadEquiRectHDREnvLight("studio022.hdr");

	TileNodeEditGraphDelegate nodeGraphDelegate;

	static const char* MaterialFilename = "Materials.txt";
	Log("Loading nodes...\n");
	LoadNodes(MaterialFilename, &nodeGraphDelegate);
	Log("\nDone!\n");

	// Main loop
	while (!imApp.Done())
	{
		
		imApp.NewFrame();
		
		ImGuiIO& io = ImGui::GetIO();
		
		ImGui::SetNextWindowSize(ImVec2(1100, 900), ImGuiSetCond_FirstUseEver);
		if (ImGui::Begin("Imogen"))
		{
			if (ImGui::Button("bake"))
			{
				Bake("bakedTexture.png", nodeGraphDelegate.mBakeTargetIndex, 4096, 4096);
			}
			if (ImGui::CollapsingHeader("Logger", 0))
			{
				ImGui::BeginGroup();
				ImguiAppLog::Log->DrawEmbedded();
				ImGui::EndGroup();
			}
			ImGui::BeginChild("ImogenEdit", ImVec2(256, 0));

			int selNode = nodeGraphDelegate.mSelectedNodeIndex;
			if (ImGui::CollapsingHeader("Preview", 0, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
				ImGui::ImageButton((ImTextureID)((selNode != -1) ? GetEvaluationTexture(selNode) : 0), ImVec2(256, 256));
				ImGui::PopStyleVar(1);
				ImRect rc(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
				if (rc.Contains(io.MousePos))
				{
					if (io.MouseDown[0])
					{
						ImVec2 ratio((io.MousePos.x - rc.Min.x) / rc.GetSize().x, (io.MousePos.y - rc.Min.y) / rc.GetSize().y);
						nodeGraphDelegate.SetMouseRatios(ratio.x, ratio.y);
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
			NodeGraph(&nodeGraphDelegate);
			ImGui::EndGroup();
			
			
			ImGui::End();
		}

		HandleEditor(editor, nodeGraphDelegate);
		RunEvaluation();

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