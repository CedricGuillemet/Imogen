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
		
		ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiSetCond_FirstUseEver);
		if (ImGui::Begin("Imogen"))
		{
			
			NodeGraph(&nodeGraphDelegate);
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