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

    static char buf[10240];
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

///////////////////////////////////////////////////////////////////////////////////////////////////
// matrix will receive the calculated perspective matrix.
// You would have to upload to your shader
// or use glLoadMatrixf if you aren't using shaders.
void Mat4x4::glhPerspectivef2(float fovyInDegrees, float aspectRatio, float znear, float zfar)
{
    float ymax, xmax;
    ymax = znear * tanf(fovyInDegrees * 3.14159f / 360.0f);
    xmax = ymax * aspectRatio;
    glhFrustumf2(-xmax, xmax, -ymax, ymax, znear, zfar);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Mat4x4::glhFrustumf2(float left, float right, float bottom, float top, float znear, float zfar)
{
    float temp, temp2, temp3, temp4;
    temp = 2.0f * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;
    m16[0] = temp / temp2;
    m16[1] = 0.0;
    m16[2] = 0.0;
    m16[3] = 0.0;
    m16[4] = 0.0;
    m16[5] = temp / temp3;
    m16[6] = 0.0;
    m16[7] = 0.0;
    m16[8] = (right + left) / temp2;
    m16[9] = (top + bottom) / temp3;
    m16[10] = (-zfar - znear) / temp4;
    m16[11] = -1.0f;
    m16[12] = 0.0;
    m16[13] = 0.0;
    m16[14] = (-temp * zfar) / temp4;
    m16[15] = 0.0;
}

void Mat4x4::lookAtRH(const Vec4& eye, const Vec4& at, const Vec4& up)
{
    Vec4 X, Y, Z, tmp;

    Z.Normalize(eye - at);
    Y.Normalize(up);

    tmp.Cross(Y, Z);
    X.Normalize(tmp);

    tmp.Cross(Z, X);
    Y.Normalize(tmp);

    m[0][0] = X.x;
    m[0][1] = Y.x;
    m[0][2] = Z.x;
    m[0][3] = 0.0f;

    m[1][0] = X.y;
    m[1][1] = Y.y;
    m[1][2] = Z.y;
    m[1][3] = 0.0f;

    m[2][0] = X.z;
    m[2][1] = Y.z;
    m[2][2] = Z.z;
    m[2][3] = 0.0f;

    m[3][0] = -X.Dot(eye);
    m[3][1] = -Y.Dot(eye);
    m[3][2] = -Z.Dot(eye);
    m[3][3] = 1.0f;
}


void Mat4x4::lookAtLH(const Vec4& eye, const Vec4& at, const Vec4& up)
{
    Vec4 X, Y, Z, tmp;

    Z.Normalize(at - eye);
    Y.Normalize(up);

    tmp.Cross(Y, Z);
    X.Normalize(tmp);

    tmp.Cross(Z, X);
    Y.Normalize(tmp);

    m[0][0] = X.x;
    m[0][1] = Y.x;
    m[0][2] = Z.x;
    m[0][3] = 0.0f;

    m[1][0] = X.y;
    m[1][1] = Y.y;
    m[1][2] = Z.y;
    m[1][3] = 0.0f;

    m[2][0] = X.z;
    m[2][1] = Y.z;
    m[2][2] = Z.z;
    m[2][3] = 0.0f;

    m[3][0] = -X.Dot(eye);
    m[3][1] = -Y.Dot(eye);
    m[3][2] = -Z.Dot(eye);
    m[3][3] = 1.0f;
}

void Mat4x4::LookAt(const Vec4& eye, const Vec4& at, const Vec4& up)
{
    Vec4 X, Y, Z, tmp;

    Z.Normalize(at - eye);
    Y.Normalize(up);

    tmp.Cross(Y, Z);
    X.Normalize(tmp);

    tmp.Cross(Z, X);
    Y.Normalize(tmp);

    m[0][0] = X.x;
    m[0][1] = X.y;
    m[0][2] = X.z;
    m[0][3] = 0.0f;

    m[1][0] = Y.x;
    m[1][1] = Y.y;
    m[1][2] = Y.z;
    m[1][3] = 0.0f;

    m[2][0] = Z.x;
    m[2][1] = Z.y;
    m[2][2] = Z.z;
    m[2][3] = 0.0f;

    m[3][0] = eye.x;
    m[3][1] = eye.y;
    m[3][2] = eye.z;
    m[3][3] = 1.0f;
}

void Mat4x4::PerspectiveFovLH2(const float fovy, const float aspect, const float zn, const float zf)
{
    /*
        xScale     0          0               0
    0        yScale       0               0
    0          0       zf/(zf-zn)         1
    0          0       -zn*zf/(zf-zn)     0
    where:
    */
    /*
    +    pout->m[0][0] =3D 1.0f / (aspect * tan(fovy/2.0f));
    +    pout->m[1][1] =3D 1.0f / tan(fovy/2.0f);
    +    pout->m[2][2] =3D zf / (zf - zn);
    +    pout->m[2][3] =3D 1.0f;
    +    pout->m[3][2] =3D (zf * zn) / (zn - zf);
    +    pout->m[3][3] =3D 0.0f;



    float yscale = cosf(fovy*0.5f);

    float xscale = yscale / aspect;

    */
    m[0][0] = 1.0f / (aspect * tanf(fovy * 0.5f));
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[0][3] = 0.0f;

    m[1][0] = 0.0f;
    m[1][1] = 1.0f / tanf(fovy * 0.5f);
    m[1][2] = 0.0f;
    m[1][3] = 0.0f;

    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = zf / (zf - zn);
    m[2][3] = 1.0f;

    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = (zf * zn) / (zn - zf);
    m[3][3] = 0.0f;
}

void Mat4x4::OrthoOffCenterLH(const float l, float r, float b, const float t, float zn, const float zf)
{
    m[0][0] = 2 / (r - l);
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[0][3] = 0.0f;

    m[1][0] = 0.0f;
    m[1][1] = 2 / (t - b);
    m[1][2] = 0.0f;
    m[1][3] = 0.0f;

    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = 1.0f / (zf - zn);
    m[2][3] = 0.0f;

    m[3][0] = (l + r) / (l - r);
    m[3][1] = (t + b) / (b - t);
    m[3][2] = zn / (zn - zf);
    m[3][3] = 1.0f;
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

void GetTextureDimension(TextureHandle textureId, int* w, int* h)
{
	*w = 256;
	*h = 256;
    /* todogl
	int miplevel = 0;
    glBindTexture(GL_TEXTURE_2D, textureId);
    #ifdef GL_TEXTURE_WIDTH
    glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_WIDTH, w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_HEIGHT, h);
    #endif
	*/
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