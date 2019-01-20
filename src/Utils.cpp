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
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#include <SDL.h>
#include <vector>
#include "Utils.h"
#include "Evaluation.h"
#include "tinydir.h"

void FlipVImage(Image *image)
{
    int pixelSize = (image->mFormat == TextureFormat::RGB8) ? 3 : 4;
    int stride = image->mWidth * pixelSize;
    for (int y = 0; y < image->mHeight / 2; y++)
    {
        for (int x = 0; x < stride; x++)
        {
            unsigned char * p1 = &image->GetBits()[y * stride + x];
            unsigned char * p2 = &image->GetBits()[(image->mHeight - 1 - y) * stride + x];
            ImSwap(*p1, *p2);
        }
    }
}

void TexParam(TextureID MinFilter, TextureID MagFilter, TextureID WrapS, TextureID WrapT, TextureID texMode)
{
    glTexParameteri(texMode, GL_TEXTURE_MIN_FILTER, MinFilter);
    glTexParameteri(texMode, GL_TEXTURE_MAG_FILTER, MagFilter);
    glTexParameteri(texMode, GL_TEXTURE_WRAP_S, WrapS);
    glTexParameteri(texMode, GL_TEXTURE_WRAP_T, WrapT);
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

void FullScreenTriangle::Init()
{
    TextureID fsVA;

    float fsVts[] = { 0.f,0.f, 2.f,0.f, 0.f,2.f };
    glGenBuffers(1, &fsVA);
    glBindBuffer(GL_ARRAY_BUFFER, fsVA);
    glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(float) * 2, fsVts, GL_STATIC_DRAW);

    glGenVertexArrays(1, &mGLFullScreenVertexArrayName);
    glBindVertexArray(mGLFullScreenVertexArrayName);
    glBindBuffer(GL_ARRAY_BUFFER, fsVA);
    glVertexAttribPointer(SemUV0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(SemUV0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void FullScreenTriangle::Render()
{
    glBindVertexArray(mGLFullScreenVertexArrayName);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

FullScreenTriangle gFSQuad;

unsigned int LoadShader(const std::string &shaderString, const char *fileName)
{
    TextureID programObject = glCreateProgram();
    if (programObject == 0)
        return 0;

    GLint compiled;
    const char *shaderTypeStrings[] = { "\n#version 430 core\n#define VERTEX_SHADER\n", "\n#version 430 core\n#define FRAGMENT_SHADER\n" };
    TextureID shaderTypes[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
    TextureID compiledShader[2];

    for (int i = 0; i < 2; i++)
    {
        // Create the shader object
        int shader = glCreateShader(shaderTypes[i]);

        if (shader == 0)
            return false;

        int stringsCount = 2;
        const char ** strings = (const char**)malloc(sizeof(char*) * stringsCount); //new const char*[stringsCount];
        int * stringLength = (int*)malloc(sizeof(int) * stringsCount); //new int[stringsCount];
        strings[0] = shaderTypeStrings[i];
        stringLength[0] = int(strlen(shaderTypeStrings[i]));
        strings[stringsCount - 1] = shaderString.c_str();
        stringLength[stringsCount - 1] = int(shaderString.length());

        // Load and compile the shader source
        glShaderSource(shader, stringsCount, strings, stringLength);
        glCompileShader(shader);


        free(stringLength);
        free(strings);

        // Check the compile status
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled == 0)
        {
            GLint info_len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
            if (info_len > 1)
            {
                char* info_log = (char*)malloc(sizeof(char) * info_len);
                glGetShaderInfoLog(shader, info_len, NULL, info_log);
                Log("Error compiling shader: %s \n", fileName);
                Log(info_log);
                Log("\n");
                free(info_log);
            }
            glDeleteShader(shader);
            return 0;
        }
        compiledShader[i] = shader;
    }



    GLint linked;

    for (int i = 0; i < 2; i++)
        glAttachShader(programObject, compiledShader[i]);


    // Link the program
    glLinkProgram(programObject);

    glBindAttribLocation(programObject, SemUV0, "inUV");

    // Check the link status
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
    if (linked == 0)
    {
        GLint info_len = 0;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1)
        {
            char* info_log = (char*)malloc(sizeof(char) * info_len);
            glGetProgramInfoLog(programObject, info_len, NULL, info_log);
            Log("Error linking program:\n");
            Log(info_log);
            free(info_log);
        }
        glDeleteProgram(programObject);
        return 0;
    }

    // Delete these here because they are attached to the program object.
    for (int i = 0; i < 2; i++)
        glDeleteShader(compiledShader[i]);

    // attributes
    return programObject;
}


unsigned int LoadShaderTransformFeedback(const std::string &shaderString, const char *filename)
{
    GLuint programHandle = glCreateProgram();
    GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);

    const char *src[2] = { "#version 430\n", shaderString.c_str() };
    int size[2];
    for (int j = 0; j < 2; j++)
        size[j] = int(strlen(src[j]));

    glShaderSource(vsHandle, 2, src, size);
    glCompileShader(vsHandle);

    GLint compiled;
    // Check the compile status
    glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &compiled);
    if (compiled == 0)
    {
        GLint info_len = 0;
        glGetShaderiv(vsHandle, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1)
        {
            char info_log[2048];
            glGetShaderInfoLog(vsHandle, info_len, NULL, info_log);
            Log("Error compiling Transform Feedback shader %s\n", filename);
            Log(info_log);
        }
        return 0;
    }

    glAttachShader(programHandle, vsHandle);

    static const char* varyings[] = { "outCompute0", "outCompute1", "outCompute2", "outCompute3", 
        /*"outCompute4", "outCompute5", "outCompute6", "outCompute7", 
        "outCompute8", "outCompute9", "outCompute10", "outCompute11", 
        "outCompute12", "outCompute13", "outCompute14", "outCompute15"*/ };
    glTransformFeedbackVaryings(programHandle, sizeof(varyings) / sizeof(const char*), varyings, GL_INTERLEAVED_ATTRIBS);


    glLinkProgram(programHandle);

    return programHandle;
}

void DebugLogText(const char *szText);

int Log(const char *szFormat, ...)
{
    va_list ptr_arg;
    va_start(ptr_arg, szFormat);

    static char buf[10240];
    vsprintf(buf, szFormat, ptr_arg);

    static FILE *fp = fopen("log.txt", "wt");
    if (fp)
    {
        fprintf(fp, buf);
        fflush(fp);
    }
    DebugLogText(buf);
    va_end(ptr_arg);
    return 0;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//matrix will receive the calculated perspective matrix.
//You would have to upload to your shader
// or use glLoadMatrixf if you aren't using shaders.
void Mat4x4::glhPerspectivef2(float fovyInDegrees, float aspectRatio,
    float znear, float zfar)
{
    float ymax, xmax;
    ymax = znear * tanf(fovyInDegrees * 3.14159f / 360.0f);
    xmax = ymax * aspectRatio;
    glhFrustumf2(-xmax, xmax, -ymax, ymax, znear, zfar);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Mat4x4::glhFrustumf2(float left, float right, float bottom, float top,
    float znear, float zfar)
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

void Mat4x4::lookAtRH(const Vec4 &eye, const Vec4 &at, const Vec4 &up)
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


void Mat4x4::lookAtLH(const Vec4 &eye, const Vec4 &at, const Vec4 &up)
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

void Mat4x4::LookAt(const Vec4 &eye, const Vec4 &at, const Vec4 &up)
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
    m[0][0] = 1.0f / (aspect * tanf(fovy*0.5f));
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[0][3] = 0.0f;

    m[1][0] = 0.0f;
    m[1][1] = 1.0f / tanf(fovy*0.5f);
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

void TagTime(const char *tagInfo)
{
    static uint64_t lastTime = -1;
    if (lastTime == -1)
    {
        lastTime = SDL_GetPerformanceCounter();
        Log("%s\n", tagInfo);
        return;
    }
    uint64_t t = SDL_GetPerformanceCounter();

    double v = double(t - lastTime) / double(SDL_GetPerformanceFrequency());
    Log("%s : %5.3f s\n", tagInfo, float(v));
    lastTime = t;
}

void DiscoverFiles(const char *extension, const char *directory, std::vector<std::string>& files)
{
    tinydir_dir dir;
    tinydir_open(&dir, directory);

    while (dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&dir, &file);

        if (!file.is_dir && !strcmp(file.extension, extension))
        {
            files.push_back(std::string(directory) + file.name);
        }

        tinydir_next(&dir);
    }

    tinydir_close(&dir);
}

void IMessageBox(const char *text, const char *title)
{
    MessageBoxA(NULL, text, title, MB_OK);
}