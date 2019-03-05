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

struct NodeGraphControler;
struct Evaluation;
class TextEditor;
struct Library;
struct Builder;
struct MySequence;

enum EVALUATOR_TYPE
{
    EVALUATOR_GLSL,
    EVALUATOR_C,
    EVALUATOR_PYTHON,
    EVALUATOR_GLSLCOMPUTE,
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
    Imogen(NodeGraphControler *nodeGraphControler);
    ~Imogen();

    void Init();
    void Finish();
    
    void Show(Builder *builder, Library& library);
    void ValidateCurrentMaterial(Library& library);
    void DiscoverNodes(const char *extension, const char *directory, EVALUATOR_TYPE evaluatorType, std::vector<EvaluatorFile>& files);

    std::vector<EvaluatorFile> mEvaluatorFiles;
    
    int GetCurrentMaterialIndex();
    void SetExistingMaterialActive(int materialIndex);
    void SetExistingMaterialActive(const char * materialName);
    void DecodeThumbnailAsync(Material * material);

    static void RenderPreviewNode(int selNode, NodeGraphControler& nodeGraphControler, bool forceUI = false);
protected:
    void HandleEditor(TextEditor &editor);
    void ShowAppMainMenuBar();
    void ShowTitleBar(Builder *builder);
    void LibraryEdit(Library& library);
    void ClearAll();
    void UpdateNewlySelectedGraph();
    void ShowTimeLine();
    void ShowNodeGraph();

    static void ReadLine(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line_start);
    static void WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);
    static void* ReadOpen(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name);

    MySequence *mSequence;
    NodeGraphControler *mNodeGraphControler;

    bool mbShowTimeline = false;
    bool mbShowLibrary = false;
    bool mbShowNodes = false;
    bool mbShowShaders = false;
    bool mbShowLog = false;
    bool mbShowParameters = false;
    int mLibraryViewMode = 1;

    static Imogen *instance;
};


extern int gEvaluationTime;

struct UndoRedo
{
    UndoRedo();
    virtual ~UndoRedo();

    virtual void Undo()
    {
        if (mSubUndoRedo.empty())
            return;
        for (int i = int(mSubUndoRedo.size()) - 1; i >= 0; i--)
        {
            mSubUndoRedo[i]->Undo();
        }
    }
    virtual void Redo()
    {
        for (auto& undoRedo : mSubUndoRedo)
        {
            undoRedo->Redo();
        }
    }
    template <typename T> void AddSubUndoRedo(const T& subUndoRedo)
    {
        mSubUndoRedo.push_back(std::make_shared<T>(subUndoRedo));
    }
    void Discard() { mbDiscarded = true; }
    bool IsDiscarded() const { return mbDiscarded; }
protected:
    std::vector<std::shared_ptr<UndoRedo> > mSubUndoRedo;
    bool mbDiscarded;
};

struct UndoRedoHandler
{
    UndoRedoHandler() : mbProcessing(false), mCurrent(NULL) {}
    ~UndoRedoHandler()
    {
        Clear();
    }

    void Undo()
    {
        if (mUndos.empty())
            return;
        mbProcessing = true;
        mUndos.back()->Undo();
        mRedos.push_back(mUndos.back());
        mUndos.pop_back();
        mbProcessing = false;
    }

    void Redo()
    {
        if (mRedos.empty())
            return;
        mbProcessing = true;
        mRedos.back()->Redo();
        mUndos.push_back(mRedos.back());
        mRedos.pop_back();
        mbProcessing = false;
    }

    template <typename T> void AddUndo(const T &undoRedo)
    {
        if (undoRedo.IsDiscarded())
            return;
        if (mCurrent && &undoRedo != mCurrent)
            mCurrent->AddSubUndoRedo(undoRedo);
        else
            mUndos.push_back(std::make_shared<T>(undoRedo));
        mbProcessing = true;
        mRedos.clear();
        mbProcessing = false;
    }

    void Clear()
    {
        mbProcessing = true;
        mUndos.clear();
        mRedos.clear();
        mbProcessing = false;
    }

    bool mbProcessing;
    UndoRedo* mCurrent;
    //private:

    std::vector<std::shared_ptr<UndoRedo> > mUndos;
    std::vector<std::shared_ptr<UndoRedo> > mRedos;
};

extern UndoRedoHandler gUndoRedoHandler;

inline UndoRedo::UndoRedo() : mbDiscarded(false)
{
    if (!gUndoRedoHandler.mCurrent)
    {
        gUndoRedoHandler.mCurrent = this;
    }
}

inline UndoRedo::~UndoRedo()
{
    if (gUndoRedoHandler.mCurrent == this)
    {
        gUndoRedoHandler.mCurrent = NULL;
    }
}

template<typename T> struct URChange : public UndoRedo
{
    URChange(int index, std::function<T*(int index)> GetElements, std::function<void(int index)> Changed = [](int index) {}) : GetElements(GetElements), mIndex(index), Changed(Changed)
    {
        if (gUndoRedoHandler.mbProcessing)
            return;

        mPreDo = *GetElements(mIndex);
    }
    virtual ~URChange()
    {
        if (gUndoRedoHandler.mbProcessing || mbDiscarded)
            return;

        if (*GetElements(mIndex) != mPreDo)
        {
            mPostDo = *GetElements(mIndex);
            gUndoRedoHandler.AddUndo(*this);
        }
        else
        {
            // TODO: should not be here unless asking for too much useless undo
        }
    }
    virtual void Undo()
    {
        *GetElements(mIndex) = mPreDo;
        Changed(mIndex);
        UndoRedo::Undo();
    }
    virtual void Redo()
    {
        UndoRedo::Redo();
        *GetElements(mIndex) = mPostDo;
        Changed(mIndex);
    }

    T mPreDo;
    T mPostDo;
    int mIndex;

    std::function<T*(int index)> GetElements;
    std::function<void(int index)> Changed;
};


struct URDummy : public UndoRedo
{
    URDummy() : UndoRedo()
    {
        if (gUndoRedoHandler.mbProcessing)
            return;
    }
    virtual ~URDummy()
    {
        if (gUndoRedoHandler.mbProcessing)
            return;

        gUndoRedoHandler.AddUndo(*this);
    }
    virtual void Undo()
    {
        UndoRedo::Undo();
    }
    virtual void Redo()
    {
        UndoRedo::Redo();
    }
};


template<typename T> struct URDel : public UndoRedo
{
    URDel(int index, std::function<std::vector<T>* ()> GetElements, std::function<void(int index)> OnDelete = [](int index) {}, std::function<void(int index)> OnNew = [](int index) {}) :
        GetElements(GetElements), mIndex(index), OnDelete(OnDelete), OnNew(OnNew)
    {
        if (gUndoRedoHandler.mbProcessing)
            return;

        mDeletedElement = (*GetElements())[mIndex];
    }
    virtual ~URDel()
    {
        if (gUndoRedoHandler.mbProcessing || mbDiscarded)
            return;
        // add to handler
        gUndoRedoHandler.AddUndo(*this);
    }
    virtual void Undo()
    {
        GetElements()->insert(GetElements()->begin() + mIndex, mDeletedElement);
        OnNew(mIndex);
        UndoRedo::Undo();
    }
    virtual void Redo()
    {
        UndoRedo::Redo();
        OnDelete(mIndex);
        GetElements()->erase(GetElements()->begin() + mIndex);
    }

    T mDeletedElement;
    int mIndex;

    std::function<std::vector<T>* ()> GetElements;
    std::function<void(int index)> OnDelete;
    std::function<void(int index)> OnNew;
};

template<typename T> struct URAdd : public UndoRedo
{
    URAdd(int index, std::function<std::vector<T>* ()> GetElements, std::function<void(int index)> OnDelete = [](int index) {}, std::function<void(int index)> OnNew = [](int index) {}) :
        GetElements(GetElements), mIndex(index), OnDelete(OnDelete), OnNew(OnNew)
    {
    }
    virtual ~URAdd()
    {
        if (gUndoRedoHandler.mbProcessing || mbDiscarded)
            return;

        mAddedElement = (*GetElements())[mIndex];
        // add to handler
        gUndoRedoHandler.AddUndo(*this);
    }
    virtual void Undo()
    {
        OnDelete(mIndex);
        GetElements()->erase(GetElements()->begin() + mIndex);
        UndoRedo::Undo();
    }
    virtual void Redo()
    {
        UndoRedo::Redo();
        GetElements()->insert(GetElements()->begin() + mIndex, mAddedElement);
        OnNew(mIndex);
    }

    T mAddedElement;
    int mIndex;

    std::function<std::vector<T>* ()> GetElements;
    std::function<void(int index)> OnDelete;
    std::function<void(int index)> OnNew;
};
