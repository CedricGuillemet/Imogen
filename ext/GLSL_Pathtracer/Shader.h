#pragma once

// third-party libraries
#include <GL/gl3w.h>
#include <string>

namespace GLSLPathTracer
{
    class Shader
    {
    private:
        GLuint _object;
    public:
        Shader(const std::string& filePath, GLuint shaderType);
        GLuint object() const;
    };
}