// dear imgui: standalone example application for Emscripten, using SDL2 + OpenGL3
// This is mostly the same code as the SDL2 + OpenGL3 example, simply with the modifications needed to run on Emscripten.
// It is possible to combine both code into a single source file that will compile properly on Desktop and using Emscripten.
// See https://github.com/ocornut/imgui/pull/2492 as an example on how to do just that.
//
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (Emscripten is a C++-to-javascript compiler, used to publish executables for the web. See https://emscripten.org/)

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>

#include "Platform.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "NodeGraph.h"
#include "NodeGraphControler.h"
#include "EvaluationStages.h"
#include "Imogen.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "Evaluators.h"
#include "Loader.h"
#include "UI.h"
#include "imMouseState.h"

// Emscripten requires to have full control over the main loop. We're going to store our SDL book-keeping variables globally.
// Having a single function that acts as a loop prevents us to store state in the stack of said function. So we need some location for this.

struct LoopData
{
    Imogen*                 mImogen             = nullptr;
    NodeGraphControler*     mNodeGraphControler = nullptr;
    Builder*                mBuilder            = nullptr;
    SDL_Window*             mWindow             = nullptr;
    SDL_GLContext           mGLContext          = nullptr;
};

Builder*                gBuilder = nullptr;
bool done = false;
// For clarity, our main loop code is declared at the end.
void main_loop(void*);

Library library;
UndoRedoHandler gUndoRedoHandler;

TaskScheduler g_TS;

#if USE_GLDEBUG
void APIENTRY openglCallbackFunction(GLenum /*source*/,
                                     GLenum type,
                                     GLuint id,
                                     GLenum severity,
                                     GLsizei /*length*/,
                                     const GLchar* message,
                                     const void* /*userParam*/)
{
    const char* typeStr = "";
    const char* severityStr = "";

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
#endif

#ifdef __EMSCRIPTEN__
void ImWebConsoleOutput(const char* szText)
{
    printf(szText);
}
   
EM_JS(void, HideLoader, (), {
    document.getElementById("loader").style.display = "none";
});
#else
SDL_Window* glThreadWindow;
SDL_GLContext glThreadContext;

void MakeThreadContext()
{
    SDL_GL_MakeCurrent(glThreadWindow, glThreadContext);
}
#endif

std::function<void(bool capturing)> renderImogenFrame;

void RenderImogenFrame()
{
    renderImogenFrame(true);
}

int main(int, char**)
{
#ifdef WIN32
    // locale for sscanf
    setlocale(LC_ALL, "C");
#endif

#ifdef __EMSCRIPTEN__
    AddLogOutput(ImWebConsoleOutput);
#endif

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    LoopData loopdata;
    // For the browser using Emscripten, we are going to use WebGL1 with GL ES2. See the Makefile. for requirement details.
    // It is very likely the generated file won't work in many browsers. 
    
#ifdef __EMSCRIPTEN__
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif __APPLE__
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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    loopdata.mWindow = SDL_CreateWindow("Imogen 0.13 Web Edition", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
#ifndef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    glThreadContext = SDL_GL_CreateContext(loopdata.mWindow);
    glThreadWindow = loopdata.mWindow;
#endif
    loopdata.mGLContext = SDL_GL_CreateContext(loopdata.mWindow);
    if (!loopdata.mGLContext)
    {
        fprintf(stderr, "Failed to initialize GL context!\n");
        return 1;
    }
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#ifndef __EMSCRIPTEN__
#if GL_VERSION_3_2
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
#endif
#endif // __EMSCRIPTEN__

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = "imgui.ini";

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(loopdata.mWindow, loopdata.mGLContext);
    ImGui_ImplOpenGL3_Init(glsl_version);

#if USE_GLDEBUG
    // opengl debug
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback((GLDEBUGPROCARB)openglCallbackFunction, NULL);
    GLuint unusedIds = 0;
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);
#endif

    g_TS.Initialize();

#if USE_PYTHON    
    Evaluators::InitPython();
    TagTime("Python interpreter Init");
#endif

    LoadMetaNodes();
    
#if USE_FFMPEG
    FFMPEGCodec::RegisterAll();
    FFMPEGCodec::Log = Log;
    TagTime("FFMPEG Init");
#endif
    stbi_set_flip_vertically_on_load(1);
    stbi_flip_vertically_on_write(1);
    
    ImGui::StyleColorsDark();
    static const char* libraryFilename = "library.dat";
    LoadLib(&library, libraryFilename);

    NodeGraphControler nodeGraphControler;
    Imogen imogen(&nodeGraphControler);

    Builder builder;
    imogen.Init();
    gDefaultShader.Init();

    gEvaluators.SetEvaluators(imogen.mEvaluatorFiles);

    loopdata.mImogen = &imogen;
    loopdata.mNodeGraphControler = &nodeGraphControler;
    gBuilder = loopdata.mBuilder = &builder;
    InitFonts();
    imogen.SetExistingMaterialActive(".default");

#ifdef __EMSCRIPTEN__
    HideLoader();
    // This function call won't return, and will engage in an infinite loop, processing events from the browser, and dispatching them.
    emscripten_set_main_loop_arg(main_loop, &loopdata, 0, true);
#else   
    while (!done)
    {
        main_loop(&loopdata);
    }
    imogen.ValidateCurrentMaterial(library);

    g_TS.WaitforAllAndShutdown();

    // save lib after all TS thread done in case a job adds something to the library (ie, thumbnail, paint 2D/3D)
    SaveLib(&library, libraryFilename);

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    imogen.Finish(); // keep dock being saved

    SDL_GL_DeleteContext(loopdata.mGLContext);
    SDL_DestroyWindow(loopdata.mWindow);
    SDL_Quit();
#if USE_PYTHON
    pybind11::finalize_interpreter();
#endif

#endif
    return 0;
}

void main_loop(void* arg)
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
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
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
        SDL_GL_MakeCurrent(loopdata->mWindow, loopdata->mGLContext);
        // render everything
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0., 0., 0., 0.);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        g_TS.RunPinnedTasks();
    };

    renderImogenFrame(false);
    SDL_GL_SwapWindow(loopdata->mWindow);
#ifndef __EMSCRIPTEN__
    loopdata->mImogen->RunDeferedCommands();
#endif
}
