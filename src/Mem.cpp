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
#include "Mem.h"
#include <malloc.h>

#ifdef RETAIL
MemoryProfileHistoryNULL gMemoryHistory;
#else
MemoryProfileHistory_t gMemoryHistory;
#endif

template<> bool MemoryProfileHistory<64, MODULE_COUNT>::inited = false;

void *imguiMalloc(size_t n, void* user_data)
{
    return HeapAllocatorBase<unsigned char, MODULE_IMGUI>().allocate(n);
}

void imguiFree(void *ptr, void* user_data)
{
    if (!ptr)
        return;
#ifdef WIN32
    auto ptrSize = _msize(ptr);
#else
    auto ptrSize = malloc_usable_size(ptr);
#endif
    return HeapAllocatorBase<unsigned char, MODULE_IMGUI>().deallocate((unsigned char*)ptr, ptrSize);
}


void *imageMalloc(size_t n)
{
    return HeapAllocatorBase<unsigned char, MODULE_IMAGE>().allocate(n);
}

void imageFree(void *ptr)
{
    if (!ptr)
        return;
#ifdef WIN32
    auto ptrSize = _msize(ptr);
#else
    auto ptrSize = malloc_usable_size(ptr);
#endif
    return HeapAllocatorBase<unsigned char, MODULE_IMAGE>().deallocate((unsigned char*)ptr, ptrSize);
}

void vramTextureAlloc(size_t n)
{
    gMemoryHistory.logAllocation(MODULE_TEXTURE, n);
}

void vramTextureFree(size_t n)
{
    gMemoryHistory.logDeallocation(MODULE_TEXTURE, n);
}