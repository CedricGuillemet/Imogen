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
#include <stdio.h>
#include <stdlib.h>
#include <io.h> 
#include <fcntl.h> 
#include "NodeGraph.h"
#include "NodeGraphControler.h"
#include "EvaluationStages.h"
#include "Imogen.h"
#include "TaskScheduler.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "ffmpegCodec.h"
#include "Evaluators.h"
#include "cmft/clcontext.h"
#include "cmft/clcontext_internal.h"
#include "Loader.h"
#include "UI.h"

unsigned int gCPUCount = 1;
cmft::ClContext* clContext = NULL;
bool gbIsPlaying = false;
bool gPlayLoop = false;
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

Library library;
Imogen imogen;
enki::TaskScheduler g_TS;
UndoRedoHandler gUndoRedoHandler;

SDL_Window* window;
SDL_GLContext glThreadContext;

void MakeThreadContext()
{
    SDL_GL_MakeCurrent(window, glThreadContext);
}

int main(int, char**)
{
    TagTime("App start");
    // log
    GLSLPathTracer::Log = Log;
    AddLogOutput(ImConsoleOutput);
    g_TS.Initialize();
    TagTime("Enki TS Init");
    pybind11::initialize_interpreter(true); // start the interpreter and keep it alive
    gEvaluators.InitPythonModules();
    pybind11::exec( R"(
        import sys
        import Imogen
        class CatchImogenIO:
            def __init__(self):
                pass
            def write(self, txt):
                Imogen.Log(txt)
        catchImogenIO = CatchImogenIO()
        sys.stdout = catchImogenIO
        sys.stderr = catchImogenIO
        print("Python stdout, stderr catched.\n"))");
    pybind11::module::import("Plugins");

    TagTime("Python interpreter Init");
    LoadMetaNodes();
    FFMPEGCodec::RegisterAll();
    FFMPEGCodec::Log = Log;
    TagTime("FFMPEG Init");

    stbi_set_flip_vertically_on_load(1);
    stbi_flip_vertically_on_write(1);

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }
    TagTime("SDL Init");

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
    window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
        SDL_WINDOW_OPENGL | /*SDL_WINDOW_BORDERLESS |*/ SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY | SDL_WINDOW_MAXIMIZED/*| SDL_WINDOW_BORDERLESS/* */);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    glThreadContext = SDL_GL_CreateContext(window);
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
    TagTime("OpenCL Init");

    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    InitFonts();
    
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    TagTime("Imgui Init");

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
    TagTime("Library loaded");

    imogen.Init();
    TagTime("Imogen Init");

    TagTime("Evaluation Init");
    gEvaluators.SetEvaluators(imogen.mEvaluatorFiles);

    gCPUCount = SDL_GetCPUCount();

    Builder *builder = new Builder;

    // default Material
    SetExistingMaterialActive(".default");

    TagTime("App init done");

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
        // undo/redo
        if (io.KeyCtrl && ImGui::IsKeyPressedMap(ImGuiKey_Z))
            gUndoRedoHandler.Undo();
        if ((io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressedMap(ImGuiKey_Z)) ||
            (io.KeyCtrl && ImGui::IsKeyPressedMap(ImGuiKey_Y)) )
            gUndoRedoHandler.Redo();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();
        InitCallbackRects();

        if (gbIsPlaying)
        {
            gEvaluationTime++;
            if (gEvaluationTime >= gNodeDelegate.mEvaluationStages.mFrameMax)
            {
                if (gPlayLoop)
                {
                    gEvaluationTime = gNodeDelegate.mEvaluationStages.mFrameMin;
                }
                else
                {
                    gbIsPlaying = false;
                }
            }
            gNodeDelegate.SetTime(gEvaluationTime, true);
            gNodeDelegate.ApplyAnimation(gEvaluationTime);
        }
        gCurrentContext->RunDirty();
        imogen.Show(builder, library, gNodeDelegate);

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
    delete builder;

    clDestroy(clContext);
    // Unload opencl lib.
    if (clLoaded)
    {
        cmft::clUnload();
    }

    imogen.ValidateCurrentMaterial(library, gNodeDelegate);
    SaveLib(&library, libraryFilename);

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    imogen.Finish(); // keep dock being saved

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    pybind11::finalize_interpreter();
    g_TS.WaitforAllAndShutdown();
    return 0;
}