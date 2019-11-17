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
#include <vector>
#include "Utils.h"
#include "EvaluationStages.h"
#include "tinydir.h"

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

std::vector<LogOutput> outputs;
void AddLogOutput(LogOutput output)
{
    outputs.push_back(output);
}

int Log(const char* szFormat, ...)
{
    va_list ptr_arg;
    va_start(ptr_arg, szFormat);

    static char buf[102400];
    vsprintf(buf, szFormat, ptr_arg);

    static FILE* fp = fopen("log.txt", "wt");
    if (fp)
    {
        fprintf(fp, "%s", buf);
        fflush(fp);
    }
    for (auto output : outputs)
        output(buf);
    va_end(ptr_arg);
    return 0;
}

void DiscoverFiles(const char* ext, const char* directory, std::vector<std::string>& files)
{
    tinydir_dir dir;
    tinydir_open(&dir, directory);

    while (dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&dir, &file);

        if (!file.is_dir && !strcmp(file.extension, ext))
        {
            files.push_back(std::string(directory) + file.name);
        }

        tinydir_next(&dir);
    }

    tinydir_close(&dir);
}

void IMessageBox(const char* text, const char* title)
{
#ifdef WIN
    MessageBoxA(NULL, text, title, MB_OK);
#endif
}

void OpenShellURL(const std::string& url)
{
#ifdef WIN
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
}

std::string GetGroup(const std::string& name)
{
    for (int i = int(name.length()) - 1; i >= 0; i--)
    {
        if (name[i] == '/')
        {
            return name.substr(0, i);
        }
    }
    return "";
}

std::string GetName(const std::string& name)
{
    for (int i = int(name.length()) - 1; i >= 0; i--)
    {
        if (name[i] == '/')
        {
            return name.substr(i + 1);
        }
    }
    return name;
}

void Splitpath(const char* completePath, char* drive, char* dir, char* filename, char* ext)
{
#ifdef WIN32
    _splitpath(completePath, drive, dir, filename, ext);
#endif
#ifdef __linux__
    char* pathCopy = (char*)completePath;
    int Counter = 0;
    int Last = 0;
    int Rest = 0;

    // no drives available in linux .
    // exts are not common in linux
    // but considered anyway
    strcpy(drive, "");

    while (*pathCopy != '\0')
    {
        // search for the last slash
        while (*pathCopy != '/' && *pathCopy != '\0')
        {
            pathCopy++;
            Counter++;
        }
        if (*pathCopy == '/')
        {
            pathCopy++;
            Counter++;
            Last = Counter;
        }
        else
            Rest = Counter - Last;
    }
    // directory is the first part of the path until the
    // last slash appears
    strncpy(dir, completePath, Last);
    // strncpy doesnt add a '\0'
    dir[Last] = '\0';
    // filename is the part behind the last slahs
    strcpy(filename, pathCopy -= Rest);
    // get ext if there is any
    while (*filename != '\0')
    {
        // the part behind the point is called ext in windows systems
        // at least that is what i thought apperantly the '.' is used as part
        // of the ext too .
        if (*filename == '.')
        {
            while (*filename != '\0')
            {
                *ext = *filename;
                ext++;
                filename++;
            }
        }
        if (*filename != '\0')
        {
            filename++;
        }
    }
    *ext = '\0';
    return;
#endif
}