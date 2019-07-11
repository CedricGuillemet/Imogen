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

Evaluators gEvaluators;

extern TaskScheduler g_TS;

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
    {"memmove", (void*)memmove},
    {"strcpy", (void*)strcpy},
    {"strlen", (void*)strlen},
    {"fabsf", (void*)fabsf},
    {"strcmp", (void*)strcmp},
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
    {"GLTFReadAsync", (void*)EvaluationAPI::GLTFReadAsync},
    {"ReadImageAsync", (void*)EvaluationAPI::ReadImageAsync},
    
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
    m.def("NewGraph", [](const std::string& graphName) { Imogen::instance->NewMaterial(graphName); });
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
    graph.def("Build", [](PyGraph& pyGraph) {
        extern Builder* gBuilder;
        if (gBuilder)
        {
            Material* material = pyGraph.mGraph;
            gBuilder->Add(material);
        }
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
#endif


static duk_ret_t js_print(duk_context* ctx)
{
    duk_idx_t nargs;
    const duk_uint8_t* buf;
    duk_size_t sz_buf;

    nargs = duk_get_top(ctx);
    if (nargs == 1 && duk_is_buffer(ctx, 0)) 
    {
        buf = (const duk_uint8_t*)duk_get_buffer(ctx, 0, &sz_buf);
        std::string str((const char*)buf, (size_t)sz_buf);
        Log(str.c_str());
    }
    else 
    {
        duk_push_string(ctx, " ");
        duk_insert(ctx, 0);
        duk_join(ctx, nargs);
        Log("%s\n", duk_to_string(ctx, -1));
    }
    return 0;
}

static void pushJSIntArray(duk_context* ctx, const int* vals, size_t count)
{
    auto arr_idx = duk_push_array(ctx);
    for (size_t i = 0; i < count; i++)
    {
        duk_push_int(ctx, vals[i]);
        duk_put_prop_index(ctx, arr_idx, int(i));
    }
}

static void pushJSFloatArray(duk_context* ctx, const float* vals, size_t count)
{
    auto arr_idx = duk_push_array(ctx);
    for (size_t i = 0; i < count; i++)
    {
        duk_push_number(ctx, vals[i]);
        duk_put_prop_index(ctx, arr_idx, int(i));
    }
}

static void pushJSIntProperty(duk_context* ctx, const int val, const char* name, int object)
{
    duk_push_int(ctx, val);
    duk_put_prop_string(ctx, object, name);
}

/*
* This is the image destructor
*/
template<typename T> duk_ret_t js_dtor(duk_context* ctx)
{
    // The object to delete is passed as first argument instead
    duk_get_prop_string(ctx, 0, "\xff""\xff""deleted");

    bool deleted = duk_to_boolean(ctx, -1);
    duk_pop(ctx);

    if (!deleted) {
        duk_get_prop_string(ctx, 0, "\xff""\xff""data");
        delete static_cast<T*>(duk_to_pointer(ctx, -1));
        duk_pop(ctx);

        // Mark as deleted
        duk_push_boolean(ctx, true);
        duk_put_prop_string(ctx, 0, "\xff""\xff""deleted");
    }

    return 0;
}

/*
* This is image function, constructor. Note that it can be called
* as a standard function call, you may need to check for
* duk_is_constructor_call to be sure that it is constructed
* as a "new" statement.
*/
template<typename T> duk_ret_t js_ctor(duk_context* ctx)
{
    // Get arguments
    //float x = duk_require_number(ctx, 0);
    //float y = duk_require_number(ctx, 1);

    // Push special this binding to the function being constructed
    duk_push_this(ctx);

    // Store the underlying object
    duk_push_pointer(ctx, new T);
    duk_put_prop_string(ctx, -2, "\xff""\xff""data");

    // Store a boolean flag to mark the object as deleted because the destructor may be called several times
    duk_push_boolean(ctx, false);
    duk_put_prop_string(ctx, -2, "\xff""\xff""deleted");

    // Store the function destructor
    duk_push_c_function(ctx, js_dtor<T>, 1);
    duk_set_finalizer(ctx, -2);

    return 0;
}

/*
* Basic toString method
*/

duk_ret_t js_Image_toString(duk_context* ctx)
{
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "\xff""\xff""data");
    Image* img = static_cast<Image*>(duk_to_pointer(ctx, -1));
    duk_pop(ctx);
    duk_push_sprintf(ctx, "Image[%d, %d]", img->mWidth, img->mHeight);

    return 1;
}

// methods, add more here
const duk_function_list_entry methods_Image[] = {
    { "toString",   js_Image_toString,  0 },
    { nullptr,  nullptr,        0 }
};

duk_ret_t js_LoadSVG(duk_context* ctx)
{
    int ret = EVAL_ERR;
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 3)
    {
        duk_get_prop_string(ctx, -2, "\xff""\xff""data");
        Image* image = static_cast<Image*>(duk_to_pointer(ctx, -1));
        duk_pop(ctx);

        const char* str = duk_get_string(ctx, -3);
        int dpi = duk_get_int(ctx, -1);

        if (str && dpi && image)
        {
            ret = Image::LoadSVG(str, image, float(dpi));
        }

        duk_pop_n(ctx, 3);
    }
    duk_push_int(ctx, ret);
    return 1;
}

duk_ret_t js_SetEvaluationImage(duk_context* ctx)
{
    int ret = EVAL_ERR;
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 3)
    {
        duk_get_prop_string(ctx, -1, "\xff""\xff""data");
        Image* image = static_cast<Image*>(duk_to_pointer(ctx, -1));
        duk_pop(ctx);

        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -3));
        int target = duk_get_int(ctx, -2);

        if (context && image)
        {
            ret = EvaluationAPI::SetEvaluationImage(context, target, image);
        }

        duk_pop_n(ctx, 3);
    }
    duk_push_int(ctx, ret);
    return 1;
}

duk_ret_t js_GetEvaluationSize(duk_context* ctx)
{
    int ret = EVAL_ERR;
    int width = 0, height = 0;
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 2)
    {
        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -2));
        int target = duk_get_int(ctx, -1);

        if (context && target > -1)
        {
            ret = EvaluationAPI::GetEvaluationSize(context, target, &width, &height);
        }

        duk_pop_n(ctx, 2);
    }
    const int v[] = { width, height, ret };
    pushJSIntArray(ctx, v, 3);
    return 1;
}

duk_ret_t js_SetEvaluationSize(duk_context* ctx)
{
    int ret = EVAL_ERR;
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 4)
    {
        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -4));
        int target = duk_get_int(ctx, -3);
        int width = duk_get_int(ctx, -2);
        int height = duk_get_int(ctx, -1);

        if (context && target > -1 && width > 0 && height > 0)
        {
            ret = EvaluationAPI::SetEvaluationSize(context, target, width, height);
        }

        duk_pop_n(ctx, 4);
    }
    duk_push_int(ctx, ret);
    return 1;
}

duk_ret_t js_SetEvaluationCubeSize(duk_context* ctx)
{
    int ret = EVAL_ERR;
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 4)
    {
        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -4));
        int target = duk_get_int(ctx, -3);
        int size = duk_get_int(ctx, -2);
        int mipCount = duk_get_int(ctx, -1);

        if (context && target > -1 && size > 0 && mipCount > 0)
        {
            ret = EvaluationAPI::SetEvaluationCubeSize(context, target, size, mipCount);
        }

        duk_pop_n(ctx, 4);
    }
    duk_push_int(ctx, ret);
    return 1;
}

duk_ret_t js_EnableFrameClear(duk_context* ctx)
{
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 3)
    {
        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -3));
        int target = duk_get_int(ctx, -2);
        bool enable = duk_get_boolean(ctx, -1);

        if (context && target > -1)
        {
            EvaluationAPI::EnableFrameClear(context, target, enable);
        }

        duk_pop_n(ctx, 3);
    }
    duk_push_int(ctx, EVAL_OK);
    return 1;
}

duk_ret_t js_EnableDepthBuffer(duk_context* ctx)
{
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 3)
    {
        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -3));
        int target = duk_get_int(ctx, -2);
        bool enable = duk_get_boolean(ctx, -1);

        if (context && target > -1)
        {
            EvaluationAPI::EnableDepthBuffer(context, target, enable);
        }

        duk_pop_n(ctx, 3);
    }
    duk_push_int(ctx, EVAL_OK);
    return 1;
}

duk_ret_t js_AllocateComputeBuffer(duk_context* ctx)
{
    int ret = EVAL_ERR;
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 4)
    {
        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -4));
        int target = duk_get_int(ctx, -3);
        int elementCount = duk_get_int(ctx, -2);
        int elementSize = duk_get_int(ctx, -1);

        if (context && target > -1 && elementCount > 0 && elementSize > 0)
        {
            ret = EvaluationAPI::AllocateComputeBuffer(context, target, elementCount, elementSize);
        }

        duk_pop_n(ctx, 4);
    }
    duk_push_int(ctx, ret);
    return 1;
}

duk_ret_t js_SetBlendingMode(duk_context* ctx)
{
    int ret = EVAL_ERR;
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 4)
    {
        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -4));
        int target = duk_get_int(ctx, -3);
        int blendSrc = duk_get_int(ctx, -2);
        int blendDst = duk_get_int(ctx, -1);

        if (context && target > -1 && blendSrc >= 0 && blendSrc < BLEND_LAST && blendDst >= 0 && blendDst < BLEND_LAST)
        {
            EvaluationAPI::SetBlendingMode(context, target, blendSrc, blendDst);
            ret = EVAL_OK;
        }

        duk_pop_n(ctx, 4);
    }
    duk_push_int(ctx, ret);
    return 1;
}

duk_ret_t js_OverrideInput(duk_context* ctx)
{
    int ret = EVAL_ERR;
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 4)
    {
        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -4));
        int target = duk_get_int(ctx, -3);
        int slot = duk_get_int(ctx, -2);
        int node = duk_get_int(ctx, -1);

        if (context && target > -1 && slot >= 0)
        {
            ret = EvaluationAPI::OverrideInput(context, target, slot, node);
        }

        duk_pop_n(ctx, 4);
    }
    duk_push_int(ctx, ret);
    return 1;
}

duk_ret_t js_SetVertexSpace(duk_context* ctx)
{
    int ret = EVAL_ERR;
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 4)
    {
        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -3));
        int target = duk_get_int(ctx, -2);
        int space = duk_get_int(ctx, -1);

        if (context && target > -1 && space >= 0 && space <= 1)
        {
            EvaluationAPI::SetVertexSpace(context, target, space);
            ret = EVAL_OK;
        }

        duk_pop_n(ctx, 4);
    }
    duk_push_int(ctx, ret);
    return 1;
}

duk_ret_t js_GetEvaluationScene(duk_context* ctx)
{
    int ret = EVAL_ERR;
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 4)
    {
        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -4));
        int target = duk_get_int(ctx, -3);
        int elementCount = duk_get_int(ctx, -2);
        int elementSize = duk_get_int(ctx, -1);

        if (context && target > -1 && elementCount > 0 && elementSize > 0)
        {
            ret = EvaluationAPI::AllocateComputeBuffer(context, target, elementCount, elementSize);
        }

        duk_pop_n(ctx, 4);
    }
    duk_push_int(ctx, ret);
    return 1;
}

duk_ret_t js_SetEvaluationScene(duk_context* ctx)
{
    int ret = EVAL_ERR;
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 4)
    {
        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -4));
        int target = duk_get_int(ctx, -3);
        int elementCount = duk_get_int(ctx, -2);
        int elementSize = duk_get_int(ctx, -1);

        if (context && target > -1 && elementCount > 0 && elementSize > 0)
        {
            ret = EvaluationAPI::AllocateComputeBuffer(context, target, elementCount, elementSize);
        }

        duk_pop_n(ctx, 4);
    }
    duk_push_int(ctx, ret);
    return 1;
}

duk_ret_t js_GetEvaluationSceneName(duk_context* ctx)
{
    int ret = EVAL_ERR;
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 4)
    {
        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -4));
        int target = duk_get_int(ctx, -3);
        int elementCount = duk_get_int(ctx, -2);
        int elementSize = duk_get_int(ctx, -1);

        if (context && target > -1 && elementCount > 0 && elementSize > 0)
        {
            ret = EvaluationAPI::AllocateComputeBuffer(context, target, elementCount, elementSize);
        }

        duk_pop_n(ctx, 4);
    }
    duk_push_int(ctx, ret);
    return 1;
}

duk_ret_t js_SetProcessing(duk_context* ctx)
{
    duk_idx_t nargs = duk_get_top(ctx);
    if (nargs == 3)
    {
        EvaluationContext* context = static_cast<EvaluationContext*>(duk_get_pointer(ctx, -3));
        int target = duk_get_int(ctx, -2);
        int processing = duk_get_int(ctx, -1);

        if (context && target > -1)
        {
            EvaluationAPI::SetProcessing(context, target, processing);
        }

        duk_pop_n(ctx, 3);
    }
    duk_push_int(ctx, EVAL_OK);
    return 1;
}

void js_RegisterFunction(duk_context* ctx, const char* functionName, duk_ret_t(*function)(duk_context* ctx), int parameterCount)
{
    duk_push_global_object(ctx);
    duk_push_string(ctx, functionName);
    duk_push_c_function(ctx, function, parameterCount);
    duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_WRITABLE | DUK_DEFPROP_SET_CONFIGURABLE);
    duk_pop(ctx);
}


Evaluators::Evaluators()
{
    m_ctx = duk_create_heap_default();
    assert(m_ctx);

    js_RegisterFunction(m_ctx, "log", js_print, DUK_VARARGS);
    js_RegisterFunction(m_ctx, "print", js_print, DUK_VARARGS);
    js_RegisterFunction(m_ctx, "LoadSVG", js_LoadSVG, 3);
    js_RegisterFunction(m_ctx, "SetEvaluationImage", js_SetEvaluationImage, 3);
    js_RegisterFunction(m_ctx, "GetEvaluationSize", js_GetEvaluationSize, 2);
    js_RegisterFunction(m_ctx, "SetEvaluationSize", js_SetEvaluationSize, 4);
    js_RegisterFunction(m_ctx, "SetEvaluationCubeSize", js_SetEvaluationCubeSize, 4);
    js_RegisterFunction(m_ctx, "EnableFrameClear", js_EnableFrameClear, 4);
    js_RegisterFunction(m_ctx, "EnableDepthBuffer", js_EnableDepthBuffer, 4);
    js_RegisterFunction(m_ctx, "AllocateComputeBuffer", js_AllocateComputeBuffer, 4);


    js_RegisterFunction(m_ctx, "SetBlendingMode", js_SetBlendingMode, 4);
    js_RegisterFunction(m_ctx, "OverrideInput", js_OverrideInput, 4);
    js_RegisterFunction(m_ctx, "SetVertexSpace", js_SetVertexSpace, 3);

    js_RegisterFunction(m_ctx, "GetEvaluationScene", js_GetEvaluationScene, 3);
    js_RegisterFunction(m_ctx, "SetEvaluationScene", js_SetEvaluationScene, 3);

    js_RegisterFunction(m_ctx, "GetEvaluationSceneName", js_GetEvaluationSceneName, 2);

    js_RegisterFunction(m_ctx, "SetProcessing", js_SetProcessing, 3);

    
    //GetEvaluationSceneName(context, target)

    /*
    SetBlendingMode(context, evaluation.targetIndex, ONE, ONE_MINUS_SRC_ALPHA);
    OverrideInput(context, evaluation.targetIndex, 0, -1); 
    SetVertexSpace(context, evaluation.targetIndex, VertexSpace_UV);
    
    GetEvaluationScene(context, evaluation.inputIndices[0], &scene)
    SetEvaluationScene(context, evaluation.targetIndex, scene);
    */

    // Image class
    duk_push_c_function(m_ctx, js_ctor<Image>, 2);
    duk_push_object(m_ctx);
    duk_put_function_list(m_ctx, -1, methods_Image);
    duk_put_prop_string(m_ctx, -2, "prototype");
    duk_put_global_string(m_ctx, "Image");

    duk_eval_string(m_ctx, R"(
				const DirtyInput = (1 << 0);
				const DirtyParameter = (1 << 1);
				const DirtyMouse = (1 << 2);
				const DirtyCamera = (1 << 3);
				const DirtyTime = (1 << 4);
				const DirtySampler = (1 << 5);
				const EVAL_OK = 0;
				const EVAL_ERR = 1;
				const EVAL_DIRTY = 2;
                const VertexSpace_UV = 0;
                const VertexSpace_World = 1;
                const ZERO = 0;
	            const ONE = 1;
	            const SRC_COLOR = 2;
	            const ONE_MINUS_SRC_COLOR = 3;
	            const DST_COLOR = 4;
	            const ONE_MINUS_DST_COLOR = 5;
	            const SRC_ALPHA = 6;
	            const ONE_MINUS_SRC_ALPHA = 7;
	            const DST_ALPHA = 8;
	            const ONE_MINUS_DST_ALPHA = 9;
	            const CONSTANT_COLOR = 10;
	            const ONE_MINUS_CONSTANT_COLOR = 11;
	            const CONSTANT_ALPHA = 12;
	            const ONE_MINUS_CONSTANT_ALPHA = 13;
	            const SRC_ALPHA_SATURATE = 14;
			    )");
}

Evaluators::~Evaluators()
{
    duk_destroy_heap(m_ctx);
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

#if USE_LIBTCC
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
#endif

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
            if (shader.mType != -1)
                mEvaluatorPerNodeType[shader.mType].mPyModule = shader.mPyModule;
        }
        catch (...)
        {
            Log("Python exception\n");
        }
    }
#endif

    for (auto& file : evaluatorfilenames)
    {
        if (file.mEvaluatorType != EVALUATOR_JS)
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
            const std::string filename = file.mFilename;

            EvaluatorScript& shader = mEvaluatorScripts[filename];
            duk_eval_string(m_ctx, str.c_str());
        }
        catch (std::exception e)
        {
            printf("Duktape exception: %s\n", e.what());
        }
    }
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
    mEvaluatorPerNodeType[nodeType].mName = nodeName;
    mEvaluatorPerNodeType[nodeType].mNodeType = nodeType;
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
    iter = mEvaluatorScripts.find(nodeName + ".js");
    if (iter != mEvaluatorScripts.end())
    {
        mask |= EvaluationJS;
        iter->second.mType = int(nodeType);
        mEvaluatorPerNodeType[nodeType].m_ctx = m_ctx;
    }
#if USE_PYTHON
    iter = mEvaluatorScripts.find(nodeName + ".py");
    if (iter != mEvaluatorScripts.end())
    {
        mask |= EvaluationPython;
        iter->second.mType = int(nodeType);
        mEvaluatorPerNodeType[nodeType].mPyModule = iter->second.mPyModule;
    }
#endif
    return mask;
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
void Evaluator::RunPython() const
{
    mPyModule.attr("main")(gEvaluators.mImogenModule.attr("accessor_api")());
}
#endif

int Evaluator::RunJS(unsigned char* parametersBuffer, const EvaluationInfo* evaluationInfo, EvaluationContext* context) const
{
    if (duk_get_global_string(m_ctx, mName.c_str()))
    {
        // parameters
        auto parameters = duk_push_object(m_ctx);

        const auto& params = gMetaNodes[mNodeType].mParams;
        int parameterIndex = 0;
        for (const auto& param : params)
        {
            unsigned char* paramBuffer = parametersBuffer;
            paramBuffer += GetParameterOffset(uint32_t(mNodeType), parameterIndex++);
            float* pf = (float*)paramBuffer;
            int* pi = (int*)paramBuffer;
            Camera* cam = (Camera*)paramBuffer;
            ImVec2* iv2 = (ImVec2*)paramBuffer;
            ImVec4* iv4 = (ImVec4*)paramBuffer;
            const char* str = (const char*)paramBuffer;
            switch (param.mType)
            {
            case Con_Angle:
            case Con_Float:
                duk_push_number(m_ctx, pf[0]);
                break;
            case Con_Angle2:
            case Con_Float2:
                pushJSFloatArray(m_ctx, pf, 2);
                break;
            case Con_Angle3:
            case Con_Float3:
                pushJSFloatArray(m_ctx, pf, 3);
                break;
            case Con_Angle4:
            case Con_Color4:
            case Con_Float4:
                pushJSFloatArray(m_ctx, pf, 4);
                break;
            case Con_Ramp:
                pushJSFloatArray(m_ctx, pf, 2 * 8);
                break;
            case Con_Ramp4:
                pushJSFloatArray(m_ctx, pf, 4 * 8);
                break;
            case Con_Enum:
            case Con_Int:
                duk_push_int(m_ctx, pi[0]);
                break;
            case Con_Int2:
                pushJSIntArray(m_ctx, pi, 2);
                break;
            case Con_FilenameRead:
            case Con_FilenameWrite:
                duk_push_string(m_ctx, str);
                break;
            case Con_Bool:
                duk_push_boolean(m_ctx, pi[0] != 0);
                break;
            default:
                continue;
            }
            duk_put_prop_string(m_ctx, parameters, param.mName.c_str());
        }

        //duk_push_string(ctx, "mySVGFile.svg");
        //duk_put_prop_string(ctx, parameters, "filename");


        // evaluation infos
        auto evaluation = duk_push_object(m_ctx);

        pushJSIntProperty(m_ctx, evaluationInfo->targetIndex, "targetIndex", evaluation);
        pushJSIntProperty(m_ctx, evaluationInfo->forcedDirty, "forcedDirty", evaluation);
        pushJSIntProperty(m_ctx, evaluationInfo->uiPass, "uiPass", evaluation);
        pushJSIntProperty(m_ctx, evaluationInfo->passNumber, "passNumber", evaluation);
        pushJSIntProperty(m_ctx, evaluationInfo->frame, "frame", evaluation);
        pushJSIntProperty(m_ctx, evaluationInfo->localFrame, "localFrame", evaluation);
        pushJSIntProperty(m_ctx, evaluationInfo->vertexSpace, "vertexSpace", evaluation);
        pushJSIntProperty(m_ctx, evaluationInfo->dirtyFlag, "dirtyFlag", evaluation);
        pushJSIntProperty(m_ctx, evaluationInfo->mipmapNumber, "mipmapNumber", evaluation);
        pushJSIntProperty(m_ctx, evaluationInfo->mipmapCount, "mipmapCount", evaluation);

        pushJSIntArray(m_ctx, evaluationInfo->keyModifier, 4);
        duk_put_prop_string(m_ctx, evaluation, "keyModifier");

        pushJSIntArray(m_ctx, evaluationInfo->inputIndices, 8);
        duk_put_prop_string(m_ctx, evaluation, "inputIndices");


        duk_push_pointer(m_ctx, context);

        if (duk_pcall(m_ctx, 3) != DUK_EXEC_SUCCESS)
        {
            Log(duk_safe_to_string(m_ctx, -1));
        }
        
    }
    return EVAL_OK;
}

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
        return EVAL_OK;
    }

    int SetEvaluationImage(EvaluationContext* evaluationContext, int target, const Image* image)
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
        #if USE_FFMPEG
        if (stage.mDecoder.get() != (FFMPEGCodec::Decoder*)image->mDecoder)
            stage.mDecoder = std::shared_ptr<FFMPEGCodec::Decoder>((FFMPEGCodec::Decoder*)image->mDecoder);
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

    int ReadImageAsync(EvaluationContext* context, char *filename, int target, int face)
    {
        std::string strFilename = filename;
        Job(context, [context, target, strFilename, face]() {
            Image image;
            if (Image::Read(strFilename.c_str(), &image) == EVAL_OK)
            {
                JobMain(context, [image, face, context, target](){
                    if (face)
                    {
                        SetEvaluationImageCube(context, target, &image, face);
                    }
                    else
                    {
                        SetEvaluationImage(context, target, &image);
                    }
                    //FreeImage(&data->image);
                    SetProcessing(context, target, 0);
                    context->SetTargetDirty(target, Dirty::Input, true);
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
