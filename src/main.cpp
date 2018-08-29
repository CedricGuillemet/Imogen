#include "imgui.h"
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
				ImGui::Image((ImTextureID)((selNode != -1)?GetEvaluationTexture(selNode):0), ImVec2(256, 256));

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

	imApp.Finish();

	return 0;
}