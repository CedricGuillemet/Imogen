// https://github.com/CedricGuillemet/Imogen
//
// The MIT License(MIT)
//
// Copyright(c) 2019 Cedric Guillemet
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

#include "Platform.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_bgfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "GraphEditor.h"
#include "GraphControler.h"
#include "EvaluationStages.h"
#include "Imogen.h"
#include "Evaluators.h"
#include "Loader.h"
#include "UI.h"
#include "imMouseState.h"
#include "UndoRedo.h"
#include "Mem.h"
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bgfx/embedded_shader.h>
#include "Scene.h"

#include "C:\Users\cedri\Dev\test\Color.dx11.h"
#include "C:\Users\cedri\Dev\test\Color.dx9.h"
#include "C:\Users\cedri\Dev\test\Color.esl.h"
#include "C:\Users\cedri\Dev\test\Color.glsl.h"
#include "C:\Users\cedri\Dev\test\Color.mtl.h"
#include "C:\Users\cedri\Dev\test\Color.spv.h"
#include "C:\Users\cedri\Dev\test\NodeVS.dx11.h"
#include "C:\Users\cedri\Dev\test\NodeVS.dx9.h"
#include "C:\Users\cedri\Dev\test\NodeVS.esl.h"
#include "C:\Users\cedri\Dev\test\NodeVS.glsl.h"
#include "C:\Users\cedri\Dev\test\NodeVS.mtl.h"
#include "C:\Users\cedri\Dev\test\NodeVS.spv.h"

/*
static const bgfx::EmbeddedShader s_embeddedShaders[] =
{
	BGFX_EMBEDDED_SHADER(ColorFS),
	BGFX_EMBEDDED_SHADER(NodeVS),

	BGFX_EMBEDDED_SHADER_END()
};
*/
bx::AllocatorI* getDefaultAllocator();
// Emscripten requires to have full control over the main loop. We're going to store our SDL book-keeping variables globally.
// Having a single function that acts as a loop prevents us to store state in the stack of said function. So we need some location for this.

struct LoopData
{
    Imogen*                 mImogen             = nullptr;
    GraphControler*     	mNodeGraphControler = nullptr;
    Builder*                mBuilder            = nullptr;
    SDL_Window*             mWindow             = nullptr;
    SDL_GLContext           mGLContext          = nullptr;
};

Builder*                gBuilder = nullptr;
bool done = false;
// For clarity, our main loop code is declared at the end.
void MainLoop(void*);

Library library;
UndoRedoHandler gUndoRedoHandler;

TaskScheduler g_TS;


#ifdef __EMSCRIPTEN__

#include "JSGlue.h"

#else

SDL_Window* glThreadWindow;
SDL_GLContext glThreadContext;

void MakeThreadContext()
{
    SDL_GL_MakeCurrent(glThreadWindow, glThreadContext);
}

static bool sdlSetWindow(SDL_Window* _window)
{
	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	if (!SDL_GetWindowWMInfo(_window, &wmi)) {
		return false;
	}

	bgfx::PlatformData pd;
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
	pd.ndt = wmi.info.x11.display;
	pd.nwh = (void*)(uintptr_t)wmi.info.x11.window;
#elif BX_PLATFORM_OSX
	pd.ndt = NULL;
	pd.nwh = wmi.info.cocoa.window;
#elif BX_PLATFORM_WINDOWS
	pd.ndt = NULL;
	pd.nwh = wmi.info.win.window;
#elif BX_PLATFORM_STEAMLINK
	pd.ndt = wmi.info.vivante.display;
	pd.nwh = wmi.info.vivante.window;
#endif // BX_PLATFORM_
	pd.context = NULL;
	pd.backBuffer = NULL;
	pd.backBufferDS = NULL;
	bgfx::setPlatformData(pd);

	return true;
}

#endif

std::function<void(bool capturing)> renderImogenFrame;

void RenderImogenFrame()
{
    renderImogenFrame(true);
}

// because of asynchornous local storage DB mount
#ifndef __EMSCRIPTEN__
int main(int argc, char** argv)
#else
int main(int argc, char** argv)
{
    MountJSDirectory();
}
int main_Async(int argc, char** argv)
#endif
{
#ifdef WIN32
    // locale for sscanf
    setlocale(LC_ALL, "C");
    ImGui::SetAllocatorFunctions(imguiMalloc, imguiFree);
#endif

#ifdef __EMSCRIPTEN__
    AddLogOutput(ImWebConsoleOutput);
    
#endif
    AddLogOutput(ImConsoleOutput);

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    LoopData loopdata;

    // For the browser using Emscripten, we are going to use WebGL2 with GL ES3.
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_MAXIMIZED);
    loopdata.mWindow = SDL_CreateWindow(IMOGENCOMPLETETITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
#ifndef __EMSCRIPTEN__
	sdlSetWindow(loopdata.mWindow);
#endif
	bgfx::renderFrame();

#ifndef __EMSCRIPTEN__
	bgfx::Init init;
	init.type = bgfx::RendererType::Count;//:Direct3D9;//:OpenGL;
	bgfx::init(init);
#else
	bgfx::init();
#endif

	// Enable debug text.
	//bgfx::setDebug(BGFX_DEBUG_TEXT /*| BGFX_DEBUG_STATS*/);

	// Set view 0 clear state.
	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

	bgfx::frame();
#ifndef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    glThreadContext = SDL_GL_CreateContext(loopdata.mWindow);
    glThreadWindow = loopdata.mWindow;
#endif
    /*loopdata.mGLContext = SDL_GL_CreateContext(loopdata.mWindow);
    if (!loopdata.mGLContext)
    {
        fprintf(stderr, "Failed to initialize GL context!\n");
        return 1;
    }*/

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
	//bx::AllocatorI* g_allocator = getDefaultAllocator();
	//imguiCreate(16.f, g_allocator);

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "imgui.ini";

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(loopdata.mWindow, loopdata.mGLContext);

    InitFonts();

    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    ImGui_Implbgfx_Init();

    g_TS.Initialize();

#if USE_PYTHON    
    Evaluators::InitPython();
#endif

    LoadMetaNodes();
    
#if USE_FFMPEG
    FFMPEGCodec::RegisterAll();
    FFMPEGCodec::Log = Log;
#endif
    
    ImGui::StyleColorsDark();
    RecentLibraries recentLibraries;

    LoadLib(&library, recentLibraries.GetMostRecentLibraryPath());

    GraphControler nodeGraphControler;
    Imogen imogen(&nodeGraphControler, recentLibraries);

    Builder builder;

    bool bDebugWindow = false;
    for (int i = 1;i<argc;i++)
    {
        if (!strcmp(argv[i], "-debug"))
        {
            bDebugWindow = true;
        }
        if (!strcmp(argv[i], "-runplugin") && i < (argc-1))
        {
            imogen.RunCommandAsync(argv[i+1], true);
        }
    }
    imogen.Init(bDebugWindow);

    gEvaluators.SetEvaluators();

    loopdata.mImogen = &imogen;
    loopdata.mNodeGraphControler = &nodeGraphControler;
    gBuilder = loopdata.mBuilder = &builder;
    imogen.SetExistingMaterialActive(".default");

	std::string infoTitle = IMOGENCOMPLETETITLE;
	switch (bgfx::getCaps()->vendorId)
	{
		case BGFX_PCI_ID_NVIDIA:
			infoTitle += " - NVIDIA";
			break;
		case BGFX_PCI_ID_AMD:
			infoTitle += " - AMD";
			break;
		case BGFX_PCI_ID_INTEL:
			infoTitle += " - Intel";
			break;
		default:
			break;
	}
	switch (bgfx::getCaps()->rendererType)
	{
	case bgfx::RendererType::Direct3D9:    //!< Direct3D 9.0
		infoTitle += " - Direct3D 9";
		break;
	case bgfx::RendererType::Direct3D11:   //!< Direct3D 11.0
		infoTitle += " - Direct3D 11";
		break;
	case bgfx::RendererType::Direct3D12:   //!< Direct3D 12.0
		infoTitle += " - Direct3D 12";
		break;
	case bgfx::RendererType::Gnm:          //!< GNM
		infoTitle += " - GNM";
		break;
	case bgfx::RendererType::Metal:        //!< Metal
		infoTitle += " - Metal";
		break;
	case bgfx::RendererType::Nvn:          //!< NVN
		infoTitle += " - NVM";
		break;
	case bgfx::RendererType::OpenGLES:     //!< OpenGL ES 2.0+
		infoTitle += " - OpenGL ES";
		break;
	case bgfx::RendererType::OpenGL:       //!< OpenGL 2.1+
		infoTitle += " - OpenGL";
		break;
	case bgfx::RendererType::Vulkan:       //!< Vulkan
		infoTitle += " - Vulkan";
		break;
	}
	SDL_SetWindowTitle(loopdata.mWindow, infoTitle.c_str());

#ifdef __EMSCRIPTEN__
    HideLoader();
    // This function call won't return, and will engage in an infinite loop, processing events from the browser, and dispatching them.
    emscripten_set_main_loop_arg(MainLoop, &loopdata, 0, true);
#else   
    while (!done)
    {
        MainLoop(&loopdata);
    }
    imogen.ValidateCurrentMaterial(library);
    g_TS.WaitforAllAndShutdown();

    // save lib after all TS thread done in case a job adds something to the library (ie, thumbnail, paint 2D/3D)
    SaveLib(&library, library.mFilename);

    // Cleanup
    ImGui_Implbgfx_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    imogen.Finish(); // keep dock being saved

	// evaluators
	gEvaluators.Clear();

	// Shutdown bgfx.
	bgfx::shutdown();

    SDL_GL_DeleteContext(loopdata.mGLContext);
    SDL_DestroyWindow(loopdata.mWindow);
    SDL_Quit();
#if USE_PYTHON
    pybind11::finalize_interpreter();
#endif

#endif
    return 0;
}

void MainLoop(void* arg)
{
    LoopData *loopdata = (LoopData*)arg;
    ImGuiIO& io = ImGui::GetIO();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
        {
            done = true;
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(loopdata->mWindow))
        {
            done = true;
        }
    }
	
    renderImogenFrame = [&](bool capturing) {
		int width;
		int height;
		SDL_GetWindowSize(loopdata->mWindow, &width, &height);
		auto stats = bgfx::getStats();
		if (stats->width != width || stats->height != height)
		{
			bgfx::reset(width, height, BGFX_RESET_VSYNC | BGFX_RESET_FLIP_AFTER_RENDER);
		}
		io.DisplaySize = ImVec2(width, height);
        
		ImGui_Implbgfx_NewFrame();
        ImGui_ImplSDL2_NewFrame(loopdata->mWindow);
        ImGui::NewFrame();

        InitCallbackRects();
        loopdata->mImogen->HandleHotKeys();

        loopdata->mNodeGraphControler->mEditingContext.RunDirty();
        loopdata->mImogen->Show(loopdata->mBuilder, library, capturing);
        if (!capturing && loopdata->mImogen->ShowMouseState())
        {
            ImMouseState();
        }

        // Rendering
        ImGui::Render();
		bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));
		bgfx::touch(0);

		ImGui_Implbgfx_RenderDrawData(ImGui::GetDrawData());

		bgfx::frame();
		g_TS.RunPinnedTasks();
    };
	
    renderImogenFrame(false);
	
#ifndef __EMSCRIPTEN__
    loopdata->mImogen->RunDeferedCommands();
#endif
}
