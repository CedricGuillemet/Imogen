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
#include <assert.h>
#include <memory.h>
#include <malloc.h>

enum MODULES
{
    MODULE_IMGUI,
    MODULE_DEFAULT,
    MODULE_TEXTURE,
    MODULE_COUNT
};

template <size_t historyLength, size_t maxModuleCount> struct MemoryProfileHistory
{
    template <size_t historyLength2> struct MemoryHistory
    {
        //MemoryHistory() : mOffset(0) { memset(mValues, 0, sizeof(size_t) * historyLength); }
        size_t mValues[historyLength2];
        size_t mOffset;
        void addValue(size_t value)
        {
            mValues[mOffset] = value;
            ++mOffset %= historyLength2;
        }
        size_t getValue() const
        {
            return mValues[(mOffset + historyLength2 - 1) % historyLength2];
        }
        size_t getMin() const
        {
            size_t v = mValues[0];
            for (int i = 1; i < historyLength2; i++)
                v = (v < mValues[i]) ? v : mValues[i];
            return v;
        }
        size_t getMax() const
        {
            size_t v = mValues[0];
            for (int i = 1; i < historyLength2; i++)
                v = (v > mValues[i]) ? v : mValues[i];
            return v;
        }
        void clear()
        {
            memset(mValues, 0, historyLength2 * sizeof(size_t));
            mOffset = 0;
        }
    };
    typedef MemoryHistory<historyLength> MemHistory;


    MemoryProfileHistory() {}

    void LogChecker(size_t module)
    {
        MemoryModule& mmp = mMemory[module];
        assert(mmp.mActiveAllocationCount.getValue() >= 0);
        assert(mmp.mAllocationCount.getValue() >= mmp.mDeallocationCount.getValue());
        assert(mmp.mTotalFreed.getValue() <= mmp.mTotalAllocated.getValue());
    }

    void logAllocation(size_t module, size_t n)
    {
        if (!inited)
        {
            inited = true;
            clear();
        }
        MemoryModule& mmp = mMemory[module];
        mmp.mTotalAllocated.addValue(mmp.mTotalAllocated.getValue() + n);
        mmp.mAllocationCount.addValue(mmp.mAllocationCount.getValue() + 1);
        mmp.mActiveAllocated.addValue(mmp.mActiveAllocated.getValue() + n);
        mmp.mActiveAllocationCount.addValue(mmp.mActiveAllocationCount.getValue() + 1);
        LogChecker(module);
    }

    void logDeallocation(size_t module, size_t n)
    {
        MemoryModule& mmp = mMemory[module];
        mmp.mTotalFreed.addValue(mmp.mTotalFreed.getValue() + n);
        mmp.mDeallocationCount.addValue(mmp.mDeallocationCount.getValue() + 1);
        mmp.mActiveAllocated.addValue(mmp.mActiveAllocated.getValue() - n);
        mmp.mActiveAllocationCount.addValue(mmp.mActiveAllocationCount.getValue() - 1);
        LogChecker(module);
    }

    void dumpLeaks() {}
    bool hasAllocationFor(size_t module) const { return false; }
    void clear()
    {
        for (size_t i = 0; i < MODULE_COUNT; i++)
            clearModule(i);
    }
    void clearModule(size_t module) { mMemory[module].clear(); }
    struct MemoryModule
    {
        MemHistory mTotalAllocated;
        MemHistory mTotalFreed;
        MemHistory mAllocationCount;
        MemHistory mDeallocationCount;
        MemHistory mActiveAllocated;
        MemHistory mActiveAllocationCount;

        void clear()
        {
            mTotalAllocated.clear();
            mTotalFreed.clear();
            mAllocationCount.clear();
            mDeallocationCount.clear();
            mActiveAllocated.clear();
            mActiveAllocationCount.clear();
        }
    };

    MemoryModule mMemory[maxModuleCount];
    const MemoryModule& getMem(MODULES module) const { return mMemory[module]; }

    static bool inited;
};

struct MemoryProfileHistoryNULL
{
    void logAllocation(size_t module, size_t n) {}
    void logDeallocation(size_t module, size_t n) {}
    void clearModule(size_t) { }
};

void InitMemProf();
void MemoryFrameInit();

#ifdef RETAIL
extern MemoryProfileHistoryNULL gMemoryHistory;
#else
typedef MemoryProfileHistory<64, MODULE_COUNT> MemoryProfileHistory_t;
extern MemoryProfileHistory_t gMemoryHistory;
#endif

template <class T, size_t module> struct HeapAllocatorBase
{
    using value_type = T;
    HeapAllocatorBase() {}
    T* allocate(size_t n)
    {
        size_t sz = sizeof(T) * n;
        gMemoryHistory.logAllocation(module, sz);
        return static_cast<T*>(malloc(sz));
    }
    void deallocate(T* p, size_t n)
    {
        if (!p)
            return;

        size_t sz = sizeof(T) * n;
        gMemoryHistory.logDeallocation(module, sz);
        free(p);
    }
};



void *imguiMalloc(size_t n, void* user_data);
void imguiFree(void *ptr, void* user_data);

void vramTextureAlloc(size_t n);
void vramTextureFree(size_t n);
