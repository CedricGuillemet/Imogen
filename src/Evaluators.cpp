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
#include <SDL.h>
#include <GL/gl3w.h> // Initialize with gl3wInit()
#include "Evaluators.h"
#include "EvaluationStages.h"
#include "nfd.h"
#include "Bitmap.h"
#include "EvaluationContext.h"
#include "TaskScheduler.h"
#include <vector>
#include <map>
#include <string>
#include "Scene.h"
#include "Loader.h"
#include "TiledRenderer.h"
#include "ProgressiveRenderer.h"
#include "GPUBVH.h"
#include "Camera.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "GraphControler.h"

Evaluators gEvaluators;
extern enki::TaskScheduler g_TS;


struct EValuationFunction
{
    const char* szFunctionName;
    void* function;
};

static const EValuationFunction evaluationFunctions[] = {
    {"Log", (void*)Log},
    {"log2", (void*)static_cast<float (*)(float)>(log2)},
    {"ReadImage", (void*)EvaluationAPI::Read},
    {"WriteImage", (void*)EvaluationAPI::Write},
    {"GetEvaluationImage", (void*)EvaluationAPI::GetEvaluationImage},
    {"SetEvaluationImage", (void*)EvaluationAPI::SetEvaluationImage},
    {"SetEvaluationImageCube", (void*)EvaluationAPI::SetEvaluationImageCube},
    {"AllocateImage", (void*)EvaluationAPI::AllocateImage},
    {"FreeImage", (void*)Image::Free},
    {"SetThumbnailImage", (void*)EvaluationAPI::SetThumbnailImage},
    {"Evaluate", (void*)EvaluationAPI::Evaluate},
    {"SetBlendingMode", (void*)EvaluationAPI::SetBlendingMode},
    {"EnableDepthBuffer", (void*)EvaluationAPI::EnableDepthBuffer},
    {"EnableFrameClear", (void*)EvaluationAPI::EnableFrameClear},
    {"SetVertexSpace", (void*)EvaluationAPI::SetVertexSpace},

    {"GetEvaluationSize", (void*)EvaluationAPI::GetEvaluationSize},
    {"SetEvaluationSize", (void*)EvaluationAPI::SetEvaluationSize},
    {"SetEvaluationCubeSize", (void*)EvaluationAPI::SetEvaluationCubeSize},
    {"AllocateComputeBuffer", (void*)EvaluationAPI::AllocateComputeBuffer},
    {"SetProcessing", (void*)EvaluationAPI::SetProcessing},
    {"Job", (void*)EvaluationAPI::Job},
    {"JobMain", (void*)EvaluationAPI::JobMain},
    {"memmove", memmove},
    {"strcpy", strcpy},
    {"strlen", strlen},
    {"fabsf", fabsf},
    {"strcmp", strcmp},
    {"LoadSVG", (void*)Image::LoadSVG},
    {"LoadScene", (void*)EvaluationAPI::LoadScene},
    {"SetEvaluationScene", (void*)EvaluationAPI::SetEvaluationScene},
    {"GetEvaluationScene", (void*)EvaluationAPI::GetEvaluationScene},
    {"SetEvaluationRTScene", (void*)EvaluationAPI::SetEvaluationRTScene},
    {"GetEvaluationRTScene", (void*)EvaluationAPI::GetEvaluationRTScene},
    {"GetEvaluationSceneName", (void*)EvaluationAPI::GetEvaluationSceneName},
    {"GetEvaluationRenderer", (void*)EvaluationAPI::GetEvaluationRenderer},
    {"OverrideInput", (void*)EvaluationAPI::OverrideInput},
    {"InitRenderer", (void*)EvaluationAPI::InitRenderer},
    {"UpdateRenderer", (void*)EvaluationAPI::UpdateRenderer},
    {"ReadGLTF", (void*)EvaluationAPI::ReadGLTF},
};

static void libtccErrorFunc(void* opaque, const char* msg)
{
    Log(msg);
    Log("\n");
}

void LogPython(const std::string& str)
{
    Log(str.c_str());
}

PYBIND11_MAKE_OPAQUE(Image);

struct PyGraph
{
    Material* mGraph;
};

struct PyNode
{
    Material* mGraph;
    MaterialNode* mNode;
    int mNodeIndex;
};
#include "imHotKey.h"
extern std::vector<ImHotKey::HotKey> mHotkeys;


void RenderImogenFrame();
void GraphEditorUpdateScrolling(GraphEditorDelegate* model);

PYBIND11_EMBEDDED_MODULE(Imogen, m)
{
    pybind11::class_<Image>(m, "Image");

    m.def("Render", []() { RenderImogenFrame(); });
    m.def("CaptureScreen", [](const std::string& filename, const std::string& content) {
        extern std::map<std::string, ImRect> interfacesRect;
        ImRect rc = interfacesRect[content];
        SaveCapture(filename, int(rc.Min.x), int(rc.Min.y), int(rc.GetWidth()), int(rc.GetHeight()));
    });
    m.def("SetSynchronousEvaluation", [](bool synchronous) {
        Imogen::instance->GetNodeGraphControler()->mEditingContext.SetSynchronous(synchronous);
    });
    m.def("NewGraph", [](const std::string& graphName) { Imogen::instance->NewMaterial(graphName); });
    m.def("AddNode", [](const std::string& nodeType) -> int { return Imogen::instance->AddNode(nodeType); });
    m.def("SetParameter", [](int nodeIndex, const std::string& paramName, const std::string& value) {
        Imogen::instance->GetNodeGraphControler()->mModel.BeginTransaction(false);
        Imogen::instance->GetNodeGraphControler()->mModel.SetParameter(nodeIndex, paramName, value);
        Imogen::instance->GetNodeGraphControler()->mModel.EndTransaction();
    });
    m.def("Connect", [](int nodeSource, int slotSource, int nodeDestination, int slotDestination) {
        Imogen::instance->GetNodeGraphControler()->mModel.BeginTransaction(false);
        Imogen::instance->GetNodeGraphControler()
            ->mModel.AddLink(nodeSource, slotSource, nodeDestination, slotDestination);
        Imogen::instance->GetNodeGraphControler()->mModel.EndTransaction();
    });
    m.def("AutoLayout", []() {
        //NodeGraphUpdateEvaluationOrder(&Imogen::instance->GetNodeGraphControler()->mModel,
        //                               Imogen::instance->GetNodeGraphControler());
        Imogen::instance->GetNodeGraphControler()->mModel.NodeGraphLayout(Imogen::instance->GetNodeGraphControler()->mEvaluationStages.GetForwardEvaluationOrder());
        GraphEditorUpdateScrolling(Imogen::instance->GetNodeGraphControler());
    });
    m.def("DeleteGraph", []() { Imogen::instance->DeleteCurrentMaterial(); });


    m.def("GetMetaNodes", []() {
        auto d = pybind11::list();

        for (auto& node : gMetaNodes)
        {
            auto n = pybind11::dict();
            d.append(n);
            n["name"] = node.mName;
            n["description"] = node.mDescription;
            if (node.mCategory >= 0 && node.mCategory < MetaNode::mCategories.size())
            {
                n["category"] = MetaNode::mCategories[node.mCategory];
            }

            if (!node.mParams.empty())
            {
                auto paramdict = pybind11::list();
                n["parameters"] = paramdict;
                for (auto& param : node.mParams)
                {
                    auto p = pybind11::dict();
                    p["name"] = param.mName;
                    p["type"] = pybind11::int_(int(param.mType));
                    p["typeString"] = GetParameterTypeName(param.mType);
                    p["description"] = param.mDescription;
                    if (param.mType == Con_Enum)
                    {
                        auto e = pybind11::list();
                        p["enum"] = e;

                        char* pch = strtok((char*)param.mEnumList.c_str(), "|");
                        while (pch != NULL)
                        {
                            e.append(std::string(pch));
                            pch = strtok(NULL, "|");
                        }
                    }
                    paramdict.append(p);
                }
            }
        }

        return d;
    });

    m.def("GetHotKeys", []() {
        auto d = pybind11::list();

        for (auto& hotkey : mHotkeys)
        {
            auto h = pybind11::dict();
            d.append(h);
            h["name"] = hotkey.functionName;
            h["description"] = hotkey.functionLib;
            static char combo[512];
            ImHotKey::GetHotKeyLib(hotkey.functionKeys, combo, sizeof(combo));
            h["keys"] = std::string(combo);
        }
        return d;
    });
    auto graph = pybind11::class_<PyGraph>(m, "Graph");
    graph.def("GetEvaluationList", [](PyGraph& pyGraph) {
        auto d = pybind11::list();

        for (int index = 0; index < int(pyGraph.mGraph->mMaterialNodes.size()); index++)
        {
            auto& node = pyGraph.mGraph->mMaterialNodes[index];
            d.append(new PyNode{pyGraph.mGraph, &node, index});
        }
        return d;
    });
    graph.def("Build", [](PyGraph& pyGraph) {
        extern Builder* builder;
        Material* material = pyGraph.mGraph;
        builder->Add(material);
    });
    auto node = pybind11::class_<PyNode>(m, "Node");
    node.def("GetType", [](PyNode& node) {
        std::string& s = node.mNode->mTypeName;
        if (!s.length())
            s = std::string("EmptyNode");
        return s;
    });
    node.def("GetInputs", [](PyNode& node) {
        //
        auto d = pybind11::list();
        if (node.mNode->mType == 0xFFFFFFFF)
            return d;

        MetaNode& metaNode = gMetaNodes[node.mNode->mType];

        for (auto& con : node.mGraph->mMaterialConnections)
        {
            if (con.mOutputNodeIndex == node.mNodeIndex)
            {
                auto e = pybind11::dict();
                d.append(e);

                e["nodeIndex"] = pybind11::int_(con.mInputNodeIndex);
                e["name"] = metaNode.mInputs[con.mOutputSlotIndex].mName;
            }
        }
        return d;
    });
    node.def("GetParameters", [](PyNode& node) {
        // name, type, value
        auto d = pybind11::list();
        if (node.mNode->mType == 0xFFFFFFFF)
            return d;
        MetaNode& metaNode = gMetaNodes[node.mNode->mType];

        for (uint32_t index = 0; index < metaNode.mParams.size(); index++)
        {
            auto& param = metaNode.mParams[index];
            auto e = pybind11::dict();
            d.append(e);
            e["name"] = param.mName;
            e["type"] = pybind11::int_(int(param.mType));

            size_t parameterOffset = GetParameterOffset(node.mNode->mType, index);
            if (parameterOffset >= node.mNode->mParameters.size())
            {
                e["value"] = std::string("");
                continue;
            }

            unsigned char* ptr = &node.mNode->mParameters[parameterOffset];
            float* ptrf = (float*)ptr;
            int* ptri = (int*)ptr;
            char tmps[512];
            switch (param.mType)
            {
                case Con_Float:
                    e["value"] = pybind11::float_(ptrf[0]);
                    break;
                case Con_Float2:
                    sprintf(tmps, "%f,%f", ptrf[0], ptrf[1]);
                    e["value"] = std::string(tmps);
                    break;
                case Con_Float3:
                    sprintf(tmps, "%f,%f,%f", ptrf[0], ptrf[1], ptrf[2]);
                    e["value"] = std::string(tmps);
                    break;
                case Con_Float4:
                    sprintf(tmps, "%f,%f,%f,%f", ptrf[0], ptrf[1], ptrf[2], ptrf[3]);
                    e["value"] = std::string(tmps);
                    break;
                case Con_Color4:
                    sprintf(tmps, "%f,%f,%f,%f", ptrf[0], ptrf[1], ptrf[2], ptrf[3]);
                    e["value"] = std::string(tmps);
                    break;
                case Con_Int:
                    e["value"] = pybind11::int_(ptri[0]);
                    break;
                case Con_Int2:
                    sprintf(tmps, "%d,%d", ptri[0], ptri[1]);
                    e["value"] = std::string(tmps);
                    break;
                case Con_Ramp:
                    e["value"] = std::string("N/A");
                    break;
                case Con_Angle:
                    e["value"] = pybind11::float_(ptrf[0]);
                    break;
                case Con_Angle2:
                    sprintf(tmps, "%f,%f", ptrf[0], ptrf[1]);
                    e["value"] = std::string(tmps);
                    break;
                case Con_Angle3:
                    sprintf(tmps, "%f,%f,%f", ptrf[0], ptrf[1], ptrf[2]);
                    e["value"] = std::string(tmps);
                    break;
                case Con_Angle4:
                    sprintf(tmps, "%f,%f,%f,%f", ptrf[0], ptrf[1], ptrf[2], ptrf[3]);
                    e["value"] = std::string(tmps);
                    break;
                case Con_Enum:
                    e["value"] = pybind11::int_(ptri[0]);
                    break;
                case Con_Structure:
                    e["value"] = std::string("N/A");
                    break;
                case Con_FilenameRead:
                case Con_FilenameWrite:
                    e["value"] = std::string((char*)ptr, strlen((char*)ptr));
                    break;
                case Con_ForceEvaluate:
                    e["value"] = std::string("N/A");
                    break;
                case Con_Bool:
                    e["value"] = pybind11::bool_(ptr[0] != 0);
                    break;
                case Con_Ramp4:
                case Con_Camera:
                    e["value"] = std::string("N/A");
                    break;
            }
        }
        return d;
    });
    m.def("RegisterPlugin", [](std::string& name, std::string command) {
        mRegisteredPlugins.push_back({name, command});
        Log("Plugin registered : %s \n", name.c_str());
    });
    m.def("FileDialogRead", []() {
        nfdchar_t* outPath = NULL;
        nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);

        if (result == NFD_OKAY)
        {
            std::string res = outPath;
            free(outPath);
            return res;
        }
        return std::string();
    });
    m.def("FileDialogWrite", []() {
        nfdchar_t* outPath = NULL;
        nfdresult_t result = NFD_SaveDialog(NULL, NULL, &outPath);

        if (result == NFD_OKAY)
        {
            std::string res = outPath;
            free(outPath);
            return res;
        }
        return std::string();
    });
    m.def("Log", LogPython);
    m.def("log2", static_cast<float (*)(float)>(log2));
    m.def("ReadImage", Image::Read);
    m.def("WriteImage", Image::Write);
    m.def("GetEvaluationImage", EvaluationAPI::GetEvaluationImage);
    m.def("SetEvaluationImage", EvaluationAPI::SetEvaluationImage);
    m.def("SetEvaluationImageCube", EvaluationAPI::SetEvaluationImageCube);
    m.def("AllocateImage", EvaluationAPI::AllocateImage);
    m.def("FreeImage", Image::Free);
    m.def("SetThumbnailImage", EvaluationAPI::SetThumbnailImage);
    m.def("Evaluate", EvaluationAPI::Evaluate);
    m.def("SetBlendingMode", EvaluationAPI::SetBlendingMode);
    m.def("GetEvaluationSize", EvaluationAPI::GetEvaluationSize);
    m.def("SetEvaluationSize", EvaluationAPI::SetEvaluationSize);
    m.def("SetEvaluationCubeSize", EvaluationAPI::SetEvaluationCubeSize);
    m.def("SetProcessing", EvaluationAPI::SetProcessing);
    /*
    m.def("Job", EvaluationStages::Job );
    m.def("JobMain", EvaluationStages::JobMain );
    */
    m.def("GetLibraryGraphs", []() {
        auto d = pybind11::list();
        for (auto& graph : library.mMaterials)
        {
            const std::string& s = graph.mName;
            d.append(graph.mName);
        }
        return d;
    });
    m.def("GetGraph", [](const std::string& graphName) -> PyGraph* {
        for (auto& graph : library.mMaterials)
        {
            if (graph.mName == graphName)
            {
                return new PyGraph{&graph};
            }
        }
        return nullptr;
    });
    /*
    m.def("accessor_api", []() {
        auto d = pybind11::dict();

        d["target"] = 10;

        auto l = pybind11::list();
        l.append(5);
        l.append(-1);
        l.append(-1);
        d["inputs"] = l;

        return d;
    });
    */

    /*
    m.def("GetImage", []() {
        auto i = new Image;
        //pImage i;
        //i.a = 14;
        //printf("new img %p \n", &i);
        return i;
    });

    m.def("SaveImage", [](Image image) {
        //printf("Saving image %d\n", image.a);
        //printf("save img %p \n", image);
    });
    */
}
std::string Evaluators::GetEvaluator(const std::string& filename)
{
    return mEvaluatorScripts[filename].mText;
}

void Evaluators::SetEvaluators(const std::vector<EvaluatorFile>& evaluatorfilenames)
{
    ClearEvaluators();

    mEvaluatorPerNodeType.clear();
    mEvaluatorPerNodeType.resize(evaluatorfilenames.size(), Evaluator());

    // GLSL
    for (auto& file : evaluatorfilenames)
    {
        if (file.mEvaluatorType != EVALUATOR_GLSL && file.mEvaluatorType != EVALUATOR_GLSLCOMPUTE)
            continue;
        const std::string filename = file.mFilename;

        std::ifstream t(file.mDirectory + filename);
        if (t.good())
        {
            std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
            if (mEvaluatorScripts.find(filename) == mEvaluatorScripts.end())
                mEvaluatorScripts[filename] = EvaluatorScript(str);
            else
                mEvaluatorScripts[filename].mText = str;
        }
    }

    // GLSL
    std::string baseShader = mEvaluatorScripts["Shader.glsl"].mText;
    for (auto& file : evaluatorfilenames)
    {
        if (file.mEvaluatorType != EVALUATOR_GLSL)
            continue;
        const std::string filename = file.mFilename;

        if (filename == "Shader.glsl")
            continue;

        EvaluatorScript& shader = mEvaluatorScripts[filename];
        std::string shaderText = ReplaceAll(baseShader, "__NODE__", shader.mText);
        std::string nodeName = ReplaceAll(filename, ".glsl", "");
        shaderText = ReplaceAll(shaderText, "__FUNCTION__", nodeName + "()");

        unsigned int program = LoadShader(shaderText, filename.c_str());

        int parameterBlockIndex = glGetUniformBlockIndex(program, (nodeName + "Block").c_str());
        if (parameterBlockIndex != -1)
            glUniformBlockBinding(program, parameterBlockIndex, 1);

        parameterBlockIndex = glGetUniformBlockIndex(program, "EvaluationBlock");
        if (parameterBlockIndex != -1)
            glUniformBlockBinding(program, parameterBlockIndex, 2);
        shader.mProgram = program;
        if (shader.mType != -1)
            mEvaluatorPerNodeType[shader.mType].mGLSLProgram = program;
    }


    TagTime("GLSL init");

    // GLSL compute
    for (auto& file : evaluatorfilenames)
    {
        if (file.mEvaluatorType != EVALUATOR_GLSLCOMPUTE)
            continue;
        const std::string filename = file.mFilename;

        EvaluatorScript& shader = mEvaluatorScripts[filename];
        // std::string shaderText = ReplaceAll(baseShader, "__NODE__", shader.mText);
        std::string nodeName = ReplaceAll(filename, ".glslc", "");
        // shaderText = ReplaceAll(shaderText, "__FUNCTION__", nodeName + "()");

        unsigned int program = 0;
        if (nodeName == filename)
        {
            // glsl in compute directory
            nodeName = ReplaceAll(filename, ".glsl", "");
            program = LoadShader(shader.mText, filename.c_str());
        }
        else
        {
            program = LoadShaderTransformFeedback(shader.mText, filename.c_str());
        }

        int parameterBlockIndex = glGetUniformBlockIndex(program, (nodeName + "Block").c_str());
        if (parameterBlockIndex != -1)
            glUniformBlockBinding(program, parameterBlockIndex, 1);

        parameterBlockIndex = glGetUniformBlockIndex(program, "EvaluationBlock");
        if (parameterBlockIndex != -1)
            glUniformBlockBinding(program, parameterBlockIndex, 2);
        shader.mProgram = program;
        if (shader.mType != -1)
            mEvaluatorPerNodeType[shader.mType].mGLSLProgram = program;
    }
    TagTime("GLSL compute init");
    // C
    for (auto& file : evaluatorfilenames)
    {
        if (file.mEvaluatorType != EVALUATOR_C)
            continue;
        const std::string filename = file.mFilename;
        try
        {
            std::ifstream t(file.mDirectory + filename);
            if (!t.good())
            {
                Log("%s - Unable to load file.\n", filename.c_str());
                continue;
            }
            Log("%s\n", filename.c_str());
            std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
            if (mEvaluatorScripts.find(filename) == mEvaluatorScripts.end())
                mEvaluatorScripts[filename] = EvaluatorScript(str);
            else
                mEvaluatorScripts[filename].mText = str;

            EvaluatorScript& program = mEvaluatorScripts[filename];
            TCCState* s = tcc_new();

            int* noLib = (int*)s;
            noLib[2] = 1; // no stdlib

            tcc_set_error_func(s, 0, libtccErrorFunc);
            tcc_add_include_path(s, "Nodes/C/");
            tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

            if (tcc_compile_string(s, program.mText.c_str()) != 0)
            {
                Log("%s - Compilation error!\n", filename.c_str());
                continue;
            }

            for (auto& evaluationFunction : evaluationFunctions)
                tcc_add_symbol(s, evaluationFunction.szFunctionName, evaluationFunction.function);

            int size = tcc_relocate(s, NULL);
            if (size == -1)
            {
                Log("%s - Libtcc unable to relocate program!\n", filename.c_str());
                continue;
            }
            program.mMem = malloc(size);
            tcc_relocate(s, program.mMem);

            *(void**)(&program.mCFunction) = tcc_get_symbol(s, "main");
            if (!program.mCFunction)
            {
                Log("%s - No main function!\n", filename.c_str());
            }
            tcc_delete(s);

            if (program.mType != -1)
            {
                mEvaluatorPerNodeType[program.mType].mCFunction = program.mCFunction;
                mEvaluatorPerNodeType[program.mType].mMem = program.mMem;
            }
        }
        catch (...)
        {
            Log("Error at compiling %s", filename.c_str());
        }
    }
    TagTime("C init");

    for (auto& file : evaluatorfilenames)
    {
        if (file.mEvaluatorType != EVALUATOR_PYTHON)
            continue;
        const std::string filename = file.mFilename;
        std::string nodeName = ReplaceAll(filename, ".py", "");
        EvaluatorScript& shader = mEvaluatorScripts[filename];
        try
        {
            shader.mPyModule = pybind11::module::import("Nodes.Python.testnode");
            if (shader.mType != -1)
                mEvaluatorPerNodeType[shader.mType].mPyModule = shader.mPyModule;
        }
        catch (...)
        {
            Log("Python exception\n");
        }
    }

    TagTime("Python init");
}

void Evaluators::ClearEvaluators()
{
    // clear
    for (auto& program : mEvaluatorPerNodeType)
    {
        if (program.mGLSLProgram)
            glDeleteProgram(program.mGLSLProgram);
        if (program.mMem)
            free(program.mMem);
    }
}

int Evaluators::GetMask(size_t nodeType)
{
    const std::string& nodeName = gMetaNodes[nodeType].mName;
#ifdef _DEBUG
    mEvaluatorPerNodeType[nodeType].mName = nodeName;
#endif
    int mask = 0;
    auto iter = mEvaluatorScripts.find(nodeName + ".glsl");
    if (iter != mEvaluatorScripts.end())
    {
        mask |= EvaluationGLSL;
        iter->second.mType = int(nodeType);
        mEvaluatorPerNodeType[nodeType].mGLSLProgram = iter->second.mProgram;
    }
    iter = mEvaluatorScripts.find(nodeName + ".glslc");
    if (iter != mEvaluatorScripts.end())
    {
        mask |= EvaluationGLSLCompute;
        iter->second.mType = int(nodeType);
        mEvaluatorPerNodeType[nodeType].mGLSLProgram = iter->second.mProgram;
    }
    iter = mEvaluatorScripts.find(nodeName + ".c");
    if (iter != mEvaluatorScripts.end())
    {
        mask |= EvaluationC;
        iter->second.mType = int(nodeType);
        mEvaluatorPerNodeType[nodeType].mCFunction = iter->second.mCFunction;
        mEvaluatorPerNodeType[nodeType].mMem = iter->second.mMem;
    }
    iter = mEvaluatorScripts.find(nodeName + ".py");
    if (iter != mEvaluatorScripts.end())
    {
        mask |= EvaluationPython;
        iter->second.mType = int(nodeType);
        mEvaluatorPerNodeType[nodeType].mPyModule = iter->second.mPyModule;
    }
    return mask;
}

void Evaluators::InitPythonModules()
{
    mImogenModule = pybind11::module::import("Imogen");
    mImogenModule.dec_ref();
}

void Evaluators::InitPython()
{
    try
    {
        pybind11::initialize_interpreter(true); // start the interpreter and keep it alive
        gEvaluators.InitPythonModules();
        pybind11::exec(R"(
            import sys
            import Imogen
            class CatchImogenIO:
                def __init__(self):
                    pass
                def write(self, txt):
                    Imogen.Log(txt)
            catchImogenIO = CatchImogenIO()
            sys.stdout = catchImogenIO
            sys.stderr = catchImogenIO
            print("Python stdout, stderr catched.\n"))");
        pybind11::module::import("Plugins");
    }
    catch (std::exception e)
    {
        Log("InitPython Exception : %s\n", e.what());
    }
}

void Evaluators::ReloadPlugins()
{
    try
    {
        mRegisteredPlugins.clear();
        pybind11::exec(R"(
            import importlib
            importlib.reload(sys.modules["Plugins"])
            print("Python plugins reloaded.\n"))");
    }
    catch (std::exception e)
    {
        Log("Error at reloading Python modules. Exception : %s\n", e.what());
    }
}

void Evaluator::RunPython() const
{
    mPyModule.attr("main")(gEvaluators.mImogenModule.attr("accessor_api")());
}

namespace EvaluationAPI
{
    int SetEvaluationImageCube(EvaluationContext* evaluationContext, int target, Image* image, int cubeFace)
    {
        if (image->mNumFaces != 1)
        {
            return EVAL_ERR;
        }
        auto tgt = evaluationContext->GetRenderTarget(target);
        if (!tgt)
        {
            return EVAL_ERR;
        }

        tgt->InitCube(image->mWidth, image->mNumMips);

        Image::Upload(image, tgt->mGLTexID, cubeFace);
        evaluationContext->SetTargetDirty(target, Dirty::Parameter, true);
        return EVAL_OK;
    }

    int AllocateImage(Image* image)
    {
        return EVAL_OK;
    }

    int SetThumbnailImage(EvaluationContext* context, Image* image)
    {
        std::vector<unsigned char> pngImage;
        if (Image::EncodePng(image, pngImage) == EVAL_ERR)
            return EVAL_ERR;

        Material* material = library.Get(std::make_pair(0, context->GetMaterialUniqueId()));
        if (material)
        {
            material->mThumbnail = pngImage;
            material->mThumbnailTextureId = 0;
        }
        return EVAL_OK;
    }

    void SetBlendingMode(EvaluationContext* evaluationContext, int target, int blendSrc, int blendDst)
    {
        EvaluationContext::Evaluation& evaluation = evaluationContext->mEvaluations[target];

        evaluation.mBlendingSrc = blendSrc;
        evaluation.mBlendingDst = blendDst;
    }

    void EnableDepthBuffer(EvaluationContext* evaluationContext, int target, int enable)
    {
        EvaluationContext::Evaluation& evaluation = evaluationContext->mEvaluations[target];
        evaluation.mbDepthBuffer = enable != 0;
    }

    void EnableFrameClear(EvaluationContext* evaluationContext, int target, int enable)
    {
        EvaluationContext::Evaluation& evaluation = evaluationContext->mEvaluations[target];
        evaluation.mbClearBuffer = enable != 0;
    }

    void SetVertexSpace(EvaluationContext* evaluationContext, int target, int vertexSpace)
    {
        EvaluationContext::Evaluation& evaluation = evaluationContext->mEvaluations[target];
        evaluation.mVertexSpace = vertexSpace;
    }

    int GetEvaluationSize(const EvaluationContext* evaluationContext, int target, int* imageWidth, int* imageHeight)
    {
        if (target < 0 || target >= evaluationContext->mEvaluationStages.mStages.size())
            return EVAL_ERR;
        auto renderTarget = evaluationContext->GetRenderTarget(target);
        if (!renderTarget)
            return EVAL_ERR;
        *imageWidth = renderTarget->mImage->mWidth;
        *imageHeight = renderTarget->mImage->mHeight;
        return EVAL_OK;
    }

    int SetEvaluationSize(EvaluationContext* evaluationContext, int target, int imageWidth, int imageHeight)
    {
        if (target < 0 || target >= evaluationContext->mEvaluationStages.mStages.size())
            return EVAL_ERR;
        auto renderTarget = evaluationContext->GetRenderTarget(target);
        if (!renderTarget)
            return EVAL_ERR;
        // if (gCurrentContext->GetEvaluationInfo().uiPass)
        //    return EVAL_OK;
        renderTarget->InitBuffer(
            imageWidth, imageHeight, evaluationContext->mEvaluations[target].mbDepthBuffer);
        return EVAL_OK;
    }

    int SetEvaluationCubeSize(EvaluationContext* evaluationContext, int target, int faceWidth, int mipmapCount)
    {
        if (target < 0 || target >= evaluationContext->mEvaluationStages.mStages.size())
            return EVAL_ERR;

        auto renderTarget = evaluationContext->GetRenderTarget(target);
        if (!renderTarget)
            return EVAL_ERR;
        renderTarget->InitCube(faceWidth, mipmapCount);
        return EVAL_OK;
    }

    std::map<std::string, std::weak_ptr<Scene>> gSceneCache;

    int SetEvaluationRTScene(EvaluationContext* evaluationContext, int target, void* scene)
    {
        evaluationContext->mEvaluationStages.mStages[target].mScene = scene;
        return EVAL_OK;
    }

    int GetEvaluationRTScene(EvaluationContext* evaluationContext, int target, void** scene)
    {
        *scene = evaluationContext->mEvaluationStages.mStages[target].mScene;
        return EVAL_OK;
    }


    int SetEvaluationScene(EvaluationContext* evaluationContext, int target, void* scene)
    {
        const std::string& name = ((Scene*)scene)->mName;
        auto& stage = evaluationContext->mEvaluationStages.mStages[target];
        auto iter = gSceneCache.find(name);
        if (iter == gSceneCache.end() || iter->second.expired())
        {
            stage.mGScene = std::shared_ptr<Scene>((Scene*)scene);
            gSceneCache.insert(std::make_pair(name, stage.mGScene));
            evaluationContext->SetTargetDirty(target, Dirty::Input);
            return EVAL_OK;
        }

        if (stage.mGScene != iter->second.lock())
        {
            stage.mGScene = iter->second.lock();
            evaluationContext->SetTargetDirty(target, Dirty::Input);
        }
        return EVAL_OK;
    }

    int GetEvaluationScene(EvaluationContext* evaluationContext, int target, void** scene)
    {
        if (target >= 0 && target < evaluationContext->mEvaluationStages.mStages.size())
        {
            *scene = evaluationContext->mEvaluationStages.mStages[target].mGScene.get();
            return EVAL_OK;
        }
        return EVAL_ERR;
    }

    const char* GetEvaluationSceneName(EvaluationContext* evaluationContext, int target)
    {
        void* scene;
        if (GetEvaluationScene(evaluationContext, target, &scene) == EVAL_OK && scene)
        {
            const std::string& name = ((Scene*)scene)->mName;
            return name.c_str();
        }
        return "";
    }

    int GetEvaluationRenderer(EvaluationContext* evaluationContext, int target, void** renderer)
    {
        *renderer = evaluationContext->mEvaluationStages.mStages[target].renderer;
        return EVAL_OK;
    }


    int OverrideInput(EvaluationContext* evaluationContext, int target, int inputIndex, int newInputTarget)
    {
        evaluationContext->mEvaluationStages.mInputs[target].mOverrideInputs[inputIndex] = newInputTarget;
        return EVAL_OK;
    }

    int GetEvaluationImage(EvaluationContext* evaluationContext, int target, Image* image)
    {
        if (target == -1 || target >= evaluationContext->mEvaluationStages.mStages.size())
        {
            return EVAL_ERR;
        }

        auto tgt = evaluationContext->GetRenderTarget(target);
        if (!tgt)
        {
            return EVAL_ERR;
        }

        // compute total size
        auto img = tgt->mImage;
        unsigned int texelSize = textureFormatSize[img->mFormat];
        unsigned int texelFormat = glInternalFormats[img->mFormat];
        uint32_t size = 0; // img.mNumFaces * img.mWidth * img.mHeight * texelSize;
        for (int i = 0; i < img->mNumMips; i++)
            size += img->mNumFaces * (img->mWidth >> i) * (img->mHeight >> i) * texelSize;

        image->Allocate(size);
        image->mWidth = img->mWidth;
        image->mHeight = img->mHeight;
        image->mNumMips = img->mNumMips;
        image->mFormat = img->mFormat;
        image->mNumFaces = img->mNumFaces;

        unsigned char* ptr = image->GetBits();
        if (img->mNumFaces == 1)
        {
            glBindTexture(GL_TEXTURE_2D, tgt->mGLTexID);
            for (int i = 0; i < img->mNumMips; i++)
            {
                glGetTexImage(GL_TEXTURE_2D, i, texelFormat, GL_UNSIGNED_BYTE, ptr);
                ptr += (img->mWidth >> i) * (img->mHeight >> i) * texelSize;
            }
        }
        else
        {
            glBindTexture(GL_TEXTURE_CUBE_MAP, tgt->mGLTexID);
            for (int cube = 0; cube < img->mNumFaces; cube++)
            {
                for (int i = 0; i < img->mNumMips; i++)
                {
                    glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + cube, i, texelFormat, GL_UNSIGNED_BYTE, ptr);
                    ptr += (img->mWidth >> i) * (img->mHeight >> i) * texelSize;
                }
            }
        }
        return EVAL_OK;
    }

    int SetEvaluationImage(EvaluationContext* evaluationContext, int target, Image* image)
    {
        EvaluationStage& stage = evaluationContext->mEvaluationStages.mStages[target];
        auto tgt = evaluationContext->GetRenderTarget(target);
        if (!tgt)
            return EVAL_ERR;
        unsigned int texelSize = textureFormatSize[image->mFormat];
        unsigned int inputFormat = glInputFormats[image->mFormat];
        unsigned int internalFormat = glInternalFormats[image->mFormat];
        unsigned char* ptr = image->GetBits();
        if (image->mNumFaces == 1)
        {
            tgt->InitBuffer(image->mWidth, image->mHeight, evaluationContext->mEvaluations[target].mbDepthBuffer);

            glBindTexture(GL_TEXTURE_2D, tgt->mGLTexID);

            for (int i = 0; i < image->mNumMips; i++)
            {
                glTexImage2D(GL_TEXTURE_2D,
                             i,
                             internalFormat,
                             image->mWidth >> i,
                             image->mHeight >> i,
                             0,
                             inputFormat,
                             GL_UNSIGNED_BYTE,
                             ptr);
                ptr += (image->mWidth >> i) * (image->mHeight >> i) * texelSize;
            }

            if (image->mNumMips > 1)
                TexParam(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_2D);
            else
                TexParam(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_2D);
        }
        else
        {
            tgt->InitCube(image->mWidth, image->mNumMips);
            glBindTexture(GL_TEXTURE_CUBE_MAP, tgt->mGLTexID);

            for (int face = 0; face < image->mNumFaces; face++)
            {
                for (int i = 0; i < image->mNumMips; i++)
                {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                                 i,
                                 internalFormat,
                                 image->mWidth >> i,
                                 image->mWidth >> i,
                                 0,
                                 inputFormat,
                                 GL_UNSIGNED_BYTE,
                                 ptr);
                    ptr += (image->mWidth >> i) * (image->mWidth >> i) * texelSize;
                }
            }

            if (image->mNumMips > 1)
                TexParam(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_CUBE_MAP);
            else
                TexParam(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_CUBE_MAP);
        }
        if (stage.mDecoder.get() != (FFMPEGCodec::Decoder*)image->mDecoder)
            stage.mDecoder = std::shared_ptr<FFMPEGCodec::Decoder>((FFMPEGCodec::Decoder*)image->mDecoder);
        evaluationContext->SetTargetDirty(target, Dirty::Input, true);
        return EVAL_OK;
    }

    int LoadScene(const char* filename, void** pscene)
    {
        // todo: make a real good cache system
        static std::map<std::string, GLSLPathTracer::Scene*> cachedScenes;
        std::string sFilename(filename);
        auto iter = cachedScenes.find(sFilename);
        if (iter != cachedScenes.end())
        {
            *pscene = iter->second;
            return EVAL_OK;
        }

        GLSLPathTracer::Scene* scene = GLSLPathTracer::LoadScene(sFilename);
        if (!scene)
        {
            Log("Unable to load scene\n");
            return EVAL_ERR;
        }
        cachedScenes.insert(std::make_pair(sFilename, scene));
        *pscene = scene;

        Log("Scene Loaded\n\n");

        scene->buildBVH();

        // --------Print info on memory usage ------------- //

        Log("Triangles: %d\n", scene->triangleIndices.size());
        Log("Triangle Indices: %d\n", scene->gpuBVH->bvhTriangleIndices.size());
        Log("Vertices: %d\n", scene->vertexData.size());

        long long scene_data_bytes = sizeof(GLSLPathTracer::GPUBVHNode) * scene->gpuBVH->bvh->getNumNodes() +
                                     sizeof(GLSLPathTracer::TriangleData) * scene->gpuBVH->bvhTriangleIndices.size() +
                                     sizeof(GLSLPathTracer::VertexData) * scene->vertexData.size() +
                                     sizeof(GLSLPathTracer::NormalTexData) * scene->normalTexData.size() +
                                     sizeof(GLSLPathTracer::MaterialData) * scene->materialData.size() +
                                     sizeof(GLSLPathTracer::LightData) * scene->lightData.size();

        Log("GPU Memory used for BVH and scene data: %d MB\n", scene_data_bytes / 1048576);

        long long tex_data_bytes = scene->texData.albedoTextureSize.x * scene->texData.albedoTextureSize.y *
                                       scene->texData.albedoTexCount * 3 +
                                   scene->texData.metallicRoughnessTextureSize.x *
                                       scene->texData.metallicRoughnessTextureSize.y *
                                       scene->texData.metallicRoughnessTexCount * 3 +
                                   scene->texData.normalTextureSize.x * scene->texData.normalTextureSize.y *
                                       scene->texData.normalTexCount * 3 +
                                   scene->hdrLoaderRes.width * scene->hdrLoaderRes.height * sizeof(GL_FLOAT) * 3;

        Log("GPU Memory used for Textures: %d MB\n", tex_data_bytes / 1048576);

        Log("Total GPU Memory used: %d MB\n", (scene_data_bytes + tex_data_bytes) / 1048576);

        return EVAL_OK;
    }

    int InitRenderer(EvaluationContext* evaluationContext, int target, int mode, void* scene)
    {
        GLSLPathTracer::Scene* rdscene = (GLSLPathTracer::Scene*)scene;
        evaluationContext->mEvaluationStages.mStages[target].mScene = scene;

        GLSLPathTracer::Renderer* currentRenderer =
            (GLSLPathTracer::Renderer*)evaluationContext->mEvaluationStages.mStages[target].renderer;
        if (!currentRenderer)
        {
            // auto renderer = new GLSLPathTracer::TiledRenderer(rdscene, "Stock/PathTracer/Tiled/");
            auto renderer = new GLSLPathTracer::ProgressiveRenderer(rdscene, "Stock/PathTracer/Progressive/");
            renderer->init();
            evaluationContext->mEvaluationStages.mStages[target].renderer = renderer;
        }
        return EVAL_OK;
    }

    int UpdateRenderer(EvaluationContext* evaluationContext, int target)
    {
        auto& eval = evaluationContext->mEvaluationStages;
        GLSLPathTracer::Renderer* renderer = (GLSLPathTracer::Renderer*)eval.mStages[target].renderer;
        GLSLPathTracer::Scene* rdscene = (GLSLPathTracer::Scene*)eval.mStages[target].mScene;

        const Camera* camera = GetCameraParameter(eval.mStages[target].mType, eval.mParameters[target]);
        if (camera)
        {
            Vec4 pos = camera->mPosition;
            Vec4 lk = camera->mPosition + camera->mDirection;
            GLSLPathTracer::Camera newCam(glm::vec3(pos.x, pos.y, pos.z), glm::vec3(lk.x, lk.y, lk.z), 90.f);
            newCam.updateCamera();
            *rdscene->camera = newCam;
        }

        renderer->update(0.0166f);
        auto tgt = evaluationContext->GetRenderTarget(target);
        renderer->render();

        tgt->BindAsTarget();
        renderer->present();

        float progress = renderer->getProgress();
        evaluationContext->StageSetProgress(target, progress);
        bool renderDone = progress >= 1.f - FLT_EPSILON;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);

        if (renderDone)
        {
            evaluationContext->StageSetProcessing(target, false);
            return EVAL_OK;
        }
        return EVAL_DIRTY;
    }

    void SetProcessing(EvaluationContext* context, int target, int processing)
    {
        context->StageSetProcessing(target, processing);
    }

    int AllocateComputeBuffer(EvaluationContext* context, int target, int elementCount, int elementSize)
    {
        context->AllocateComputeBuffer(target, elementCount, elementSize);
        return EVAL_OK;
    }

    typedef int (*jobFunction)(void*);

    struct CFunctionTaskSet : enki::ITaskSet
    {
        CFunctionTaskSet(jobFunction function, void* ptr, unsigned int size)
            : enki::ITaskSet(), mFunction(function), mBuffer(malloc(size))
        {
            memcpy(mBuffer, ptr, size);
        }
        virtual void ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum)
        {
            mFunction(mBuffer);
            free(mBuffer);
            delete this;
        }
        jobFunction mFunction;
        void* mBuffer;
    };

    struct CFunctionMainTask : enki::IPinnedTask
    {
        CFunctionMainTask(jobFunction function, void* ptr, unsigned int size)
            : enki::IPinnedTask(0) // set pinned thread to 0
            , mFunction(function)
            , mBuffer(malloc(size))
        {
            memcpy(mBuffer, ptr, size);
        }
        virtual void Execute()
        {
            mFunction(mBuffer);
            free(mBuffer);
            delete this;
        }
        jobFunction mFunction;
        void* mBuffer;
    };

    int Job(EvaluationContext* evaluationContext, int (*jobFunction)(void*), void* ptr, unsigned int size)
    {
        if (evaluationContext->IsSynchronous())
        {
            return jobFunction(ptr);
        }
        else
        {
            g_TS.AddTaskSetToPipe(new CFunctionTaskSet(jobFunction, ptr, size));
        }
        return EVAL_OK;
    }

    int JobMain(EvaluationContext* evaluationContext, int (*jobMainFunction)(void*), void* ptr, unsigned int size)
    {
        if (evaluationContext->IsSynchronous())
        {
            return jobMainFunction(ptr);
        }
        else
        {
            g_TS.AddPinnedTask(new CFunctionMainTask(jobMainFunction, ptr, size));
        }
        return EVAL_OK;
    }

    int Read(EvaluationContext* evaluationContext, const char* filename, Image* image)
    {
        if (Image::Read(filename, image) == EVAL_OK)
            return EVAL_OK;
        // try to load movie
        auto decoder = evaluationContext->mEvaluationStages.FindDecoder(filename);
        *image = Image::DecodeImage(decoder, evaluationContext->GetCurrentTime());
        if (!image->mWidth || !image->mHeight)
            return EVAL_ERR;
        return EVAL_OK;
    }

    int Write(EvaluationContext* evaluationContext, const char* filename, Image* image, int format, int quality)
    {
        if (format == 7)
        {
            FFMPEGCodec::Encoder* encoder =
                evaluationContext->GetEncoder(std::string(filename), image->mWidth, image->mHeight);
            std::string fn(filename);
            encoder->AddFrame(image->GetBits(), image->mWidth, image->mHeight);
            return EVAL_OK;
        }

        return Image::Write(filename, image, format, quality);
    }

    int Evaluate(EvaluationContext* evaluationContext, int target, int width, int height, Image* image)
    {
        EvaluationContext context(evaluationContext->mEvaluationStages, true, width, height);
        context.SetCurrentTime(evaluationContext->GetCurrentTime());
        // set all nodes as dirty so that evaluation (in build) will not bypass most nodes
        context.DirtyAll();
        while (context.RunBackward(target))
        {
            // processing... maybe good on next run
        }
        GetEvaluationImage(&context, target, image);
        return EVAL_OK;
    }

    inline char* ReadFile(const char* szFileName, int& bufSize)
    {
        FILE* fp = fopen(szFileName, "rb");
        if (fp)
        {
            fseek(fp, 0, SEEK_END);
            bufSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            char* buf = new char[bufSize];
            fread(buf, bufSize, 1, fp);
            fclose(fp);
            return buf;
        }
        return NULL;
    }

    int ReadGLTF(EvaluationContext* evaluationContext, const char* filename, Scene** scene)
    {
        std::string strFilename(filename);
        auto iter = gSceneCache.find(strFilename);
        if (iter != gSceneCache.end() && (!iter->second.expired()))
        {
            *scene = iter->second.lock().get();
            return EVAL_OK;
        }
        cgltf_options options = {0};
        cgltf_data* data = NULL;
        cgltf_result result = cgltf_parse_file(&options, filename, &data);
        if (result != cgltf_result_success)
            return EVAL_ERR;

        result = cgltf_load_buffers(&options, data, filename);
        if (result != cgltf_result_success)
            return EVAL_ERR;

        Scene* sc = new Scene;
        sc->mName = strFilename;
        // gSceneCache.insert(std::make_pair(strFilename, sc));
        // gScenePointerCache.insert(std::make_pair(sc.get(), sc));

        sc->mMeshes.resize(data->meshes_count);
        for (unsigned int i = 0; i < data->meshes_count; i++)
        {
            auto& gltfMesh = data->meshes[i];
            auto& mesh = sc->mMeshes[i];
            mesh.mPrimitives.resize(gltfMesh.primitives_count);
            // attributes
            for (unsigned int j = 0; j < gltfMesh.primitives_count; j++)
            {
                auto& gltfPrim = gltfMesh.primitives[j];
                auto& prim = mesh.mPrimitives[j];

                for (unsigned int k = 0; k < gltfPrim.attributes_count; k++)
                {
                    unsigned int format = 0;
                    auto attr = gltfPrim.attributes[k];
                    switch (attr.type)
                    {
                        case cgltf_attribute_type_position:
                            format = Scene::Mesh::Format::POS;
                            break;
                        case cgltf_attribute_type_normal:
                            format = Scene::Mesh::Format::NORM;
                            break;
                        case cgltf_attribute_type_texcoord:
                            format = Scene::Mesh::Format::UV;
                            break;
                        case cgltf_attribute_type_color:
                            format = Scene::Mesh::Format::COL;
                            break;
                    }
                    const char* buffer = ((char*)attr.data->buffer_view->buffer->data) +
                                         attr.data->buffer_view->offset + attr.data->offset;
                    prim.AddBuffer(buffer, format, (unsigned int)attr.data->stride, (unsigned int)attr.data->count);
                }

                // indices
                const char* buffer = ((char*)gltfPrim.indices->buffer_view->buffer->data) +
                                     gltfPrim.indices->buffer_view->offset + gltfPrim.indices->offset;
                prim.AddIndexBuffer(
                    buffer, (unsigned int)gltfPrim.indices->stride, (unsigned int)gltfPrim.indices->count);
            }
        }

        sc->mWorldTransforms.resize(data->nodes_count);
        sc->mMeshIndex.resize(data->nodes_count, -1);

        // transforms
        for (unsigned int i = 0; i < data->nodes_count; i++)
        {
            cgltf_node_transform_world(&data->nodes[i], sc->mWorldTransforms[i]);
        }

        for (unsigned int i = 0; i < data->nodes_count; i++)
        {
            if (!data->nodes[i].mesh)
                continue;
            sc->mMeshIndex[i] = int(data->nodes[i].mesh - data->meshes);
        }


        cgltf_free(data);
        *scene = sc;
        return EVAL_OK;
    }

} // namespace EvaluationAPI
