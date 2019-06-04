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

Library library;

#if USE_ENKITS
enki::TaskScheduler g_TS;
#endif

UndoRedoHandler gUndoRedoHandler;
Builder* builder;
SDL_Window* window;
SDL_GLContext glThreadContext;

void MakeThreadContext()
{
    SDL_GL_MakeCurrent(window, glThreadContext);
}

std::function<void(bool capturing)> renderImogenFrame;

void RenderImogenFrame()
{
    renderImogenFrame(true);
}

bool done = false;
    NodeGraphControler nodeGraphControler;
    Imogen imogen(&nodeGraphControler);

void LoopFunction(void *arg)
{
    SDL_GLContext *gl_context = (SDL_GLContext*)arg;
        ImGuiIO& io = ImGui::GetIO();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            done = true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(window))
            done = true;
    }

    renderImogenFrame = [&](bool capturing) {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();
        InitCallbackRects();
        imogen.HandleHotKeys();


        nodeGraphControler.mEditingContext.RunDirty();
        imogen.Show(builder, library, capturing);
        if (!capturing && imogen.ShowMouseState())
        {
            ImMouseState();
        }
        // render everything
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);

        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0., 0., 0., 0.);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

    ImGui::Begin("Another Window");         // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::Text("Hello from another window!");
    ImGui::Button("Close Me");
    ImGui::End();


        ImGui::Render();
        SDL_GL_MakeCurrent(window, *gl_context);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        #if USE_ENKITS
        g_TS.RunPinnedTasks();
        #endif
    };

    renderImogenFrame(false);
    SDL_GL_SwapWindow(window);
    imogen.RunDeferedCommands();
}

int main(int, char**)
{
    


    TagTime("App start");
    // log
    //GLSLPathTracer::Log = Log;
    AddLogOutput(ImConsoleOutput);
#if USE_ENKITS
    g_TS.Initialize();
#endif

    TagTime("Enki TS Init");
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

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }
    TagTime("SDL Init");

    // Decide GL+GLSL versions
#ifdef emscripten_set_main_loop_arg
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif
#endif
    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    window = SDL_CreateWindow("",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              1280,
                              720,
                              SDL_WINDOW_OPENGL | /*SDL_WINDOW_BORDERLESS |*/ SDL_WINDOW_RESIZABLE |
                                  SDL_WINDOW_UTILITY | SDL_WINDOW_MAXIMIZED /*| SDL_WINDOW_BORDERLESS/* */);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    glThreadContext = SDL_GL_CreateContext(window);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
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
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();


    InitFonts();

    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    TagTime("Imgui Init");

    
#if USE_GLDEBUG
    // opengl debug
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback((GLDEBUGPROCARB)openglCallbackFunction, NULL);
    GLuint unusedIds = 0;
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);
#endif

    // Setup style
    ImGui::StyleColorsDark();

    static const char* libraryFilename = "library.dat";

    LoadLib(&library, libraryFilename);
    TagTime("Library loaded");

    imogen.Init();
    gDefaultShader.Init();
    TagTime("Imogen Init");

    TagTime("Evaluation Init");
    gEvaluators.SetEvaluators(imogen.mEvaluatorFiles);

#ifdef emscripten_set_main_loop_arg
    emscripten_set_main_loop_arg(LoopFunction, &gl_context, 0, true);
#else
    builder = new Builder;

    // default Material
    imogen.SetExistingMaterialActive(".default");

    TagTime("App init done");

    // Main loop
    
    while (!done)
    {
        LoopFunction(nullptr);
    }
    
    delete builder;

    imogen.ValidateCurrentMaterial(library);

#if USE_ENKITS
    g_TS.WaitforAllAndShutdown();
#endif
    // save lib after all TS thread done in case a job adds something to the library (ie, thumbnail, paint 2D/3D)
    SaveLib(&library, libraryFilename);

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    imogen.Finish(); // keep dock being saved

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
#if USE_PYTHON
    pybind11::finalize_interpreter();
#endif
#endif // 
    return 0;
}