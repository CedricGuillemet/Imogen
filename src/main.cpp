#include "imgui.h"
#include "imgui_internal.h"

#define IMAPP_IMPL
#include "ImApp.h"

#include <math.h>
#include <vector>

#include "Nodes.h"
#include "NodesDelegate.h"
#include "Evaluation.h"

int Log(const char *szFormat, ...)
{
	return 0;
}

int main(int, char**)
{
	ImApp::ImApp imApp;

	ImApp::Config config;
	config.mWidth = 1280;
	config.mHeight = 720;
	//config.mFullscreen = true;
	imApp.Init(config);

	InitEvaluation();
	
	TileNodeEditGraphDelegate nodeGraphDelegate;

	static const char* MaterialFilename = "Materials.txt";

	LoadNodes(MaterialFilename, &nodeGraphDelegate);

	// Main loop
	while (!imApp.Done())
	{
		
		imApp.NewFrame();
		
		ImGuiIO& io = ImGui::GetIO();
		
		ImGui::SetNextWindowSize(ImVec2(1900, 1000), ImGuiSetCond_FirstUseEver);
		if (ImGui::Begin("Imogen"))
		{
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