#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <Scene.h>

namespace GLSLPathTracer
{
    void LoadModel(Scene *scene, std::string filename, float materialId);
    bool LoadScene(Scene *scene, const char* filename);
}
