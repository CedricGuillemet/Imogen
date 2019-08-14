#pragma once

namespace GLSLPathTracer
{
    class Program;

    class Quad
    {
    public:
        Quad();
        void Draw(Program *);
    private:
        unsigned int vao, vbo;
    };
}