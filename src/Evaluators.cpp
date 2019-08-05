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
#include "Evaluators.h"
#include "EvaluationStages.h"
#include "Bitmap.h"
#include "EvaluationContext.h"
#include <vector>
#include <map>
#include <string>
#include "Scene.h"
#include "Loader.h"
#include "TiledRenderer.h"
#include "ProgressiveRenderer.h"
#include "GPUBVH.h"
#include "Camera.h"
#include <fstream>
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "GraphControler.h"
#include <functional>
#include <bgfx/embedded_shader.h>
#include "EmbeddedShaders.cpp"

Evaluators gEvaluators;

extern TaskScheduler g_TS;

struct EValuationFunction
{
    const char* szFunctionName;
    void* function;
};

void LogPython(const std::string& str)
{
    Log(str.c_str());
}

#if USE_PYTHON
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
    m.def("NewGraph", [](const std::string& graphName) 
    { 
        Imogen::instance->NewMaterial(graphName);
    });
    m.def("Build", []()
    {
        extern Builder* gBuilder;
        auto controler = Imogen::instance->GetNodeGraphControler();
        controler->ApplyDirtyList();
        gBuilder->Add("Graph", controler->mEvaluationStages);
    });
    m.def("AddNode", [](const std::string& nodeType) -> int { return Imogen::instance->AddNode(nodeType); });
    m.def("SetParameter", [](int nodeIndex, const std::string& paramName, const std::string& value) {
        Imogen::instance->GetNodeGraphControler()->mModel.BeginTransaction(false);
        Imogen::instance->GetNodeGraphControler()->mModel.SetParameter(nodeIndex, paramName, value);
        Imogen::instance->GetNodeGraphControler()->mModel.EndTransaction();
    });
    m.def("Connect", [](int nodeSource, int slotSource, int nodeDestination, int slotDestination) {
        auto& model = Imogen::instance->GetNodeGraphControler()->mModel;
        model.BeginTransaction(false);
        model.AddLink(nodeSource, slotSource, nodeDestination, slotDestination);
        model.EndTransaction();
    });
    m.def("AutoLayout", []() {
        auto controler = Imogen::instance->GetNodeGraphControler();
        controler->ApplyDirtyList();
        controler->mModel.NodeGraphLayout(controler->mEvaluationStages.GetForwardEvaluationOrder());
        controler->ApplyDirtyList();
        GraphEditorUpdateScrolling(controler);
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
    m.def("log2", static_cast<float (*)(float)>(log2f));
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
#endif

Evaluators::Evaluators()
{
}

Evaluators::~Evaluators()
{
}
/*
std::string Evaluators::GetEvaluator(const std::string& filename)
{
    return mEvaluatorScripts[filename].mText;
}
*/
void Evaluators::SetEvaluators()
{
    ClearEvaluators();

	const bgfx::RendererType::Enum type = bgfx::getRendererType();
	ShaderHandle nodeVSShader = bgfx::createEmbeddedShader(s_embeddedShaders, type, "Node_vs");

	// default shaders
	mBlitProgram = bgfx::createProgram(nodeVSShader, bgfx::createEmbeddedShader(s_embeddedShaders, type, "Blit_fs"), false);


    mEvaluatorPerNodeType.clear();
    mEvaluatorPerNodeType.resize(gMetaNodes.size(), nullptr);

	extern std::map<std::string, NodeFunction> nodeFunctions;

	for (int i = 0; i < gMetaNodes.size(); i++)
	{
		const std::string& nodeName = gMetaNodes[i].mName;
		std::string vsShaderName = nodeName + "_vs";
		std::string fsShaderName = nodeName + "_fs";
		std::string csShaderName = nodeName + "_cs";
		ShaderHandle vsShader = bgfx::createEmbeddedShader(s_embeddedShaders, type, vsShaderName.c_str());
		ShaderHandle fsShader = bgfx::createEmbeddedShader(s_embeddedShaders, type, fsShaderName.c_str());
		ShaderHandle csShader = bgfx::createEmbeddedShader(s_embeddedShaders, type, csShaderName.c_str());
		ProgramHandle program{0};
		int mask = 0;
		if (vsShader.idx != bgfx::kInvalidHandle && fsShader.idx != bgfx::kInvalidHandle)
		{
			// valid VS/FS
			program = bgfx::createProgram(vsShader, fsShader, false);
			mask |= EvaluationGLSL;
			assert(program.idx);
		}
		else if (fsShader.idx != bgfx::kInvalidHandle)
		{
			// valid FS -> use nodeVS
			program = bgfx::createProgram(nodeVSShader, fsShader, false);
			mask |= EvaluationGLSL;
			assert(program.idx);
		}
		else if (vsShader.idx != bgfx::kInvalidHandle)
		{
			// valid CS
			mask |= EvaluationGLSLCompute;
		}

		EvaluatorScript& shader = mEvaluatorScripts[nodeName];

		if (program.idx)
		{
			bgfx::UniformHandle handle = bgfx::createUniform("u_color", bgfx::UniformType::Vec4, 1);
			shader.mUniforms.push_back(handle);
			shader.mProgram = program;
		}
		// C++ functions
		auto iter = nodeFunctions.find(nodeName);
		if (iter != nodeFunctions.end())
		{
			shader.mCFunction = iter->second;
			mask |= EvaluationC;
		}
		shader.mMask = mask;
		shader.mType = i;
		mEvaluatorPerNodeType[i] = &shader;
		auto& evalNode = mEvaluatorPerNodeType[i];
		//assert(mask); enable when compute shaders are back
	}

	


	/*
#if USE_PYTHON
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
        }
        catch (...)
        {
            Log("Python exception\n");
        }
    }
#endif
*/
	
}

void Evaluators::ClearEvaluators()
{
    // clear
    for (auto& program : mEvaluatorPerNodeType)
    {
        /* todogl if (program.mGLSLProgram)
            glDeleteProgram(program.mGLSLProgram);
			*/

    }
}

int Evaluators::GetMask(size_t nodeType) const
{
	auto& evalNode = mEvaluatorPerNodeType[nodeType];
	return evalNode->mMask;
}

#if USE_PYTHON
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
#endif
void Evaluators::ReloadPlugins()
{
    #if USE_PYTHON
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
    #endif
}
#if USE_PYTHON
void Evaluators::EvaluatorScript::RunPython() const
{
    mPyModule.attr("main")(gEvaluators.mImogenModule.attr("accessor_api")());
}
#endif

namespace EvaluationAPI
{
    int SetEvaluationImageCube(EvaluationContext* evaluationContext, int target, const Image* image, int cubeFace)
    {
        if (image->mNumFaces != 1)
        {
            return EVAL_ERR;
        }
        auto tgt = evaluationContext->GetRenderTarget(target);
        if (!tgt)
        {
            tgt = evaluationContext->CreateRenderTarget(target);
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
        {
            return EVAL_ERR;
        }
        Material* material = library.Get(std::make_pair(0, context->GetMaterialUniqueId()));
        if (material)
        {
            material->mThumbnail = pngImage;
			material->mThumbnailTextureHandle = {0};
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
        {
            renderTarget = evaluationContext->CreateRenderTarget(target);
        }
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
        {
            renderTarget = evaluationContext->CreateRenderTarget(target);
        }
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
		/* todogl
#ifdef glGetTexImage
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
#endif
*/
        return EVAL_OK;
    }

    int SetEvaluationImage(EvaluationContext* evaluationContext, int target, const Image* image)
    {
        EvaluationStage& stage = evaluationContext->mEvaluationStages.mStages[target];
        auto tgt = evaluationContext->GetRenderTarget(target);
        if (!tgt)
        {
            tgt = evaluationContext->CreateRenderTarget(target);
        }
        unsigned int texelSize = textureFormatSize[image->mFormat];
        unsigned int inputFormat = glInputFormats[image->mFormat];
        unsigned int internalFormat = glInternalFormats[image->mFormat];
        unsigned char* ptr = image->GetBits();
        
		//TextureID texType = 0;// todogl GL_TEXTURE_2D;
		if (image->mNumFaces == 1)
		{
			tgt->InitBuffer(image->mWidth, image->mHeight, evaluationContext->mEvaluations[target].mbDepthBuffer);
		}
		else
		{
			tgt->InitCube(image->mWidth, image->mNumMips);
			//texType = 0;// todogl GL_TEXTURE_CUBE_MAP;
		}
		for (int face = 0; face < image->mNumFaces; face++)
		{
			int cubeFace = (image->mNumFaces == 1)?-1:face;
			for (int i = 0; i < image->mNumMips; i++)
			{
				Image::Upload(image, tgt->mGLTexID, cubeFace, i);
			}
		}
		/* todogl
		if (image->mNumMips > 1)
		{
			TexParam(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, texType);
		}
		else
		{
			TexParam(GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, texType);
		}
		*/
#if USE_FFMPEG
        if (stage.mDecoder.get() != (FFMPEGCodec::Decoder*)image->mDecoder)
		{
            stage.mDecoder = std::shared_ptr<FFMPEGCodec::Decoder>((FFMPEGCodec::Decoder*)image->mDecoder);
		}
#endif
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
                                   scene->hdrLoaderRes.width * scene->hdrLoaderRes.height * sizeof(float) * 3;

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
        /* todogl
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);
		*/
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

    struct CFunctionTaskSet : TaskSet
    {
        CFunctionTaskSet(jobFunction function, void* ptr, unsigned int size)
            : TaskSet(), mFunction(function), mBuffer(malloc(size))
        {
            memcpy(mBuffer, ptr, size);
        }
        virtual void ExecuteRange(TaskSetPartition range, uint32_t threadnum)
        {
            mFunction(mBuffer);
            free(mBuffer);
            delete this;
        }
        jobFunction mFunction;
        void* mBuffer;
    };

    struct FunctionTaskSet : TaskSet
    {
        FunctionTaskSet(std::function<int()> function)
            : TaskSet(), mFunction(function)
        {
        }
        virtual void ExecuteRange(TaskSetPartition range, uint32_t threadnum)
        {
            mFunction();
            delete this;
        }
        std::function<int()> mFunction;
    };

    struct CFunctionMainTask : PinnedTask
    {
        CFunctionMainTask(jobFunction function, void* ptr, unsigned int size)
            : PinnedTask(0) // set pinned thread to 0
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


    struct FunctionMainTask : PinnedTask
    {
        FunctionMainTask(std::function<int()> function)
            : PinnedTask(0) // set pinned thread to 0
            , mFunction(function)
        {
        }
        virtual void Execute()
        {
            mFunction();
            delete this;
        }
        std::function<int()> mFunction;
    };

    int Job(EvaluationContext* evaluationContext, std::function<int()> function)
    {
        if (evaluationContext->IsSynchronous())
        {
            return function();
        }
        else
        {
            g_TS.AddTaskSetToPipe(new FunctionTaskSet(function));
        }
        return EVAL_OK;
    }

    int JobMain(EvaluationContext* evaluationContext, std::function<int()> function)
    {
        if (evaluationContext->IsSynchronous())
        {
            return function();
        }
        else
        {
            g_TS.AddPinnedTask(new FunctionMainTask(function));
        }
        return EVAL_OK;
    }

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
            #if USE_FFMPEG
        // try to load movie
        auto decoder = evaluationContext->mEvaluationStages.FindDecoder(filename);
        *image = Image::DecodeImage(decoder, evaluationContext->GetCurrentTime());
        if (!image->mWidth || !image->mHeight)
            return EVAL_ERR;
            #endif
        return EVAL_OK;
    }

    int Write(EvaluationContext* evaluationContext, const char* filename, Image* image, int format, int quality)
    {
        if (format == 7)
        {
            #if USE_FFMPEG
            FFMPEGCodec::Encoder* encoder =
                evaluationContext->GetEncoder(std::string(filename), image->mWidth, image->mHeight);
            std::string fn(filename);
            encoder->AddFrame(image->GetBits(), image->mWidth, image->mHeight);
            #endif
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

    int ReadGLTF(EvaluationContext* evaluationContext, const char* filename, Scene** scene)
    {
        std::string strFilename(filename);
        auto iter = gSceneCache.find(strFilename);
        if (iter != gSceneCache.end() && (!iter->second.expired()))
        {
            *scene = iter->second.lock().get();
            return EVAL_OK;
        }
        cgltf_options options;
        memset(&options, 0, sizeof(options));
        cgltf_data* data = NULL;
        cgltf_result result = cgltf_parse_file(&options, filename, &data);
        if (result != cgltf_result_success)
            return EVAL_ERR;

        result = cgltf_load_buffers(&options, data, filename);
        if (result != cgltf_result_success)
            return EVAL_ERR;

        Scene* sc = new Scene;
        sc->mName = strFilename;
        *scene = sc;

        JobMain(evaluationContext, [evaluationContext, sc, data]() {
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
            return EVAL_OK;
        });
        return EVAL_OK;
    }

    int GLTFReadAsync(EvaluationContext* context, const char* filename, int target)
    {
        SetProcessing(context, target, 1);
        std::string strFilename = filename;
        Job(context, [context, target, strFilename](){
            Scene *scene;
            if (ReadGLTF(context, strFilename.c_str(), &scene) == EVAL_OK)
            {
                JobMain(context, [context, scene, target](){
                    SetEvaluationScene(context, target, scene);
                    SetProcessing(context, target, 0);
                    return EVAL_OK;
                });
            }
            else
            {
                SetProcessing(context, target, 0);
            }
            return EVAL_OK;
        });
        return EVAL_OK;
    }

    int ReadImageAsync(EvaluationContext* context, const char *filename, int target, int face)
    {
        std::string strFilename = filename;
        Job(context, [context, target, strFilename, face]() {
            Image image;
            if (Image::Read(strFilename.c_str(), &image) == EVAL_OK)
            {
                JobMain(context, [image, face, context, target](){
                    if (face != -1)
                    {
                        SetEvaluationImageCube(context, target, &image, face);
                    }
                    else
                    {
                        SetEvaluationImage(context, target, &image);
                    }
                    //FreeImage(&data->image);
                    SetProcessing(context, target, 0);
                    // don't set childOnly or the node will not be evaluated and we wont get the thumbnail
                    context->SetTargetDirty(target, Dirty::Input, false);
                    return EVAL_OK;
                });
            }
            else
            {
                SetProcessing(context, target, 0);
            }
            return EVAL_OK;
        });
        return EVAL_OK;
    }
} // namespace EvaluationAPI
