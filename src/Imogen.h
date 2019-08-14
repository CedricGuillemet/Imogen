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
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include "imgui.h"
#include "imgui_internal.h"
#include "Library.h"

struct GraphControler;
struct Evaluation;
struct Library;
struct Builder;
struct MySequence;

enum EVALUATOR_TYPE
{
    EVALUATOR_GLSL,
    EVALUATOR_C,
    EVALUATOR_PYTHON,
    EVALUATOR_GLSLCOMPUTE,
    EVALUATOR_JS,
};

struct EvaluatorFile
{
    std::string mDirectory;
    std::string mFilename;
    EVALUATOR_TYPE mEvaluatorType;
};

// plugins
struct RegisteredPlugin
{
    std::string mName;
    std::string mPythonCommand;
};
extern std::vector<RegisteredPlugin> mRegisteredPlugins;

struct Imogen
{
    Imogen(GraphControler* nodeGraphControler, RecentLibraries& recentLibraries);
    ~Imogen();

    void Init(bool bDebugWindow);
    void Finish();

    void Show(Builder* builder, Library& library, bool capturing);
    void ValidateCurrentMaterial(Library& library);

    std::vector<EvaluatorFile> mEvaluatorFiles;

    void SetExistingMaterialActive(int materialIndex);
    void SetExistingMaterialActive(const char* materialName);
    void DecodeThumbnailAsync(Material* material);

    static void RenderPreviewNode(int selNode, GraphControler& nodeGraphControler, bool forceUI = false);
    void HandleHotKeys();

    Material& NewMaterial(const std::string& materialName = "Name_Of_New_Material");
    // helper for python scripting
    int AddNode(const std::string& nodeType);
    void DeleteCurrentMaterial();

    void RunDeferedCommands();
    static Imogen* instance;

    GraphControler* GetNodeGraphControler()
    {
        return mNodeGraphControler;
    }
    bool ShowMouseState() const
    {
        return mbShowMouseState;
    }
    void RunCommandAsync(const std::string& command, bool quitAfterRunCommand)
    {
        for (auto& plugin : mRegisteredPlugins)
        {
            if (plugin.mName == command)
            {
                mRunCommand = plugin.mPythonCommand;
                mbQuitAfterRunCommand = quitAfterRunCommand;
                break;
            }
        }
    }

protected:
    void ShowAppMainMenuBar();
    void ShowTitleBar(Builder* builder);
    void LibraryEdit(Library& library);
    void ClearAll();
    void UpdateNewlySelectedGraph();
    void ShowTimeLine();
    void ShowNodeGraph();
    void BuildCurrentMaterial(Builder* builder);
    void PlayPause();
    void ShowDebugWindow();
    void ShowExtractedViews();
    int EditRecentLibraries(RecentLibraries& recentLibraries);
    void CloseLibrary();
    void ImportMaterial();
    void ExportMaterial();

    void Playback(bool timeHasChanged);

    int GetFunctionByName(const char* functionName) const;
    bool ImageButton(const char* functionName, TextureHandle icon, ImVec2 size);
    bool Button(const char* functionName, const char* label, ImVec2 size);

    static void ReadLine(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line_start);
    static void WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);
    static void* ReadOpen(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name);

    MySequence* mSequence;
    GraphControler* mNodeGraphControler;
    Builder* mBuilder;
    bool mbShowTimeline = false;
    bool mbShowLibrary = false;
    bool mbShowNodes = false;
    bool mbShowLog = false;
    bool mbShowParameters = false;
    bool mbShowMouseState = false;
    bool mbDebugWindow = false;
    int mLibraryViewMode = 1;

    float mMainMenuDest = -440.f;
    float mMainMenuPos = -440.f;

    const char* mNewPopup = nullptr;
    int mSelectedMaterial = -1;
    int mCurrentShaderIndex = -1;

    bool mbIsPlaying = false;
    bool mbPlayLoop = false;
    int mCurrentTime = 0;

    std::string mRunCommand;
    bool mbQuitAfterRunCommand = false;
    std::vector<std::function<void()>> mHotkeyFunctions;

    RecentLibraries& mRecentLibraries;
};

void AddExtractedView(size_t nodeIndex);
void ClearExtractedViews();
void ExtractedViewNodeDeleted(size_t nodeIndex);
void ExtractedViewNodeInserted(size_t nodeIndex);
