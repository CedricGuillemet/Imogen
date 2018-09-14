#include "imgui.h"
#include "imgui_internal.h"
#define IMAPP_IMPL
#include "ImApp.h"
#include <math.h>
#include <vector>
#include "Nodes.h"
#include "NodesDelegate.h"
#include "Evaluation.h"
#include "Imogen.h"

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

int main(int, char**)
{
	ImApp::ImApp imApp;

	ImApp::Config config;
	config.mWidth = 1280;
	config.mHeight = 720;
	imApp.Init(config);
	
	Evaluation evaluation;
	Imogen imogen;
	
	evaluation.SetEvaluationGLSL(imogen.shaderFileNames);
	evaluation.LoadEquiRect("studio017PoT.png");

	TileNodeEditGraphDelegate nodeGraphDelegate(evaluation);

	//
	static const char* MaterialFilename = "Materials.txt";
	LoadNodes(MaterialFilename, &nodeGraphDelegate);

	// Main loop
	while (!imApp.Done())
	{
		imApp.NewFrame();

		imogen.Show(nodeGraphDelegate, evaluation);

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