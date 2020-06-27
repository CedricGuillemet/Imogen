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

struct RecentLibraries
{
#ifdef __EMSCRIPTEN__
    const char* RecentFilename = "/offline/ImogenRecent";
#else
    const char* RecentFilename = "ImogenRecent";
#endif
    RecentLibraries()
    {
        ReadRecentLibraries();
    }

    struct RecentLibrary
    {
        std::string mName;
        std::string mPath;

        std::string ComputeFullPath() const
        {
            return mPath + mName + ".imogen";
        }

        bool operator == (const RecentLibrary& other) const
        {
            return mName == other.mName && mPath == other.mPath;
        }
    };

    const std::vector<RecentLibrary>& GetRecentLibraries() const
    {
        return mRecentLibraries;
    }

    void ReadRecentLibraries();

    void WriteRecentLibraries();

    bool IsNamedUsed(const std::string& name) const
    {
        for (const auto& recent : mRecentLibraries)
        {
            if (recent.mName == name)
            {
                return true;
            }
        }
        return false;
    }

    size_t AddRecent(const std::string& path, const std::string& name);
    size_t AddRecent(const std::string& completePath);

    bool IsValidForAddingRecent(const std::string& completePath) const;

    bool IsValidForCreatingRecent(const std::string& path, const std::string& name) const;

    std::string GetMostRecentLibraryPath() const
    {
        if (mMostRecentLibrary == -1)
        {
            return {};
        }
        const auto& recentLibrary = mRecentLibraries[mMostRecentLibrary];
        return recentLibrary.ComputeFullPath();
    }

    void SetMostRecentLibraryIndex(int index)
    {
        mMostRecentLibrary = index;
        WriteRecentLibraries();
    }

    int GetMostRecentLibraryIndex() const
    {
        return mMostRecentLibrary;
    }

    int mMostRecentLibrary = -1;
    std::vector<RecentLibrary> mRecentLibraries;
};
