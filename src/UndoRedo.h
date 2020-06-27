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

#include <functional>

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
    template<typename T>
    void AddSubUndoRedo(const T& subUndoRedo)
    {
        mSubUndoRedo.push_back(std::make_shared<T>(subUndoRedo));
    }

    bool HasSubUndoRedo() const
    {
        return !mSubUndoRedo.empty();
    }

    virtual size_t GetMemSize() const = 0;
    virtual size_t GetMemUsed() const
    {
        size_t ret = 0;
        for (const auto& sub : mSubUndoRedo)
        {
            ret += sub->GetMemSize();
        }
        return ret;
    }

protected:
    std::vector<std::shared_ptr<UndoRedo>> mSubUndoRedo;
};

struct UndoRedoHandler
{
    UndoRedoHandler() : mbProcessing(false), mCurrent(NULL)
    {
    }
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

    template<typename T>
    void AddUndo(const T& undoRedo)
    {
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

    size_t GetMemUsed() const
    {
        size_t ret = 0;
        for (const auto& undo : mUndos)
        {
            ret += undo->GetMemUsed();
        }
        for (const auto& redo : mRedos)
        {
            ret += redo->GetMemUsed();
        }
        return ret;

    }

    bool mbProcessing;
    UndoRedo* mCurrent;
    // private:

    std::vector<std::shared_ptr<UndoRedo>> mUndos;
    std::vector<std::shared_ptr<UndoRedo>> mRedos;
};

extern UndoRedoHandler gUndoRedoHandler;

inline UndoRedo::UndoRedo()
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

template<typename T>
struct URChange : public UndoRedo
{
    URChange(int index,
             std::function<T*(int index)> GetElements,
             std::function<void(int index)> Changed = [](int index) {})
        : GetElements(GetElements), mIndex(index), Changed(Changed)
    {
        if (gUndoRedoHandler.mbProcessing)
            return;

        mPreDo = *GetElements(mIndex);
    }

    virtual ~URChange()
    {
        if (gUndoRedoHandler.mbProcessing)
            return;

        mPostDo = *GetElements(mIndex);
        gUndoRedoHandler.AddUndo(*this);
    }

    void Undo() override
    {
        *GetElements(mIndex) = mPreDo;
        Changed(mIndex);
        UndoRedo::Undo();
    }

    void Redo() override
    {
        UndoRedo::Redo();
        *GetElements(mIndex) = mPostDo;
        Changed(mIndex);
    }

    size_t GetMemSize() const override
    {
        return sizeof(URChange<T>);
    }

    T mPreDo;
    T mPostDo;
    int mIndex;

    std::function<T*(int index)> GetElements;
    std::function<void(int index)> Changed;
};


template<typename T>
struct URChangeValue : public UndoRedo
{
	URChangeValue(T& value,
		std::function<void()> Changed = []() {})
		: mValue(value), Changed(Changed)
	{
		if (gUndoRedoHandler.mbProcessing)
			return;

		mPreDo = mValue;
	}

	virtual ~URChangeValue()
	{
		if (gUndoRedoHandler.mbProcessing)
			return;

		mPostDo = mValue;
		gUndoRedoHandler.AddUndo(*this);
	}

	void Undo() override
	{
		mValue = mPreDo;
		Changed();
		UndoRedo::Undo();
	}

	void Redo() override
	{
		UndoRedo::Redo();
		mValue = mPostDo;
		Changed();
	}

	size_t GetMemSize() const override
	{
		return sizeof(URChangeValue<T>);
	}

	T mPreDo;
	T mPostDo;
	T& mValue;

	std::function<void()> Changed;
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

    void Undo() override
    {
        UndoRedo::Undo();
    }

    void Redo() override
    {
        UndoRedo::Redo();
    }

    size_t GetMemSize() const override
    {
        return sizeof(URDummy);
    }
};


template<typename T>
struct URDel : public UndoRedo
{
    URDel(int index,
          std::function<std::vector<T>*()> GetElements,
          std::function<void(int index)> OnDelete = [](int index) {},
          std::function<void(int index)> OnNew = [](int index) {})
        : GetElements(GetElements), mIndex(index), OnDelete(OnDelete), OnNew(OnNew)
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

    void Undo() override
    {
        GetElements()->insert(GetElements()->begin() + mIndex, mDeletedElement);
        OnNew(mIndex);
        UndoRedo::Undo();
    }

    void Redo() override
    {
        UndoRedo::Redo();
        OnDelete(mIndex);
        GetElements()->erase(GetElements()->begin() + mIndex);
    }

    size_t GetMemSize() const override
    {
        return sizeof(URDummy);
    }

    T mDeletedElement;
    int mIndex;

    std::function<std::vector<T>*()> GetElements;
    std::function<void(int index)> OnDelete;
    std::function<void(int index)> OnNew;
};

template<typename T>
struct URAdd : public UndoRedo
{
    URAdd(int index,
          std::function<std::vector<T>*()> GetElements,
          std::function<void(int index)> OnDelete = [](int index) {},
          std::function<void(int index)> OnNew = [](int index) {})
        : GetElements(GetElements), mIndex(index), OnDelete(OnDelete), OnNew(OnNew)
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

    void Undo() override
    {
        OnDelete(mIndex);
        GetElements()->erase(GetElements()->begin() + mIndex);
        UndoRedo::Undo();
    }

    void Redo() override
    {
        UndoRedo::Redo();
        GetElements()->insert(GetElements()->begin() + mIndex, mAddedElement);
        OnNew(mIndex);
    }

    size_t GetMemSize() const override
    {
        return sizeof(URAdd<T>);
    }

    T mAddedElement;
    int mIndex;

    std::function<std::vector<T>*()> GetElements;
    std::function<void(int index)> OnDelete;
    std::function<void(int index)> OnNew;
};
