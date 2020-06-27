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

#include "Libraries.h"
#include "Utils.h"
#include "Library.h"

size_t RecentLibraries::AddRecent(const std::string& path, const std::string& name)
{
    RecentLibrary newRecentLibrary{ name, path };
    if (std::find(mRecentLibraries.begin(), mRecentLibraries.end(), newRecentLibrary) == mRecentLibraries.end())
    {
        mRecentLibraries.push_back(newRecentLibrary);
        WriteRecentLibraries();
    }
    return mRecentLibraries.size() - 1;
}

size_t RecentLibraries::AddRecent(const std::string& completePath)
{
    char drive[256];
    char dir[256];
    char filename[256];
    char ext[256];
    Splitpath(completePath.c_str(), drive, dir, filename, ext);
    return AddRecent(std::string(drive) + std::string(dir), std::string(filename));
}

void RecentLibraries::ReadRecentLibraries()
{
    LoadRecent(this, RecentFilename);
    if (mRecentLibraries.empty())
    {
        // add default
#ifdef __EMSCRIPTEN__
        mRecentLibraries.push_back({ "DefaultLibrary", "" });
#else
        mRecentLibraries.push_back({ "DefaultLibrary", "./" });
#endif
    }
    else
    {
        auto iter = mRecentLibraries.begin();
        for (; iter != mRecentLibraries.end();)
        {
            FILE* fp = fopen(iter->ComputeFullPath().c_str(), "rb");
            if (!fp)
            {
                Log("Library %s removed from recent because not found in FS\n", iter->ComputeFullPath().c_str());
                mMostRecentLibrary = -1; // something happened (deleted libs) -> display all libraries
                iter = mRecentLibraries.erase(iter);
                continue;
            }
            fclose(fp);
            ++iter;
        }
    }
}

void RecentLibraries::WriteRecentLibraries()
{
    SaveRecent(this, RecentFilename);
}

bool RecentLibraries::IsValidForAddingRecent(const std::string& completePath) const
{
    if (!completePath.length())
    {
        return false;
    }
    for (const auto& recent : mRecentLibraries)
    {
        if (recent.ComputeFullPath() == completePath)
        {
            return false;
        }
    }

    FILE* fp = fopen(completePath.c_str(), "rb");
    if (!fp)
    {
        return false;
    }
    fclose(fp);

    return true;
}

bool RecentLibraries::IsValidForCreatingRecent(const std::string& path, const std::string& name) const
{
#ifdef __EMSCRIPTEN__
    if (!name.size() || IsNamedUsed(name))
    {
        return false;
    }
#else
    if (!path.size() || !name.size())
    {
        return false;
    }
    if (IsNamedUsed(name))
    {
        return false;
    }
    FILE* fp = fopen(RecentLibrary{ name, path }.ComputeFullPath().c_str(), "rb");
    if (fp)
    {
        fclose(fp);
        return false;
    }
#endif
    return true;
}