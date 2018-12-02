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

#pragma once

#include <vector>
#include <string>
#include <memory>
#include "imgui.h"
#include "imgui_internal.h"
#include "Library.h"

struct TileNodeEditGraphDelegate;
struct Evaluation;
class TextEditor;
struct Library;


enum EVALUATOR_TYPE
{
	EVALUATOR_GLSL,
	EVALUATOR_C,
	EVALUATOR_PYTHON,
};

struct EvaluatorFile
{
	std::string mDirectory;
	std::string mFilename;
	EVALUATOR_TYPE mEvaluatorType;
};

struct Imogen
{
	Imogen();
	~Imogen();

	void Init();
	void Finish();
	
	void Show(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation);
	void ValidateCurrentMaterial(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate);
	void DiscoverNodes(const char *extension, const char *directory, EVALUATOR_TYPE evaluatorType, std::vector<EvaluatorFile>& files);

	std::vector<EvaluatorFile> mEvaluatorFiles;
	int GetCurrentMaterialIndex();

protected:
	void HandleEditor(TextEditor &editor, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation);
};

void DebugLogText(const char *szText);
enum CallbackDisplayType
{
	CBUI_Node,
	CBUI_Progress,
	CBUI_Cubemap
};
struct ImogenDrawCallback
{
	CallbackDisplayType mType;
	ImRect mClippedRect;
	ImRect mOrginalRect;
	size_t mNodeIndex;
};
extern std::vector<ImogenDrawCallback> mCallbackRects;
void InitCallbackRects();
size_t AddNodeUICallbackRect(CallbackDisplayType type, const ImRect& rect, size_t nodeIndex);
extern int gEvaluationTime;

struct UndoRedo
{
	UndoRedo();
	virtual ~UndoRedo();

	virtual void Undo()
	{
		//printf(" Undoing\n");
		if (mSubUndoRedo.empty())
			return;
		for (int i = int(mSubUndoRedo.size()) - 1; i >= 0; i--)
		{
			//printf("Undoing sub\n");
			mSubUndoRedo[i]->Undo();
		}
	}
	virtual void Redo()
	{
		//printf(" Redoing\n");
		for (auto& undoRedo : mSubUndoRedo)
		{
			//printf("Redoing sub\n");
			undoRedo->Redo();
		}
	}
	template <typename T> void AddSubUndoRedo(const T& subUndoRedo)
	{
		//printf("adding sub UR\n");
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
		mbProcessing = true;
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
		mUndos.clear();
		mRedos.clear();
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
		//printf("new master UR\n");
		gUndoRedoHandler.mCurrent = this;
	}
}

inline UndoRedo::~UndoRedo()
{
	if (gUndoRedoHandler.mCurrent == this)
	{
		//printf("remove master UR\n");
		gUndoRedoHandler.mCurrent = NULL;
	}
}

template<typename T> struct URChange : public UndoRedo
{
	URChange(int index, T* (*GetElements)(int index), void(*Changed)(int index) = [](int index) {}) : GetElements(GetElements), mIndex(index), Changed(Changed)
	{
		if (gUndoRedoHandler.mbProcessing)
			return;

		mPreDo = *GetElements(mIndex);
	}
	virtual ~URChange()
	{
		if (gUndoRedoHandler.mbProcessing)
			return;

		// add to handler
		/*if (gUndoRedoHandler.mLastElementPtr == GetElements(mIndex))
		{
			gUndoRedoHandler.mUndos.back()->UpdatePostDo();
		}
		else
		*/
		{
			mPostDo = *GetElements(mIndex);
			gUndoRedoHandler.AddUndo(*this);
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

	T* (*GetElements)(int index);
	void(*Changed)(int index);
};

template<typename T> struct URDel : public UndoRedo
{
	URDel(int index, std::vector<T>* (*GetElements)(), void(*OnDelete)(int index) = [](int index) {}, void(*OnNew)(int index) = [](int index) {}) : GetElements(GetElements), mIndex(index), OnDelete(OnDelete), OnNew(OnNew)
	{
		if (gUndoRedoHandler.mbProcessing)
			return;

		mDeletedElement = (*GetElements())[mIndex];
	}
	virtual ~URDel()
	{
		if (gUndoRedoHandler.mbProcessing)
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

	std::vector<T>* (*GetElements)();
	void(*OnDelete)(int index);
	void(*OnNew)(int index);
};

template<typename T> struct URAdd : public UndoRedo
{
	URAdd(int index, std::vector<T>* (*GetElements)(), void(*OnDelete)(int index)  = [](int index) {}, void(*OnNew)(int index) = [](int index) {}) : GetElements(GetElements), mIndex(index), OnDelete(OnDelete), OnNew(OnNew)
	{
	}
	virtual ~URAdd()
	{
		if (gUndoRedoHandler.mbProcessing)
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

	std::vector<T>* (*GetElements)();
	void(*OnDelete)(int index);
	void(*OnNew)(int index);
};


