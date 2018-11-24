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
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <GL/gl3w.h>
#include "Nodes.h"
#include "NodesDelegate.h"
#include "Evaluation.h"
#include "Imogen.h"
#include "TaskScheduler.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "ffmpegCodec.h"
#include "Evaluators.h"
#include "cmft/clcontext.h"
#include "cmft/clcontext_internal.h"
#include "pybind11/embed.h"

TileNodeEditGraphDelegate *TileNodeEditGraphDelegate::mInstance = NULL;
unsigned int gCPUCount = 1;
cmft::ClContext* clContext = NULL;

void APIENTRY openglCallbackFunction(GLenum /*source*/,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei /*length*/,
	const GLchar* message,
	const void* /*userParam*/)
{
	const char *typeStr = "";
	const char *severityStr = "";

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		typeStr = "ERROR";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		typeStr = "DEPRECATED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		typeStr = "UNDEFINED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		typeStr = "PORTABILITY";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		typeStr = "PERFORMANCE";
		break;
	case GL_DEBUG_TYPE_OTHER:
		typeStr = "OTHER";
		// skip
		return;
		break;
	}

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_LOW:
		severityStr = "LOW";
		return;
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		severityStr = "MEDIUM";
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		severityStr = "HIGH";
		break;
	}
	Log("GL Debug (%s - %s) %s \n", typeStr, severityStr, message);
}

Evaluation gEvaluation;
Library library;
Imogen imogen;
enki::TaskScheduler g_TS;


struct pImage
{
	pImage() : a(13), b(37) {}
	int a, b;
};

PYBIND11_MAKE_OPAQUE(pImage);
int add2(int i, int j)
{
	return i + j;
}
PYBIND11_EMBEDDED_MODULE(imo, m) {
	// `m` is a `py::module` which is used to bind functions and classes
	m.def("add", [](int i, int j) {
		return i + j;
	});
	m.def("add2", add2);

	m.def("accessor_api", []() {
		auto d = pybind11::dict();

		d["target"] = 10;

		auto l = pybind11::list();
		//for (const auto &item : o.attr("begin_end")) {
		l.append(5);
		l.append(-1);
		l.append(-1);
		//}
		d["inputs"] = l;

		return d;
	});

	pybind11::class_<Image>(m, "Image");

	m.def("GetImage", []() {
		//auto i = new pImage;
		pImage i;
		i.a = 14;
		printf("new img %p \n", &i);
		return i;
	});

	m.def("SaveImage", [](pImage image) {
		//printf("Saving image %d\n", image.a);
		//printf("save img %p \n", image);
	});
}

int main(int, char**)
{
	g_TS.Initialize();
	pybind11::scoped_interpreter guard{}; // start the interpreter and keep it alive
	LoadMetaNodes();
	FFMPEGCodec::RegisterAll();
	FFMPEGCodec::Log = Log;

	stbi_set_flip_vertically_on_load(1);
	stbi_flip_vertically_on_write(1);
	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	try
	{
		pybind11::module py_module = pybind11::module::import("Nodes.Python.testnode");
		pybind11::module imo = pybind11::module::import("imo");
		//auto result = py_module.attr("test")(5, 6, imo.attr("accessor_api")()).cast<int>();
		py_module.attr("test")(5, 6, imo.attr("accessor_api")());
	}
	catch (...)
	{

	}


	// Decide GL+GLSL versions
#if __APPLE__
	// GL 3.2 Core + GLSL 150
	const char* glsl_version = "#version 150";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);
	SDL_Window* window = SDL_CreateWindow("Imogen 0.6.0", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
	bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
	bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
	bool err = gladLoadGL() == 0;
#endif
	if (err)
	{
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 1;
	}

	// open cl context
	int32_t clLoaded = 0;
	clLoaded = cmft::clLoad();
	if (clLoaded)
	{
		uint32_t clVendor = (uint32_t)CMFT_CL_VENDOR_ANY_GPU;
		uint32_t deviceType = CMFT_CL_DEVICE_TYPE_GPU;
		uint32_t deviceIndex = 0;
		clContext = cmft::clInit(clVendor, deviceType, deviceIndex);
	}
	if (clLoaded && clContext)
	{
		Log("OpenCL context created with %s / %s\n", clContext->m_deviceVendor, clContext->m_deviceName);
	}
	else
	{
		Log("OpenCL context not created.\n");
	}

	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// opengl debug
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback((GLDEBUGPROCARB)openglCallbackFunction, NULL);
	GLuint unusedIds = 0;
	glDebugMessageControl(GL_DONT_CARE,
		GL_DONT_CARE,
		GL_DONT_CARE,
		0,
		&unusedIds,
		true);

	gFSQuad.Init();

	// Setup style
	ImGui::StyleColorsDark();

	static const char* libraryFilename = "library.dat";
	
	LoadLib(&library, libraryFilename);
	
	imogen.Init();
	
	gEvaluation.Init();
	gEvaluators.SetEvaluators(imogen.mEvaluatorFiles);

	TileNodeEditGraphDelegate nodeGraphDelegate(gEvaluation);

	gCPUCount = SDL_GetCPUCount();

	// Main loop
	bool done = false;
	while (!done)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();
		InitCallbackRects();


		gCurrentContext->RunDirty();
		imogen.Show(library, nodeGraphDelegate, gEvaluation);

		

		// render everything
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(0., 0., 0., 0.);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui::Render();
		SDL_GL_MakeCurrent(window, gl_context);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		g_TS.RunPinnedTasks();
		SDL_GL_SwapWindow(window);
	}

	clDestroy(clContext);
	// Unload opencl lib.
	if (clLoaded)
	{
		cmft::clUnload();
	}

	imogen.ValidateCurrentMaterial(library, nodeGraphDelegate);
	SaveLib(&library, libraryFilename);
	gEvaluation.Finish();

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	imogen.Finish(); // keep dock being saved

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	g_TS.WaitforAllAndShutdown();
	return 0;
}