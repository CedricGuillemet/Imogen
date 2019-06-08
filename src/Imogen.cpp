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
#include "imgui.h"
#include "Imogen.h"
#include <fstream>
#include <streambuf>
#include "EvaluationStages.h"
#include "GraphControler.h"
#include "Library.h"
#include "stb_image.h"
#include "tinydir.h"
#include "stb_image.h"
#include "imgui_stdlib.h"
#include "ImSequencer.h"
#include "Evaluators.h"
#include "UI.h"
#include "imgui_markdown/imgui_markdown.h"
#include "imHotKey.h"
#include "imgInspect.h"
#include "IconsFontAwesome4.h"

Imogen* Imogen::instance = nullptr;
unsigned char* stbi_write_png_to_mem(unsigned char* pixels, int stride_bytes, int x, int y, int n, int* out_len);

extern TaskScheduler g_TS;

std::vector<RegisteredPlugin> mRegisteredPlugins;

void LinkCallback(ImGui::MarkdownLinkCallbackData data_)
{
    std::string url(data_.link, data_.linkLength);
    static const std::string graph = "thumbnail:";
    if (url.substr(0, graph.size()) == graph)
    {
        std::string materialName = url.substr(graph.size());
        ((Imogen*)data_.userData)->SetExistingMaterialActive(materialName.c_str());
        return;
    }
    OpenShellURL(url);
}

inline ImGui::MarkdownImageData ImageCallback(ImGui::MarkdownLinkCallbackData data_)
{
    std::string url(data_.link, data_.linkLength);
    static const std::string thumbnail = "thumbnail:";
    if (url.substr(0, thumbnail.size()) == thumbnail)
    {
        std::string material = url.substr(thumbnail.size());
        Material* libraryMaterial = library.GetByName(material.c_str());
        if (libraryMaterial)
        {
            ((Imogen*)data_.userData)->DecodeThumbnailAsync(libraryMaterial);
            return {true,
                    true,
                    (ImTextureID)(uint64_t)libraryMaterial->mThumbnailTextureId,
                    ImVec2(100, 100),
                    ImVec2(0.f, 1.f),
                    ImVec2(1.f, 0.f)};
        }
    }
    else
    {
        int percent = 100;
        size_t sz = url.find('@');
        if (sz != std::string::npos)
        {
            sscanf(url.c_str() + sz + 1, "%d%%", &percent);
            url = url.substr(0, sz);
        }
        unsigned int textureId = gImageCache.GetTexture(url);
        if (textureId)
        {
            int w, h;
            GetTextureDimension(textureId, &w, &h);
            return {true,
                    false,
                    (ImTextureID)(uint64_t)textureId,
                    ImVec2(float(w * percent / 100), float(h * percent / 100)),
                    ImVec2(0.f, 1.f),
                    ImVec2(1.f, 0.f)};
        }
    }

    return {false};
}

ImGui::MarkdownConfig mdConfig = {LinkCallback, ImageCallback, "", {{NULL, true}, {NULL, true}, {NULL, false}}};

struct ExtractedView
{
    size_t mNodeIndex;
    bool mFirstFrame;
};
std::vector<ExtractedView> mExtratedViews;
void AddExtractedView(size_t nodeIndex)
{
    mExtratedViews.push_back({nodeIndex, true});
}
void ClearExtractedViews()
{
    mExtratedViews.clear();
}

void Imogen::RenderPreviewNode(int selNode, GraphControler& nodeGraphControler, bool forceUI)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF000000);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF000000);
    ImGui::PushStyleColor(ImGuiCol_Button, 0xFF000000);
    float w = ImGui::GetContentRegionAvail().x;
    int imageWidth(1), imageHeight(1);

    // make 2 evaluation for node to get the UI pass image size
    if (selNode != -1 && nodeGraphControler.mModel.NodeHasUI(selNode))
    {
        nodeGraphControler.mEditingContext.AllocRenderTargetsForEditingPreview();
        EvaluationInfo evaluationInfo;
        evaluationInfo.forcedDirty = 1;
        evaluationInfo.uiPass = 1;
        nodeGraphControler.mEditingContext.RunSingle(selNode, evaluationInfo);
    }
    EvaluationAPI::GetEvaluationSize(&nodeGraphControler.mEditingContext, selNode, &imageWidth, &imageHeight);
    if (selNode != -1 && nodeGraphControler.mModel.NodeHasUI(selNode))
    {
        EvaluationInfo evaluationInfo;
        evaluationInfo.forcedDirty = 1;
        evaluationInfo.uiPass = 0;
        nodeGraphControler.mEditingContext.RunSingle(selNode, evaluationInfo);
    }
    ImTextureID displayedTexture = 0;
    ImRect rc;
    ImVec2 displayedTextureSize;
    ImVec2 mouseUVCoord(-FLT_MAX, -FLT_MAX);
    if (imageWidth && imageHeight)
    {
        float ratio = float(imageHeight) / float(imageWidth);
        float h = w * ratio;
        ImVec2 p = ImGui::GetCursorPos() + ImGui::GetWindowPos();

        if (forceUI)
        {
            ImGui::InvisibleButton("ImTheInvisibleMan", ImVec2(w, h));
            rc = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            AddUICustomDraw(draw_list, rc, DrawUICallbacks::DrawUISingle, selNode, &nodeGraphControler.mEditingContext);

            mouseUVCoord = (io.MousePos - rc.Min) / rc.GetSize();
            mouseUVCoord.y = 1.f - mouseUVCoord.y;
        }
        else
        {
            if (selNode != -1 && nodeGraphControler.NodeIsCubemap(selNode))
            {
                ImGui::InvisibleButton("ImTheInvisibleMan", ImVec2(w, h));
            }
            else
            {
                displayedTexture = (ImTextureID)(int64_t)(
                    (selNode != -1) ? nodeGraphControler.mEditingContext.GetEvaluationTexture(selNode) : 0);
                if (displayedTexture)
                {
                    auto tgt = nodeGraphControler.mEditingContext.GetRenderTarget(selNode);
                    displayedTextureSize = ImVec2(float(tgt->mImage->mWidth), float(tgt->mImage->mHeight));
                }
                ImVec2 mouseUVPos = (io.MousePos - p) / ImVec2(w, h);
                mouseUVPos.y = 1.f - mouseUVPos.y;
                Vec4 mouseUVPosv(mouseUVPos.x, mouseUVPos.y);
                mouseUVCoord = ImVec2(mouseUVPosv.x, mouseUVPosv.y);

                Vec4 uva(0, 0), uvb(1, 1);
                auto nodeType = nodeGraphControler.mModel.GetNodeType(selNode);
                /*Mat4x4* viewMatrix = nodeGraphControler.mModel.mEvaluationStages.GetParameterViewMatrix(selNode); todo
                const Camera* nodeCamera = GetCameraParameter(nodeType, nodeGraphControler.mModel.GetParameters(selNode));
                if (viewMatrix && !nodeCamera)
                {
                    Mat4x4& res = *viewMatrix;
                    Mat4x4 tr, trp, sc;
                    static float scale = 1.f;
                    scale = ImLerp(scale, 1.f, 0.15f);
                    if (ImRect(p, p + ImVec2(w, h)).Contains(io.MousePos) && ImGui::IsWindowFocused())
                    {
                        scale -= io.MouseWheel * 0.04f;
                        scale -= io.MouseDelta.y * 0.01f * ((io.KeyAlt && io.MouseDown[1]) ? 1.f : 0.f);

                        ImVec2 pix2uv = ImVec2(1.f, 1.f) / ImVec2(w, h);
                        Vec4 localTranslate;
                        localTranslate = Vec4(-io.MouseDelta.x * pix2uv.x, io.MouseDelta.y * pix2uv.y) *
                                         ((io.KeyAlt && io.MouseDown[2]) ? 1.f : 0.f) * res[0];
                        Mat4x4 localTranslateMat;
                        localTranslateMat.Translation(localTranslate);
                        res *= localTranslateMat;
                    }

                    mouseUVPosv.TransformPoint(res);
                    mouseUVCoord = ImVec2(mouseUVPosv.x, mouseUVPosv.y);
                    sc.Scale(scale, scale, 1.f);
                    tr.Translation(-mouseUVPosv);
                    trp.Translation(mouseUVPosv);
                    res *= tr * sc * trp;
                    res[0] = ImClamp(res[0], 0.05f, 1.f);
                    res[5] = res[0];

                    res[12] = ImClamp(res[12], 0.f, 1.f - res[0]);
                    res[13] = ImClamp(res[13], 0.f, 1.f - res[5]);

                    uva.TransformPoint(res);
                    uvb.TransformPoint(res);
                }
                */
                ImGui::ImageButton(displayedTexture, ImVec2(w, h), ImVec2(uva.x, uvb.y), ImVec2(uvb.x, uva.y));
            }
            rc = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            if (selNode != -1 && nodeGraphControler.NodeIsCubemap(selNode))
            {
                AddUICustomDraw(
                    draw_list, rc, DrawUICallbacks::DrawUICubemap, selNode, &nodeGraphControler.mEditingContext);
            }
            else if (selNode != -1 && nodeGraphControler.mModel.NodeHasUI(selNode))
            {
                AddUICustomDraw(
                    draw_list, rc, DrawUICallbacks::DrawUISingle, selNode, &nodeGraphControler.mEditingContext);
            }
        }
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(1);

    static int lastSentExit = -1;
    if (rc.Contains(io.MousePos))
    {
        static Image pickerImage;
        if (io.KeyShift && io.MouseDown[0] && displayedTexture && selNode > -1 && mouseUVCoord.x >= 0.f &&
            mouseUVCoord.y >= 0.f)
        {
            if (!pickerImage.GetBits())
            {
                EvaluationAPI::GetEvaluationImage(&nodeGraphControler.mEditingContext, selNode, &pickerImage);
                Log("Texel view Get image\n");
            }
            int width = pickerImage.mWidth;
            int height = pickerImage.mHeight;

            ImageInspect::inspect(width, height, pickerImage.GetBits(), mouseUVCoord, displayedTextureSize);
        }
        else if (ImGui::IsWindowFocused())
        {
            Image::Free(&pickerImage);
            ImVec2 ratio((io.MousePos.x - rc.Min.x) / rc.GetSize().x, (io.MousePos.y - rc.Min.y) / rc.GetSize().y);
            ImVec2 deltaRatio((io.MouseDelta.x) / rc.GetSize().x, (io.MouseDelta.y) / rc.GetSize().y);
            UIInput input = {ratio.x,
                             ratio.y,
                             deltaRatio.x,
                             deltaRatio.y,
                             io.MouseWheel,
                             io.MouseDown[0],
                             io.MouseDown[1],
                             io.KeyCtrl,
                             io.KeyAlt,
                             io.KeyShift};
            nodeGraphControler.SetKeyboardMouse(input, true);
        }
        lastSentExit = -1;
    }
    else
    {
        if (lastSentExit != selNode)
        {
            lastSentExit = selNode;
            UIInput input = {
                -9999.f,
                -9999.f,
                -9999.f,
                -9999.f,
                0,
                false,
                false,
                false,
                false,
                false
            };
            nodeGraphControler.SetKeyboardMouse(input, false);
        }
    }
}

template<typename T, typename Ty>
struct SortedResource
{
    SortedResource()
    {
    }
    SortedResource(unsigned int index, const std::vector<T, Ty>* res) : mIndex(index), mRes(res)
    {
    }
    SortedResource(const SortedResource& other) : mIndex(other.mIndex), mRes(other.mRes)
    {
    }
    void operator=(const SortedResource& other)
    {
        mIndex = other.mIndex;
        mRes = other.mRes;
    }
    unsigned int mIndex;
    const std::vector<T, Ty>* mRes;
    bool operator<(const SortedResource& other) const
    {
        return (*mRes)[mIndex].mName < (*mRes)[other.mIndex].mName;
    }

    static void ComputeSortedResources(const std::vector<T, Ty>& res, std::vector<SortedResource>& sortedResources)
    {
        sortedResources.resize(res.size());
        for (unsigned int i = 0; i < res.size(); i++)
            sortedResources[i] = SortedResource<T, Ty>(i, &res);
        std::sort(sortedResources.begin(), sortedResources.end());
    }
};

struct PinnedTaskUploadImage : PinnedTask
{
    PinnedTaskUploadImage(Image* image, ASyncId identifier, bool isThumbnail, GraphControler* controler)
        : PinnedTask(0) // set pinned thread to 0
        , mImage(image)
        , mIdentifier(identifier)
        , mbIsThumbnail(isThumbnail)
        , mControler(controler)
    {
    }

    virtual void Execute()
    {
        unsigned int textureId = Image::Upload(mImage, 0);
        if (mbIsThumbnail)
        {
            Material* material = library.Get(mIdentifier);
            if (material)
                material->mThumbnailTextureId = textureId;
        }
        else
        {
            /*auto* node = mControler->Get(mIdentifier); todo
            size_t nodeIndex = node - mControler->mModel.mEvaluationStages.mStages.data();
            if (node)
            {
                EvaluationAPI::SetEvaluationImage(&mControler->mEditingContext, int(nodeIndex), mImage);
                mControler->mModel.mEvaluationStages.SetEvaluationParameters(nodeIndex, mControler->mModel.GetParameters(nodeIndex));
                mControler->mEditingContext.StageSetProcessing(nodeIndex, false);
            }
            */
            Image::Free(mImage);
        }
    }
    Image* mImage;
    GraphControler* mControler;
    ASyncId mIdentifier;
    bool mbIsThumbnail;
};

struct DecodeThumbnailTaskSet : TaskSet
{
    DecodeThumbnailTaskSet(std::vector<uint8_t>* src, ASyncId identifier, GraphControler* nodeGraphControler)
        : TaskSet(), mIdentifier(identifier), mSrc(src), mNodeGraphControler(nodeGraphControler)
    {
    }
    virtual void ExecuteRange(TaskSetPartition range, uint32_t threadnum)
    {
        Image image;
        int components;
        unsigned char* data =
            stbi_load_from_memory(mSrc->data(), int(mSrc->size()), &image.mWidth, &image.mHeight, &components, 0);
        if (data)
        {
            image.SetBits(data, image.mWidth * image.mHeight * components);
            image.mNumFaces = 1;
            image.mNumMips = 1;
            image.mFormat = (components == 4) ? TextureFormat::RGBA8 : TextureFormat::RGB8;
            PinnedTaskUploadImage uploadTexTask(&image, mIdentifier, true, mNodeGraphControler);
            g_TS.AddPinnedTask(&uploadTexTask);
            g_TS.WaitforTask(&uploadTexTask);
            stbi_image_free(data);
            Image::Free(&image);
        }
        delete this;
    }
    ASyncId mIdentifier;
    std::vector<uint8_t>* mSrc;
    GraphControler* mNodeGraphControler;
};

struct EncodeImageTaskSet : TaskSet
{
    EncodeImageTaskSet(Image image, ASyncId materialIdentifier, ASyncId nodeIdentifier)
        : TaskSet(), mMaterialIdentifier(materialIdentifier), mNodeIdentifier(nodeIdentifier), mImage(image)
    {
    }
    virtual void ExecuteRange(TaskSetPartition range, uint32_t threadnum)
    {
        int outlen;
        int components = 4;
        unsigned char* bits = stbi_write_png_to_mem((unsigned char*)mImage.GetBits(),
                                                    mImage.mWidth * components,
                                                    mImage.mWidth,
                                                    mImage.mHeight,
                                                    components,
                                                    &outlen);
        if (bits)
        {
            Material* material = library.Get(mMaterialIdentifier);
            if (material)
            {
                MaterialNode* node = material->Get(mNodeIdentifier);
                if (node)
                {
                    node->mImage.resize(outlen);
                    memcpy(node->mImage.data(), bits, outlen);
                }
            }
        }
        delete this;
    }
    ASyncId mMaterialIdentifier;
    ASyncId mNodeIdentifier;
    Image mImage;
};

struct DecodeImageTaskSet : TaskSet
{
    DecodeImageTaskSet(std::vector<uint8_t>* src, ASyncId identifier, GraphControler* nodeGraphControler)
        : TaskSet(), mIdentifier(identifier), mSrc(src), mNodeGraphControler(nodeGraphControler)
    {
    }
    virtual void ExecuteRange(TaskSetPartition range, uint32_t threadnum)
    {
        Image image;
        int components;
        unsigned char* data =
            stbi_load_from_memory(mSrc->data(), int(mSrc->size()), &image.mWidth, &image.mHeight, &components, 0);
        if (data)
        {
            image.SetBits(data, image.mWidth * image.mHeight * components);
            image.mNumFaces = 1;
            image.mNumMips = 1;
            image.mFormat = (components == 3) ? TextureFormat::RGB8 : TextureFormat::RGBA8;
            PinnedTaskUploadImage uploadTexTask(&image, mIdentifier, false, mNodeGraphControler);
            g_TS.AddPinnedTask(&uploadTexTask);
            g_TS.WaitforTask(&uploadTexTask);
            stbi_image_free(data);
        }
        delete this;
    }
    ASyncId mIdentifier;
    std::vector<uint8_t>* mSrc;
    GraphControler* mNodeGraphControler;
};

void Imogen::DecodeThumbnailAsync(Material* material)
{
    static unsigned int defaultTextureId = gImageCache.GetTexture("Stock/thumbnail-icon.png");
    if (!material->mThumbnailTextureId)
    {
        material->mThumbnailTextureId = defaultTextureId;
        g_TS.AddTaskSetToPipe(
            new DecodeThumbnailTaskSet(&material->mThumbnail,
                                       std::make_pair(material - library.mMaterials.data(), material->mRuntimeUniqueId),
                                       mNodeGraphControler));
    }
}

template<typename T, typename Ty>
bool TVRes(std::vector<T, Ty>& res, const char* szName, int& selection, int index, int viewMode, Imogen* imogen)
{
    bool ret = false;
    if (!ImGui::TreeNodeEx(szName, ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_DefaultOpen))
        return ret;

    std::string currentGroup = "";
    bool currentGroupIsSkipped = false;

    std::vector<SortedResource<T, Ty>> sortedResources;
    SortedResource<T, Ty>::ComputeSortedResources(res, sortedResources);

    float regionWidth = ImGui::GetWindowContentRegionWidth();
    float currentStep = 0.f;
    float stepSize = (viewMode == 2) ? 64.f : 128.f;
    for (const auto& sortedRes : sortedResources)
    {
        unsigned int indexInRes = sortedRes.mIndex;
        bool selected = ((selection >> 16) == index) && (selection & 0xFFFF) == (int)indexInRes;
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Leaf |
                                        ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                        (selected ? ImGuiTreeNodeFlags_Selected : 0);

        std::string grp = GetGroup(res[indexInRes].mName);

        if (grp != currentGroup)
        {
            if (currentGroup.length() && !currentGroupIsSkipped)
                ImGui::TreePop();

            currentGroup = grp;
            currentStep = 0.f;
            if (currentGroup.length())
            {
                if (!ImGui::TreeNode(currentGroup.c_str()))
                {
                    currentGroupIsSkipped = true;
                    continue;
                }
            }
            currentGroupIsSkipped = false;
        }
        else if (currentGroupIsSkipped)
            continue;

        if ((regionWidth - currentStep) < stepSize)
        {
            currentStep = 0.f;
        }
        if (viewMode == 2 || viewMode == 3)
        {
            if (currentStep > FLT_EPSILON)
            {
                ImGui::SameLine();
            }
        }

        ImGui::BeginGroup();

        T& resource = res[indexInRes];
        imogen->DecodeThumbnailAsync(&resource);
        bool clicked = false;
        switch (viewMode)
        {
            case 0:
                ImGui::TreeNodeEx(GetName(resource.mName).c_str(), node_flags);
                clicked |= ImGui::IsItemClicked();
                break;
            case 1:
                ImGui::Image(
                    (ImTextureID)(int64_t)(resource.mThumbnailTextureId), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
                clicked = ImGui::IsItemClicked();
                ImGui::SameLine();
                ImGui::TreeNodeEx(GetName(resource.mName).c_str(), node_flags);
                clicked |= ImGui::IsItemClicked();
                break;
            case 2:
                ImGui::Image(
                    (ImTextureID)(int64_t)(resource.mThumbnailTextureId), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
                clicked = ImGui::IsItemClicked();
                break;
            case 3:
                ImGui::Image(
                    (ImTextureID)(int64_t)(resource.mThumbnailTextureId), ImVec2(128, 128), ImVec2(0, 1), ImVec2(1, 0));
                clicked = ImGui::IsItemClicked();
                break;
        }
        if (clicked)
        {
            selection = (index << 16) + indexInRes;
            ret = true;
        }
        ImGui::EndGroup();
        currentStep += stepSize;
    }

    if (currentGroup.length() && !currentGroupIsSkipped)
        ImGui::TreePop();

    ImGui::TreePop();
    return ret;
}

void ValidateMaterial(Library& library, GraphControler& nodeGraphControler, int materialIndex)
{
    if (materialIndex == -1)
        return;
    Material& material = library.mMaterials[materialIndex];
    const auto& model = nodeGraphControler.mModel;
    auto nodeCount = model.GetNodeCount();
    material.mMaterialNodes.resize(nodeCount);

    for (size_t i = 0; i < nodeCount; i++)
    {
        auto nodeType = model.GetNodeType(i);
        MaterialNode& dstNode = material.mMaterialNodes[i];
        MetaNode& metaNode = gMetaNodes[nodeType];
        dstNode.mRuntimeUniqueId = GetRuntimeId();
        if (metaNode.mbSaveTexture)
        {
            Image image;
            if (EvaluationAPI::GetEvaluationImage(&nodeGraphControler.mEditingContext, int(i), &image) == EVAL_OK)
            {
                g_TS.AddTaskSetToPipe(new EncodeImageTaskSet(image,
                                                             std::make_pair(materialIndex, material.mRuntimeUniqueId),
                                                             std::make_pair(i, dstNode.mRuntimeUniqueId)));
            }
        }

        dstNode.mType = uint32_t(nodeType);
        dstNode.mTypeName = metaNode.mName;
        dstNode.mParameters = model.GetParameters(i);
        dstNode.mInputSamplers = model.GetSamplers(i);
        ImVec2 nodePos = model.GetNodePos(i);
        dstNode.mPosX = int32_t(nodePos.x);
        dstNode.mPosY = int32_t(nodePos.y);
        int times[2];
        model.GetStartEndFrame(i, times[0], times[1]);
        dstNode.mFrameStart = uint32_t(times[0]); // todo serialize time as signed values
        dstNode.mFrameEnd = uint32_t(times[0]);
    }
    auto links = model.GetLinks();
    material.mMaterialConnections.resize(links.size());
    for (size_t i = 0; i < links.size(); i++)
    {
        MaterialConnection& materialConnection = material.mMaterialConnections[i];
        materialConnection.mInputNodeIndex = links[i].mInputNodeIndex;
        materialConnection.mInputSlotIndex = links[i].mInputSlotIndex;
        materialConnection.mOutputNodeIndex = links[i].mOutputNodeIndex;
        materialConnection.mOutputSlotIndex = links[i].mOutputSlotIndex;
    }
    auto rugs = model.GetRugs();
    material.mMaterialRugs.resize(rugs.size());
    for (size_t i = 0; i < rugs.size(); i++)
    {
        MaterialNodeRug& rug = material.mMaterialRugs[i];
        rug.mPosX = int32_t(rugs[i].mPos.x);
        rug.mPosY = int32_t(rugs[i].mPos.y);
        rug.mSizeX = int32_t(rugs[i].mSize.x);
        rug.mSizeY = int32_t(rugs[i].mSize.y);
        rug.mColor = rugs[i].mColor;
        rug.mComment = rugs[i].mText;
    }
    material.mAnimTrack = model.GetAnimTrack();
    model.GetStartEndFrame(material.mFrameMin, material.mFrameMax);
    material.mPinnedParameters = model.GetParameterPins();
    material.mPinnedIO = model.GetIOPins();
    material.mMultiplexInputs = model.GetMultiplexInputs();
    material.mBackgroundNode = *(uint32_t*)(&nodeGraphControler.mBackgroundNode);
}

int Imogen::AddNode(const std::string& nodeType)
{
    uint32_t type = uint32_t(GetMetaNodeIndex(nodeType));
    if (type == 0xFFFFFFFF)
    {
        return -1;
    }
    mNodeGraphControler->mModel.BeginTransaction(false);
    auto nodeIndex = mNodeGraphControler->mModel.AddNode(type, ImVec2(0, 0));
    mNodeGraphControler->mModel.SetStartEndFrame(nodeIndex, 0, 1);
    mNodeGraphControler->mModel.EndTransaction();
    return int(nodeIndex);
}

void Imogen::UpdateNewlySelectedGraph()
{
    // set new
    if (mSelectedMaterial != -1)
    {
        ClearAll();
        auto& model = mNodeGraphControler->mModel;
        model.BeginTransaction(false);
        Material& material = library.mMaterials[mSelectedMaterial];
        auto nodeCount = material.mMaterialNodes.size();
        for (size_t i = 0; i < nodeCount; i++)
        {
            MaterialNode& node = material.mMaterialNodes[i];
            if (node.mType == 0xFFFFFFFF)
                continue;

            auto nodeIndex = model.AddNode(node.mType, ImVec2(float(node.mPosX), float(node.mPosY)));
            model.SetParameters(nodeIndex, node.mParameters);
            model.SetStartEndFrame(nodeIndex, node.mFrameStart, node.mFrameEnd);
            model.SetSamplers(nodeIndex, node.mInputSamplers);

            /*auto& lastNode = mNodeGraphControler->mModel.GetEvaluationStages().mStages.back(); todo
            if (!node.mImage.empty())
            {
                mNodeGraphControler->mEditingContext.StageSetProcessing(i, true);
                g_TS.AddTaskSetToPipe(new DecodeImageTaskSet(
                    &node.mImage, std::make_pair(i, lastNode.mRuntimeUniqueId), mNodeGraphControler));
            }
            */
        }
        for (size_t i = 0; i < material.mMaterialConnections.size(); i++)
        {
            MaterialConnection& materialConnection = material.mMaterialConnections[i];
            model.AddLink(materialConnection.mInputNodeIndex,
                                                materialConnection.mInputSlotIndex,
                                                materialConnection.mOutputNodeIndex,
                                                materialConnection.mOutputSlotIndex);
        }
        for (size_t i = 0; i < material.mMaterialRugs.size(); i++)
        {
            MaterialNodeRug& rug = material.mMaterialRugs[i];
            model.AddRug({ImVec2(float(rug.mPosX), float(rug.mPosY)),
                                               ImVec2(float(rug.mSizeX), float(rug.mSizeY)),
                                               rug.mColor,
                                               rug.mComment});
        }
        material.mPinnedParameters.resize(model.GetNodeCount());
        material.mPinnedIO.resize(model.GetNodeCount());
        material.mMultiplexInputs.resize(model.GetNodeCount());


        model.SetStartEndFrame(material.mFrameMin, material.mFrameMax);
        model.SetAnimTrack(material.mAnimTrack);
        model.SetParameterPins(material.mPinnedParameters);
        model.SetIOPins(material.mPinnedIO);
        model.SetMultiplexInputs(material.mMultiplexInputs);
        model.EndTransaction();
        mNodeGraphControler->ApplyDirtyList();
        GraphEditorUpdateScrolling(mNodeGraphControler);

        mCurrentTime = 0;
        mbIsPlaying = false;
        mNodeGraphControler->mEditingContext.SetCurrentTime(mCurrentTime);

        mNodeGraphControler->mBackgroundNode = *(int*)(&material.mBackgroundNode);
        mNodeGraphControler->mEditingContext.SetMaterialUniqueId(material.mRuntimeUniqueId);

        mNodeGraphControler->mEditingContext.RunAll();
        //mNodeGraphControler->mModel.mEvaluationStages.SetTime(&mNodeGraphControler->mEditingContext, mCurrentTime, true);
        //mNodeGraphControler->mModel.mEvaluationStages.ApplyAnimation(&mNodeGraphControler->mEditingContext, mCurrentTime);

    }
}

void Imogen::NewMaterial(const std::string& materialName)
{
    int previousSelection = mSelectedMaterial;
    library.mMaterials.push_back(Material());
    Material& back = library.mMaterials.back();
    back.mName = materialName;
    back.mThumbnailTextureId = 0;
    back.mRuntimeUniqueId = GetRuntimeId();

    if (previousSelection != -1)
    {
        ValidateMaterial(library, *mNodeGraphControler, previousSelection);
    }
    mSelectedMaterial = int(library.mMaterials.size()) - 1;
    ClearAll();
}

void Imogen::ImportMaterial()
{
    #ifdef NFD_OpenDialog
    nfdchar_t* outPath = NULL;
    nfdresult_t result = NFD_OpenDialog("imogen", NULL, &outPath);

    if (result == NFD_OKAY)
    {
        Library tempLibrary;
        LoadLib(&tempLibrary, outPath);
        for (auto& material : tempLibrary.mMaterials)
        {
            Log("Importing Graph %s\n", material.mName.c_str());
            library.mMaterials.push_back(material);
        }
        free(outPath);
    }
    #endif
}

void Imogen::LibraryEdit(Library& library)
{
    int previousSelection = mSelectedMaterial;
    if (Button("MaterialNew",
               ICON_FA_PLUS " New Material",
               ImVec2(0, 0)))
    {
        NewMaterial();
    }
    ImGui::SameLine();
    if (Button("MaterialImport", "Import Material", ImVec2(0, 0))) // ImGui::Button("Import"))
    {
        ImportMaterial();
    }
    ImGui::SameLine();

    unsigned int libraryViewTextureId = gImageCache.GetTexture("Stock/library-view.png");
    static const ImVec2 iconSize(16.f, 16.f);
    for (int i = 0; i < 4; i++)
    {
        if (i)
            ImGui::SameLine();
        ImGui::PushID(99 + i);
        if (ImGui::ImageButton((ImTextureID)(int64_t)libraryViewTextureId,
                               iconSize,
                               ImVec2(float(i) * 0.25f, 0.f),
                               ImVec2(float(i + 1) * 0.25f, 1.f),
                               -1,
                               ImVec4(0.f, 0.f, 0.f, 0.f),
                               (i == mLibraryViewMode) ? ImVec4(1.f, 0.3f, 0.1f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f)))
            mLibraryViewMode = i;
        ImGui::PopID();
    }

    ImGui::BeginChild("TV");
    if (TVRes(library.mMaterials, "Graphs", mSelectedMaterial, 0, mLibraryViewMode, this))
    {
        mNodeGraphControler->mSelectedNodeIndex = -1;
        // save previous
        if (previousSelection != -1)
        {
            ValidateMaterial(library, *mNodeGraphControler, previousSelection);
        }
        UpdateNewlySelectedGraph();
    }
    ImGui::EndChild();
}

void Imogen::SetExistingMaterialActive(const char* materialName)
{
    Material* libraryMaterial = library.GetByName(materialName);
    if (libraryMaterial)
    {
        auto materialIndex = libraryMaterial - library.mMaterials.data();
        SetExistingMaterialActive(int(materialIndex));
    }
}

void Imogen::SetExistingMaterialActive(int materialIndex)
{
    if (materialIndex != mSelectedMaterial && mSelectedMaterial != -1)
    {
        ValidateMaterial(library, *mNodeGraphControler, mSelectedMaterial);
    }
    mSelectedMaterial = materialIndex;
    UpdateNewlySelectedGraph();
}

struct AnimCurveEdit : public ImCurveEdit::Delegate
{
    AnimCurveEdit(GraphControler& GraphControler,
                  ImVec2& min,
                  ImVec2& max,
                  std::vector<AnimTrack>& animTrack,
                  std::vector<bool>& visible,
                  int nodeIndex,
                  int currentTime)
        : mNodeGraphControler(GraphControler)
        , mMin(min)
        , mMax(max)
        , mAnimTrack(animTrack)
        , mbVisible(visible)
        , mNodeIndex(nodeIndex)
        , mCurrentTime(currentTime)
    {
        const auto& model = mNodeGraphControler.mModel;
        size_t type = model.GetNodeType(nodeIndex);
        const MetaNode& metaNode = gMetaNodes[type];
        std::vector<bool> parameterAddressed(metaNode.mParams.size(), false);

        auto curveForParameter = [&](int parameterIndex, ConTypes type, AnimationBase* animation) {
            const std::string& parameterName = metaNode.mParams[parameterIndex].mName;
            CurveType curveType = GetCurveTypeForParameterType(type);
            if (curveType == CurveNone)
                return;
            size_t curveCountPerParameter = GetCurveCountPerParameterType(type);
            parameterAddressed[parameterIndex] = true;
            for (size_t curveIndex = 0; curveIndex < curveCountPerParameter; curveIndex++)
            {
                std::vector<ImVec2> curvePts;
                if (!animation || animation->mFrames.empty())
                {
                    curvePts.resize(2);

                    int times[2];
                    model.GetStartEndFrame(mNodeIndex, times[0], times[1]);
                    float value = GetParameterComponentValue(
                        nodeIndex, model.GetParameters(nodeIndex),parameterIndex, int(curveIndex));
                    curvePts[0] = ImVec2(float(times[0]) + 0.5f, value);
                    curvePts[1] = ImVec2(float(times[1]) + 0.5f, value);
                }
                else
                {
                    size_t ptCount = animation->mFrames.size();
                    curvePts.resize(ptCount);
                    for (size_t i = 0; i < ptCount; i++)
                    {
                        curvePts[i] = ImVec2(float(animation->mFrames[i] + 0.5f),
                                             animation->GetFloatValue(uint32_t(i), int(curveIndex)));
                    }
                }
                mPts.push_back(curvePts);
                mValueType.push_back(type);
                mParameterIndex.push_back(parameterIndex);
                mComponentIndex.push_back(uint32_t(curveIndex));
                mCurveType.push_back(ImCurveEdit::CurveType(curveType));
                mLabels.push_back(parameterName + GetCurveParameterSuffix(type, int(curveIndex)));
                mColors.push_back(GetCurveParameterColor(type, int(curveIndex)));
            }
        };

        for (auto& track : mAnimTrack)
        {
            if (track.mNodeIndex != nodeIndex)
                continue;
            curveForParameter(track.mParamIndex, ConTypes(track.mValueType), track.mAnimation);
        }
        // non keyed parameters
        for (size_t param = 0; param < metaNode.mParams.size(); param++)
        {
            if (parameterAddressed[param])
                continue;
            curveForParameter(int(param), metaNode.mParams[param].mType, nullptr);
        }
    }

    void BakeValuesToAnimationTrack()
    {
        size_t type = mNodeGraphControler.mModel.GetNodeType(mNodeIndex);
        const MetaNode& metaNode = gMetaNodes[type];

        for (size_t curve = 0; curve < mPts.size();)
        {
            AnimTrack& animTrack = *mNodeGraphControler.mModel.GetAnimTrack(mNodeIndex, mParameterIndex[curve]);
            animTrack.mNodeIndex = mNodeIndex;
            animTrack.mParamIndex = mParameterIndex[curve];
            animTrack.mValueType = mValueType[curve];
            animTrack.mAnimation = AllocateAnimation(mValueType[curve]);
            size_t keyCount = mPts[curve].size();
            for (auto& pt : mPts[curve])
            {
                if (pt.x >= FLT_MAX)
                    keyCount--;
            }
            if (!keyCount)
                continue;
            auto anim = animTrack.mAnimation;
            anim->mFrames.resize(keyCount);
            anim->Allocate(keyCount);
            for (size_t key = 0, pointIndex = 0; key < keyCount; pointIndex++)
            {
                ImVec2 keyValue = mPts[curve][pointIndex];
                if (keyValue.x >= FLT_MAX)
                    continue;
                anim->mFrames[key] = uint32_t(keyValue.x);
                key++;
            }
            do
            {
                for (size_t key = 0, pointIndex = 0; key < keyCount; pointIndex++)
                {
                    ImVec2 keyValue = mPts[curve][pointIndex];
                    if (keyValue.x >= FLT_MAX)
                        continue;
                    anim->SetFloatValue(uint32_t(key), mComponentIndex[curve], keyValue.y);
                    key++;
                }
                curve++;
            } while (curve < mPts.size() && animTrack.mParamIndex == mParameterIndex[curve]);
        }
        /*mNodeGraphControler.mModel.ApplyAnimationForNode( todo
            &mNodeGraphControler.mEditingContext, mNodeIndex, mCurrentTime);*/
    }

    virtual ImCurveEdit::CurveType GetCurveType(size_t curveIndex) const
    {
        return mCurveType[curveIndex];
    }
    virtual ImVec2& GetMax()
    {
        return mMax;
    }
    virtual ImVec2& GetMin()
    {
        return mMin;
    }
    virtual unsigned int GetBackgroundColor()
    {
        return 0x80202060;
    }
    virtual bool IsVisible(size_t curveIndex)
    {
        return mbVisible[curveIndex];
    }
    size_t GetCurveCount()
    {
        return mPts.size();
    }
    size_t GetPointCount(size_t curveIndex)
    {
        return mPts[curveIndex].size();
    }
    uint32_t GetCurveColor(size_t curveIndex)
    {
        return mColors[curveIndex];
    }
    ImVec2* GetPoints(size_t curveIndex)
    {
        return mPts[curveIndex].data();
    }

    virtual int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value)
    {
        uint32_t parameterIndex = mParameterIndex[curveIndex];

        mPts[curveIndex][pointIndex] = value;
        for (size_t curve = 0; curve < mParameterIndex.size(); curve++)
        {
            if (mParameterIndex[curve] == parameterIndex)
            {
                mPts[curve][pointIndex].x = value.x;
            }
        }

        SortValues(curveIndex);
        BakeValuesToAnimationTrack();

        for (size_t i = 0; i < GetPointCount(curveIndex); i++)
        {
            if (mPts[curveIndex][i].x == value.x)
                return int(i);
        }
        return pointIndex;
    }

    virtual void AddPoint(size_t curveIndex, ImVec2 value)
    {
        uint32_t parameterIndex = mParameterIndex[curveIndex];

        /*AnimTrack* animTrack = gNodeDelegate.GetAnimTrack(gNodeDelegate.mSelectedNodeIndex, parameterIndex);
        unsigned char *tmp = new unsigned char[GetParameterTypeSize(ConTypes(mValueType[curveIndex]))];
        animTrack->mAnimation->GetValue(uint32_t(value.x), tmp);
        */
        for (size_t curve = 0; curve < mParameterIndex.size(); curve++)
        {
            if (mParameterIndex[curve] == parameterIndex)
            {
                mPts[curve].push_back(value);
            }
        }
        for (size_t curve = 0; curve < mParameterIndex.size(); curve++)
        {
            if (mParameterIndex[curve] == parameterIndex)
            {
                SortValues(curve);
            }
        }
        // delete[] tmp;
    }

    void DeletePoints(const ImVector<ImCurveEdit::EditPoint>& points)
    {
        // tag point
        for (int i = 0; i < points.size(); i++)
        {
            auto& selPoint = points[i];
            uint32_t parameterIndex = mParameterIndex[selPoint.curveIndex];
            for (size_t curve = 0; curve < mParameterIndex.size(); curve++)
            {
                if (mParameterIndex[curve] == parameterIndex)
                {
                    mPts[curve][selPoint.pointIndex].x = FLT_MAX;
                }
            }
        }
        BakeValuesToAnimationTrack();
    }

    virtual void BeginEdit(int /*index*/)
    {
        for (size_t curve = 0; curve < mParameterIndex.size(); curve++)
        {
            uint32_t parameterIndex = mParameterIndex[curve];
            bool parameterFound = false;
            int index = 0;
            for (auto& track : mNodeGraphControler.mModel.GetAnimTrack())
            {
                if (track.mNodeIndex == mNodeGraphControler.mSelectedNodeIndex && track.mParamIndex == parameterIndex)
                {
                    parameterFound = true;
                    break;
                }
                index++;
            }
            if (!parameterFound)
            {
                AnimTrack animTrack;
                animTrack.mNodeIndex = mNodeIndex;
                animTrack.mParamIndex = mParameterIndex[curve];
                animTrack.mValueType = mValueType[curve];
                animTrack.mAnimation = AllocateAnimation(mValueType[curve]);
                mAnimTrack.push_back(animTrack);
            }
        }
    }

    virtual void EndEdit()
    {
    }

    void MakeCurvesVisible()
    {
        float minY = FLT_MAX;
        float maxY = -FLT_MAX;
        bool somethingVisible = false;
        for (size_t curve = 0; curve < mPts.size(); curve++)
        {
            if (!mbVisible[curve])
                continue;
            somethingVisible = true;
            std::vector<ImVec2>& pts = mPts[curve];
            for (auto& pt : pts)
            {
                minY = ImMin(minY, pt.y);
                maxY = ImMax(maxY, pt.y);
            }
        }
        if (somethingVisible)
        {
            if ((maxY - minY) <= FLT_EPSILON)
            {
                mMin.y = minY - 0.1f;
                mMax.y = minY + 0.1f;
            }
            else
            {
                float marge = (maxY - minY) * 0.05f;
                mMin.y = minY - marge;
                mMax.y = maxY + marge;
            }
        }
    }

    std::vector<std::vector<ImVec2>> mPts;
    std::vector<std::string> mLabels;
    std::vector<uint32_t> mColors;
    std::vector<AnimTrack>& mAnimTrack;
    std::vector<uint32_t> mValueType;
    std::vector<uint32_t> mParameterIndex;
    std::vector<uint32_t> mComponentIndex;
    std::vector<ImCurveEdit::CurveType> mCurveType;
    std::vector<bool>& mbVisible;

    ImVec2 &mMin, &mMax;
    int mNodeIndex;
    int mCurrentTime;
    GraphControler& mNodeGraphControler;

private:
    void SortValues(size_t curveIndex)
    {
        auto b = std::begin(mPts[curveIndex]);
        auto e = std::end(mPts[curveIndex]);
        std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });
        BakeValuesToAnimationTrack();
    }
};

struct MySequence : public ImSequencer::SequenceInterface
{
    MySequence(GraphControler& GraphControler)
        : mNodeGraphControler(GraphControler), setKeyFrameOrValue(FLT_MAX, FLT_MAX)
    {
    }

    void Clear()
    {
        mbExpansions.clear();
        mbVisible.clear();
        mLastCustomDrawIndex = -1;
        setKeyFrameOrValue.x = setKeyFrameOrValue.y = FLT_MAX;
        mCurveMin = 0.f;
        mCurveMax = 1.f;
    }

    void SetCurrentTime(int currentTime)
    {
        mCurrentTime = currentTime;
    }
    virtual int GetFrameMin() const
    {
        int startFrame, endFrame;
        mNodeGraphControler.mModel.GetStartEndFrame(startFrame, endFrame);
        return startFrame;
    }
    virtual int GetFrameMax() const
    {
        int startFrame, endFrame;
        mNodeGraphControler.mModel.GetStartEndFrame(startFrame, endFrame);
        return endFrame;
    }

    virtual void BeginEdit(int index)
    {
    }
    virtual void EndEdit()
    {
        // todo
        //mNodeGraphControler.mModel.mEvaluationStages.SetTime(&mNodeGraphControler.mEditingContext, mCurrentTime, false);
    }

    virtual int GetItemCount() const
    {
        return (int)mNodeGraphControler.mModel.GetNodeCount();
    }

    virtual int GetItemTypeCount() const
    {
        return 0;
    }
    virtual const char* GetItemTypeName(int typeIndex) const
    {
        return NULL;
    }
    virtual const char* GetItemLabel(int index) const
    {
        size_t nodeType = mNodeGraphControler.mModel.GetNodeType(index);
        return gMetaNodes[nodeType].mName.c_str();
    }

    virtual void Get(int index, int** start, int** end, int* type, unsigned int* color)
    {
        const auto& model = mNodeGraphControler.mModel;
        size_t nodeType = model.GetNodeType(index);
        int times[2];
        model.GetStartEndFrame(index, times[0], times[1]);
        if (color)
            *color = gMetaNodes[nodeType].mHeaderColor;
        if (start)
            *start = &times[0];
        if (end)
            *end = &times[1];
        if (type)
            *type = int(nodeType);
    }

    virtual void Add(int type){};
    virtual void Del(int index)
    {
    }
    virtual void Duplicate(int index)
    {
    }

    virtual size_t GetCustomHeight(int index)
    {
        if (index >= mbExpansions.size())
            return false;
        return mbExpansions[index] ? 300 : 0;
    }
    virtual void DoubleClick(int index)
    {
        if (index >= mbExpansions.size())
            mbExpansions.resize(index + 1, false);
        if (mbExpansions[index])
        {
            mbExpansions[index] = false;
            return;
        }
        for (auto i = 0;i<mbExpansions.size();i++)
        {
            mbExpansions[i] = false;
        }
        mbExpansions[index] = !mbExpansions[index];
    }

    virtual void CustomDraw(int index,
                            ImDrawList* draw_list,
                            const ImRect& rc,
                            const ImRect& legendRect,
                            const ImRect& clippingRect,
                            const ImRect& legendClippingRect)
    {
        if (mLastCustomDrawIndex != index)
        {
            mLastCustomDrawIndex = index;
            mbVisible.clear();
        }

        int startFrame, endFrame;
        mNodeGraphControler.mModel.GetStartEndFrame(startFrame, endFrame);

        ImVec2 curveMin(float(startFrame), mCurveMin);
        ImVec2 curveMax(float(endFrame), mCurveMax);
        AnimCurveEdit curveEdit(mNodeGraphControler,
                                curveMin,
                                curveMax,
                                const_cast<std::vector<AnimTrack>&>(mNodeGraphControler.mModel.GetAnimTrack()),
                                mbVisible,
                                index,
                                mCurrentTime);
        mbVisible.resize(curveEdit.GetCurveCount(), true);
        draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);
        for (int i = 0; i < curveEdit.GetCurveCount(); i++)
        {
            ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
            ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i + 1) * 14.f);
            uint32_t curveColor = curveEdit.mColors[i];
            draw_list->AddText(
                pta, mbVisible[i] ? curveColor : ((curveColor & 0xFFFFFF) + 0x80000000), curveEdit.mLabels[i].c_str());
            if (ImRect(pta, ptb).Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(0))
                mbVisible[i] = !mbVisible[i];
        }
        draw_list->PopClipRect();

        ImGui::SetCursorScreenPos(rc.Min);
        ImCurveEdit::Edit(curveEdit, rc.Max - rc.Min, 137 + index, &clippingRect, &mSelectedCurvePoints);
        getKeyFrameOrValue = ImVec2(FLT_MAX, FLT_MAX);

        if (focused || curveEdit.focused)
        {
            if (ImGui::IsKeyPressedMap(ImGuiKey_Delete))
            {
                curveEdit.DeletePoints(mSelectedCurvePoints);
            }
            if (ImGui::IsKeyPressed(SDL_SCANCODE_F))
            {
                curveEdit.MakeCurvesVisible();
            }
        }

        if (setKeyFrameOrValue.x < FLT_MAX || setKeyFrameOrValue.y < FLT_MAX)
        {
            curveEdit.BeginEdit(0);
            for (int i = 0; i < mSelectedCurvePoints.size(); i++)
            {
                auto& selPoint = mSelectedCurvePoints[i];
                int keyIndex = selPoint.pointIndex;
                ImVec2 keyValue = curveEdit.mPts[selPoint.curveIndex][keyIndex];
                uint32_t parameterIndex = curveEdit.mParameterIndex[selPoint.curveIndex];
                AnimTrack* animTrack =
                    mNodeGraphControler.mModel.GetAnimTrack(mNodeGraphControler.mSelectedNodeIndex, parameterIndex);
                // UndoRedo *undoRedo = nullptr;
                // undoRedo = new URChange<AnimTrack>(int(animTrack - gNodeDelegate.GetAnimTrack().data()), [](int
                // index) { return &gNodeDelegate.mAnimTrack[index]; });

                if (setKeyFrameOrValue.x < FLT_MAX)
                {
                    keyValue.x = setKeyFrameOrValue.x;
                }
                else
                {
                    keyValue.y = setKeyFrameOrValue.y;
                }
                curveEdit.EditPoint(selPoint.curveIndex, selPoint.pointIndex, keyValue);
                // delete undoRedo;
            }
            curveEdit.EndEdit();
            setKeyFrameOrValue.x = setKeyFrameOrValue.y = FLT_MAX;
        }
        if (mSelectedCurvePoints.size() == 1)
        {
            getKeyFrameOrValue = curveEdit.mPts[mSelectedCurvePoints[0].curveIndex][mSelectedCurvePoints[0].pointIndex];
        }

        // set back visible bounds
        mCurveMin = curveMin.y;
        mCurveMax = curveMax.y;
    }

    GraphControler& mNodeGraphControler;
    std::vector<bool> mbExpansions;
    std::vector<bool> mbVisible;
    ImVector<ImCurveEdit::EditPoint> mSelectedCurvePoints;
    int mLastCustomDrawIndex;
    ImVec2 setKeyFrameOrValue;
    ImVec2 getKeyFrameOrValue;
    float mCurveMin, mCurveMax;
    int mCurrentTime;
};

std::vector<ImHotKey::HotKey> mHotkeys;

void Imogen::Init(bool bDebugWindow)
{
    mbDebugWindow = bDebugWindow;
    SetStyle();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    DiscoverNodes("glsl", "Nodes/GLSL/", EVALUATOR_GLSL, mEvaluatorFiles);
    //DiscoverNodes("c", "Nodes/C/", EVALUATOR_C, mEvaluatorFiles);
    //DiscoverNodes("py", "Nodes/Python/", EVALUATOR_PYTHON, mEvaluatorFiles);
    //DiscoverNodes("glsl", "Nodes/GLSLCompute/", EVALUATOR_GLSLCOMPUTE, mEvaluatorFiles);
    //DiscoverNodes("glslc", "Nodes/GLSLCompute/", EVALUATOR_GLSLCOMPUTE, mEvaluatorFiles);

    struct HotKeyFunction
    {
        const char* name;
        const char* lib;
        std::function<void()> function;
    };
    static const std::vector<HotKeyFunction> hotKeyFunctions = {
        {"Layout", "Reorder nodes in a simpler layout", [&]() { GetNodeGraphControler()->mModel.NodeGraphLayout(GetNodeGraphControler()->mEvaluationStages.GetForwardEvaluationOrder()); }},
        {"PlayPause", "Play or Stop current animation", [&]() { PlayPause(); }},
        {"AnimationFirstFrame",
         "Set current time to the first frame of animation",
         [&]() { 
            int startFrame, endFrame;
            GetNodeGraphControler()->mModel.GetStartEndFrame(startFrame, endFrame);
            mCurrentTime = startFrame;
         }},
        {"AnimationNextFrame", "Move to the next animation frame", [&]() { mCurrentTime++; }},
        {"AnimationPreviousFrame", "Move to previous animation frame", [&]() { mCurrentTime--; }},
        {"MaterialExport", "Export current material to a file", [&]() { ExportMaterial(); }},
        {"MaterialImport", "Import a material file in the library", [&]() { ImportMaterial(); }},
        {"ToggleLibrary", "Show or hide Libaray window", [&]() { mbShowLibrary = !mbShowLibrary; }},
        {"ToggleNodeGraph", "Show or hide Node graph window", [&]() { mbShowNodes = !mbShowNodes; }},
        {"ToggleLogger", "Show or hide Logger window", [&]() { mbShowLog = !mbShowLog; }},
        {"ToggleSequencer", "Show or hide Sequencer window", [&]() { mbShowTimeline = !mbShowTimeline; }},
        {"ToggleParameters", "Show or hide Parameters window", [&]() { mbShowParameters = !mbShowParameters; }},
        {"MaterialNew", "Create a new graph", [&]() { NewMaterial(); }},
        {"ReloadShaders",
         "Reload them",
         [&]() {
             gEvaluators.SetEvaluators(mEvaluatorFiles);
             mNodeGraphControler->mEditingContext.RunAll();
         }},
        {"DeleteSelectedNodes", "Delete selected nodes in the current graph", []() {}},
        {"AnimationSetKey",
         "Make a new animation key with the current parameters values at the current time",
         [&]() { mNodeGraphControler->mModel.MakeKey(mCurrentTime, uint32_t(mNodeGraphControler->mSelectedNodeIndex), 0); }},
        {"HotKeyEditor", "Open the Hotkey editor window", [&]() { ImGui::OpenPopup("HotKeys Editor"); }},
        {"NewNodePopup", "Open the new node menu", []() {}},
        {"Undo", "Undo the last operation", []() { Imogen::instance->GetNodeGraphControler()->mModel.Undo(); }},
        {"Redo", "Redo the last undo", []() { Imogen::instance->GetNodeGraphControler()->mModel.Redo(); }},
        {"Copy", "Copy the selected nodes", []() {}},
        {"Cut", "Cut the selected nodes", []() {}},
        {"Paste", "Paste previously copy/cut nodes", []() {}},
        {"BuildMaterial", "Build current material", [&]() { BuildCurrentMaterial(mBuilder); }},
        {"MouseState", "Show Mouse State as a tooltip", [&]() { mbShowMouseState = !mbShowMouseState; }}};

    mHotkeys.reserve(hotKeyFunctions.size());
    mHotkeyFunctions.reserve(hotKeyFunctions.size());
    for (const auto& hotKeyFunction : hotKeyFunctions)
    {
        mHotkeys.push_back({hotKeyFunction.name, hotKeyFunction.lib});
        mHotkeyFunctions.push_back(hotKeyFunction.function);
    }
}

void Imogen::Finish()
{
}

const char* GetShortCutLib(const char* functionName)
{
    static char tmps[512];
    for (int i = 0; i < mHotkeys.size(); i++)
    {
        if (!strcmp(mHotkeys[i].functionName, functionName))
        {
            ImHotKey::GetHotKeyLib(mHotkeys[i].functionKeys, tmps, sizeof(tmps), mHotkeys[i].functionName);
            return tmps;
        }
    }
    return "*FUNCTION_ERROR*";
}

void Imogen::HandleHotKeys()
{
    int hotkeyIndex = ImHotKey::GetHotKey(mHotkeys.data(), mHotkeys.size());
    if (hotkeyIndex == -1)
        return;
    if (ImGui::IsAnyItemActive())
        return;
    mHotkeyFunctions[hotkeyIndex]();
}

static const ImVec2 deltaHeight = ImVec2(0, 32);

void Imogen::RunDeferedCommands()
{
    if (!mRunCommand.size())
        return;
    std::string tmpCommand = mRunCommand;
    mRunCommand = "";
    #if USE_PYTHON
    try
    {
        pybind11::exec(tmpCommand);
    }
    catch (pybind11::error_already_set& ex)
    {
        Log(ex.what());
    }
    #endif
}

void Imogen::ShowAppMainMenuBar()
{
    ImGuiIO& io = ImGui::GetIO();

    static const ImVec2 buttonSize(440, 30);

    mMainMenuPos = ImLerp(mMainMenuPos, mMainMenuDest, 0.2f);

    ImGui::SetNextWindowSize(ImVec2(440, io.DisplaySize.y - deltaHeight.y));
    ImGui::SetNextWindowPos(ImVec2(mMainMenuPos, deltaHeight.y));
    if (!ImGui::BeginPopupModal("MainMenu", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
    {
        mMainMenuPos = -440;
        return;
    }

    if (ImGui::CollapsingHeader("Plugins", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Button("Reload Plugins", buttonSize))
        {
            Evaluators::ReloadPlugins();
        }
        ImGui::Separator();
        for (auto& plugin : mRegisteredPlugins)
        {
            if (ImGui::Button(plugin.mName.c_str(), buttonSize))
            {
                mRunCommand = plugin.mPythonCommand;
            }
        }
    }

    if (ImGui::CollapsingHeader("Graph", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (Button("Layout", "Layout", buttonSize))
        {
            GetNodeGraphControler()->mModel.NodeGraphLayout(GetNodeGraphControler()->mEvaluationStages.GetForwardEvaluationOrder());
        }
        if (Button("MaterialExport", "Export Material", buttonSize))
        {
        }
        if (Button("MaterialImport", "Import Material", buttonSize))
        {
        }
    }

    if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (Button("HotKeyEditor", "Hot Keys Editor", buttonSize))
        {
            mNewPopup = "HotKeys Editor";
        }
    }

    if (ImGui::CollapsingHeader("Windows", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox(GetShortCutLib("ToggleLibrary"), &mbShowLibrary);
        ImGui::Checkbox(GetShortCutLib("ToggleNodeGraph"), &mbShowNodes);
        ImGui::Checkbox(GetShortCutLib("ToggleLogger"), &mbShowLog);
        ImGui::Checkbox(GetShortCutLib("ToggleSequencer"), &mbShowTimeline);
        ImGui::Checkbox(GetShortCutLib("ToggleParameters"), &mbShowParameters);
    }

    ImRect windowRect(ImVec2(0, 32), ImVec2(440, io.DisplaySize.y - 32));
    if ((io.MouseClicked[0] && !windowRect.Contains(io.MousePos) && !ImGui::IsPopupOpen("HotKeys Editor")))
    {
        mMainMenuDest = -440;
    }
    if (mMainMenuPos <= -439.f && mMainMenuDest < -400.f)
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void Imogen::BuildCurrentMaterial(Builder* builder)
{
    if (mSelectedMaterial != -1)
    {
        Material& material = library.mMaterials[mSelectedMaterial];
        builder->Add(material.mName.c_str(), mNodeGraphControler->mEvaluationStages);
    }
}

int Imogen::GetFunctionByName(const char* functionName) const
{
    for (unsigned int i = 0; i < mHotkeys.size(); i++)
    {
        const auto& hotkey = mHotkeys[i];
        if (!strcmp(hotkey.functionName, functionName))
            return i;
    }
    return -1;
}

bool Imogen::ImageButton(const char* functionName, unsigned int icon, ImVec2 size)
{
    bool res = ImGui::ImageButton((ImTextureID)(int64_t)icon, size, ImVec2(0, 1), ImVec2(1, 0));
    if (ImGui::IsItemHovered())
    {
        int functionIndex = GetFunctionByName(functionName);
        if (functionIndex != -1)
        {
            char lib[512];
            auto& hotkey = mHotkeys[functionIndex];
            ImHotKey::GetHotKeyLib(hotkey.functionKeys, lib, sizeof(lib));
            ImGui::BeginTooltip();
            ImGui::Text("%s [%s]", hotkey.functionLib, lib);
            ImGui::EndTooltip();
        }
    }
    return res;
}

bool Imogen::Button(const char* functionName, const char* label, ImVec2 size)
{
    bool res = ImGui::Button(label, size);
    if (ImGui::IsItemHovered())
    {
        int functionIndex = GetFunctionByName(functionName);
        if (functionIndex != -1)
        {
            char lib[512];
            auto& hotkey = mHotkeys[functionIndex];
            ImHotKey::GetHotKeyLib(hotkey.functionKeys, lib, sizeof(lib));
            ImGui::BeginTooltip();
            ImGui::Text("%s [%s]", hotkey.functionLib, lib);
            ImGui::EndTooltip();
        }
    }
    return res;
}

void Imogen::ShowTitleBar(Builder* builder)
{
    ImGuiIO& io = ImGui::GetIO();

    if (mNewPopup)
    {
        ImGui::OpenPopup(mNewPopup);
        mNewPopup = nullptr;
    }


    ImVec2 butSize = ImVec2(32, 32);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 0.f) + deltaHeight);

    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    ImGui::PushStyleColor(ImGuiCol_Button, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0x80808080);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
    ImGui::Begin("TitleBar",
                 0,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PushID(149);
    if (ImGui::Button("", butSize))
    {
        if (ImGui::IsPopupOpen("MainMenu"))
        {
            mMainMenuDest = -440.f;
        }
        else
        {
            mMainMenuDest = 0.f;
            ImGui::OpenPopup("MainMenu");
        }
    }
    ShowAppMainMenuBar();
    ImRect rcMenu(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    ImGui::PopID();
    ImGui::SameLine();
    // imogen info strings
    ImGui::BeginChildFrame(152, ImVec2(io.DisplaySize.x - butSize.x * 4.f - 300, 32.f));
    ImGui::Text("Imogen 0.14");
    if (mSelectedMaterial != -1)
    {
        Material& material = library.mMaterials[mSelectedMaterial];
        ImGui::Text("%s", material.mName.c_str());
    }
    ImGui::EndChildFrame();
    ImGui::SameLine();

    // exporting frame / build
    unsigned int buildIcon = gImageCache.GetTexture("Stock/Build.png");
    if (ImageButton("BuildMaterial", buildIcon, ImVec2(30, 30)))
    {
        BuildCurrentMaterial(builder);
    }
    ImGui::SameLine();
    ImGui::BeginChildFrame(153, ImVec2(300.f, 32));
    static std::vector<Builder::BuildInfo> buildInfos;
    builder->UpdateBuildInfo(buildInfos);

    if (!buildInfos.empty())
    {
        auto& bi = buildInfos[0];
        ImGui::Text("Processing %s", bi.mName.c_str());
        ImGui::ProgressBar(bi.mProgress);
    }
    ImGui::EndChildFrame();
    if (ImGui::IsItemHovered() && (buildInfos.size() > 1))
    {
        ImGui::BeginTooltip();
        for (auto& bi : buildInfos)
        {
            ImGui::Text("%s", bi.mName.c_str());
        }
        ImGui::EndTooltip();
    }
    /*
    ImGui::SameLine();
    // min/max/close buttons
    ImGui::PushID(150);
    if (ImGui::Button("", butSize))
    {
        extern SDL_Window* window;
        SDL_MinimizeWindow(window);
    }
    ImRect rcMin(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    ImGui::PopID();
    ImGui::PushID(151);
    ImGui::SameLine();
    if (ImGui::Button("", butSize))
    {
        extern SDL_Window* window;
        SDL_MaximizeWindow(window);
    }
    ImRect rcMax(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    ImGui::PopID();
    ImGui::PushID(152);
    ImGui::SameLine();
    if (ImGui::Button("", butSize))
    {
        SDL_Event sdlevent;
        sdlevent.type = SDL_QUIT;
        SDL_PushEvent(&sdlevent);
    }
    ImRect rcQuit(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    ImGui::PopID();
    */
    // custom draw icons
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    uint32_t butCol = 0xFFE0E0E0;
    drawList->AddRectFilled(rcMenu.Min + ImVec2(8, 8), rcMenu.Min + ImVec2(24, 12), butCol);
    drawList->AddRectFilled(rcMenu.Min + ImVec2(8, 14), rcMenu.Min + ImVec2(24, 18), butCol);
    drawList->AddRectFilled(rcMenu.Min + ImVec2(8, 20), rcMenu.Min + ImVec2(24, 24), butCol);
    /*
    drawList->AddRectFilled(rcMin.Min + ImVec2(12, 18), rcMin.Max - ImVec2(12, 12), butCol);
    drawList->AddRectFilled(rcMax.Min + ImVec2(12, 12), rcMax.Max - ImVec2(12, 12), butCol);
    drawList->AddLine(rcQuit.Min + ImVec2(12, 12), rcQuit.Min + ImVec2(20, 20), butCol, 2.f);
    drawList->AddLine(rcQuit.Min + ImVec2(12, 20), rcQuit.Min + ImVec2(20, 12), butCol, 2.f);
    */
    ImGui::End();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(5);


    // done
}

void Imogen::PlayPause()
{
    if (!mbIsPlaying)
    {
        int startFrame, endFrame;
        auto& model = mNodeGraphControler->mModel;
        model.GetStartEndFrame(startFrame, endFrame);
        mCurrentTime = startFrame;
    }
    mbIsPlaying = !mbIsPlaying;
}

void Imogen::ShowTimeLine()
{
    int selectedEntry = mNodeGraphControler->mSelectedNodeIndex;
    static int firstFrame = 0;

    mSequence->SetCurrentTime(mCurrentTime);

    int startFrame, endFrame;
    auto& model = mNodeGraphControler->mModel;
    model.GetStartEndFrame(startFrame, endFrame);

    ImGui::PushItemWidth(80);
    ImGui::PushID(200);
    bool dirtyFrame = ImGui::InputInt("", &startFrame, 0, 0);
    ImGui::PopID();
    ImGui::SameLine();
    if (Button("AnimationFirstFrame", "|<", ImVec2(0, 0)))
    {
        mCurrentTime = startFrame;
    }
    ImGui::SameLine();
    if (Button("AnimationPreviousFrame", "<", ImVec2(0, 0)))
    {
        mCurrentTime--;
    }
    ImGui::SameLine();

    ImGui::PushID(201);
    if (ImGui::InputInt("", &mCurrentTime, 0, 0, 0))
    {
    }
    ImGui::PopID();

    ImGui::SameLine();
    if (Button("AnimationNextFrame", ">", ImVec2(0, 0)))
    {
        mCurrentTime++;
    }
    ImGui::SameLine();
    if (ImGui::Button(">|"))
    {
        mCurrentTime = endFrame;
    }
    ImGui::SameLine();

    if (Button("PlayPause", mbIsPlaying ? "Stop" : "Play", ImVec2(0, 0)))
    {
        PlayPause();
    }

    unsigned int playNoLoopTextureId = gImageCache.GetTexture("Stock/PlayNoLoop.png");
    unsigned int playLoopTextureId = gImageCache.GetTexture("Stock/PlayLoop.png");

    ImGui::SameLine();
    if (ImGui::ImageButton((ImTextureID)(uint64_t)(mbPlayLoop ? playLoopTextureId : playNoLoopTextureId),
                           ImVec2(16.f, 16.f)))
    {
        mbPlayLoop = !mbPlayLoop;
    }

    ImGui::SameLine();
    ImGui::PushID(202);
    dirtyFrame |= ImGui::InputInt("", &endFrame, 0, 0);

    if (dirtyFrame)
    {
        model.BeginTransaction(true);
        model.SetStartEndFrame(startFrame, endFrame);
        model.EndTransaction();
    }
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::SameLine(0, 40.f);
    if (Button("AnimationSetKey", "Make Key", ImVec2(0, 0)) && selectedEntry != -1)
    {
        mNodeGraphControler->mModel.MakeKey(mCurrentTime, uint32_t(selectedEntry), 0);
    }

    ImGui::SameLine(0, 50.f);

    int setf = (mSequence->getKeyFrameOrValue.x < FLT_MAX) ? int(mSequence->getKeyFrameOrValue.x) : 0;
    ImGui::PushID(203);
    if (ImGui::InputInt("", &setf, 0, 0))
    {
        mSequence->setKeyFrameOrValue.x = float(setf);
    }
    ImGui::SameLine();
    float setv = (mSequence->getKeyFrameOrValue.y < FLT_MAX) ? mSequence->getKeyFrameOrValue.y : 0.f;
    if (ImGui::InputFloat("Key", &setv))
    {
        mSequence->setKeyFrameOrValue.y = setv;
    }
    ImGui::PopID();
    ImGui::SameLine();
    int timeMask[2] = {0, 0};
    if (selectedEntry != -1)
    {
        mNodeGraphControler->mModel.GetStartEndFrame(selectedEntry, timeMask[0], timeMask[1]);
    }
    ImGui::PushItemWidth(120);
    if (ImGui::InputInt2("Time Mask", timeMask) && selectedEntry != -1)
    {
        timeMask[1] = ImMax(timeMask[1], timeMask[0]);
        timeMask[0] = ImMin(timeMask[1], timeMask[0]);
        mNodeGraphControler->mModel.SetStartEndFrame(selectedEntry, timeMask[0], timeMask[1]);
    }
    ImGui::PopItemWidth();
    ImGui::PopItemWidth();

    Sequencer(mSequence,
              &mCurrentTime,
              NULL,
              &selectedEntry,
              &firstFrame,
              ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_CHANGE_FRAME);
    if (selectedEntry != -1)
    {
        auto& model = mNodeGraphControler->mModel;
        mNodeGraphControler->mSelectedNodeIndex = selectedEntry;
        model.SelectNode(selectedEntry, true);
        int times[2];
        model.GetStartEndFrame(selectedEntry, times[0], times[1]);
        /*mNodeGraphControler->mModel.mEvaluationStages.SetStageLocalTime( todo
            &mNodeGraphControler->mEditingContext,
            selectedEntry,
            ImClamp(mCurrentTime - times[0], 0, times[1] - times[0]),
            true);*/
    }
}

void Imogen::DeleteCurrentMaterial()
{
    library.mMaterials.erase(library.mMaterials.begin() + mSelectedMaterial);
    mSelectedMaterial = int(library.mMaterials.size()) - 1;
    UpdateNewlySelectedGraph();
}

void Imogen::ShowNodeGraph()
{
    if (mSelectedMaterial != -1)
    {
        Material& material = library.mMaterials[mSelectedMaterial];
        ImGui::PushItemWidth(400);
        ImGui::InputText("Name", &material.mName);
        ImGui::SameLine();
        ImGui::PopItemWidth();

        ImGui::PushItemWidth(60);
        if (Button("MaterialExport", "Export Material", ImVec2(0, 0)))
        {
            ExportMaterial();
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete Material"))
        {
            DeleteCurrentMaterial();
        }
        ImGui::PopItemWidth();
    }
    GraphEditor(mNodeGraphControler, mSelectedMaterial != -1);
}

void Imogen::ExportMaterial()
{
    #ifdef NFD_SaveDialog
    nfdchar_t* outPath = NULL;
    nfdresult_t result = NFD_SaveDialog("imogen", NULL, &outPath);

    if (result == NFD_OKAY)
    {
        Library tempLibrary;
        Material& material = library.mMaterials[mSelectedMaterial];
        tempLibrary.mMaterials.push_back(material);
        SaveLib(&tempLibrary, outPath);
        Log("Graph %s saved at path %s\n", material.mName.c_str(), outPath);
        free(outPath);
    }
    #endif
}
void Imogen::ShowDebugWindow()
{
    ImGui::Begin("Debug");
    if (ImGui::CollapsingHeader("Thumbnails Atlas"))
    {
        auto atlases = this->GetNodeGraphControler()->mEditingContext.GetThumbnails().GetAtlasTextures();
        for (auto& atlas : atlases)
        {
            ImGui::Text("Atlas %d", int(&atlas - atlases.data()));
            ImGui::Image((ImTextureID)(int64_t)atlas.mGLTexID, ImVec2(1024, 1024));
        }
    }
    ImGui::End();
}

ImRect GetNodesDisplayRect(GraphModel* model);
ImRect GetFinalNodeDisplayRect(GraphModel* model);
std::map<std::string, ImRect> interfacesRect;
void Imogen::Show(Builder* builder, Library& library, bool capturing)
{
    int currentTime = mCurrentTime;
    ImGuiIO& io = ImGui::GetIO();
    mBuilder = builder;
    if (!capturing)
    {
        ShowTitleBar(builder);
    }
    ImGui::SetNextWindowPos(deltaHeight);
    ImGui::SetNextWindowSize(io.DisplaySize - deltaHeight);
    ImVec2 nodesWindowPos;

    if (ImGui::Begin("Imogen",
                     0,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
        ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);

        if (mbShowNodes)
        {
            if (ImGui::Begin("Nodes", &mbShowNodes))
            {
                ShowNodeGraph();
            }
            nodesWindowPos = ImGui::GetWindowPos();
            interfacesRect["Nodes"] = ImRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
            ImGui::End();
        }

        if (mbShowLibrary)
        {
            if (ImGui::Begin("Library", &mbShowLibrary))
            {
                LibraryEdit(library);
            }
            interfacesRect["Library"] = ImRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
            ImGui::End();
        }

        if (mbShowParameters)
        {
            if (ImGui::Begin("Parameters", &mbShowParameters))
            {
                mNodeGraphControler->NodeEdit();
            }
            interfacesRect["Parameters"] =
                ImRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
            ImGui::End();
        }

        if (mbShowLog)
        {
            if (ImGui::Begin("Logs", &mbShowLog))
            {
                ImguiAppLog::Log->DrawEmbedded();
            }
            interfacesRect["Logs"] = ImRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
            ImGui::End();
        }

        if (mbShowTimeline)
        {
            if (ImGui::Begin("Timeline", &mbShowTimeline))
            {
                ShowTimeLine();
            }
            interfacesRect["Timeline"] = ImRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
            ImGui::End();
        }

        // view extraction
        int index = 0;
        int removeExtractedView = -1;
        for (auto& extraction : mExtratedViews)
        {
            char tmps[512];
            sprintf(
                tmps,
                "%s_View_%03d",
                gMetaNodes[mNodeGraphControler->mModel.GetNodeType(extraction.mNodeIndex)].mName.c_str(),
                index);
            bool open = true;
            if (extraction.mFirstFrame)
            {
                extraction.mFirstFrame = false;
                ImGui::SetNextWindowSize(ImVec2(256, 256));
            }
            if (ImGui::Begin(tmps, &open))
            {
                RenderPreviewNode(int(extraction.mNodeIndex), *mNodeGraphControler, true);
            }
            ImGui::End();
            if (!open)
            {
                removeExtractedView = index;
            }
            index++;
        }
        if (removeExtractedView != -1)
            mExtratedViews.erase(mExtratedViews.begin() + removeExtractedView);

        ImGui::End();
    }

    if (mbDebugWindow)
    {
        ShowDebugWindow();
    }

    ImHotKey::Edit(mHotkeys.data(), mHotkeys.size(), "HotKeys Editor");

    Playback(currentTime != mCurrentTime);

    ImRect rc = mNodeGraphControler->mModel.GetNodesDisplayRect();
    interfacesRect["Graph"] = ImRect(nodesWindowPos + rc.Min, nodesWindowPos + rc.Max);
    rc = mNodeGraphControler->mModel.GetFinalNodeDisplayRect(mNodeGraphControler->mEvaluationStages.GetForwardEvaluationOrder());
    interfacesRect["FinalNode"] = ImRect(nodesWindowPos + rc.Min, nodesWindowPos + rc.Max);
}

void Imogen::Playback(bool timeHasChanged)
{
    if (mbIsPlaying)
    {
        mCurrentTime++;
        int startFrame, endFrame;
        mNodeGraphControler->mModel.GetStartEndFrame(startFrame, endFrame);
        if (mCurrentTime >= endFrame)
        {
            if (mbPlayLoop)
            {
                mCurrentTime = startFrame;
            }
            else
            {
                mbIsPlaying = false;
            }
        }
    }

    mNodeGraphControler->mEditingContext.SetCurrentTime(mCurrentTime);

    if (timeHasChanged || mbIsPlaying)
    {
        /*mNodeGraphControler->mModel.mEvaluationStages.SetTime(&mNodeGraphControler->mEditingContext, mCurrentTime, true); todo
        mNodeGraphControler->mModel.mEvaluationStages.ApplyAnimation(&mNodeGraphControler->mEditingContext, mCurrentTime);
        */
    }
}

void Imogen::ValidateCurrentMaterial(Library& library)
{
    ValidateMaterial(library, *mNodeGraphControler, mSelectedMaterial);
}

void Imogen::DiscoverNodes(const char* extension,
                           const char* directory,
                           EVALUATOR_TYPE evaluatorType,
                           std::vector<EvaluatorFile>& files)
{
    tinydir_dir dir;
    tinydir_open(&dir, directory);

    while (dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&dir, &file);

        if (!file.is_dir && !strcmp(file.extension, extension))
        {
            files.push_back({directory, file.name, evaluatorType});
        }

        tinydir_next(&dir);
    }

    tinydir_close(&dir);
}

struct readHelper
{
    Imogen* imogen;
};
static readHelper rhelper;

void* Imogen::ReadOpen(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name)
{
    std::string tag(name);

    if (tag.substr(0, 6) == "Imogen")
    {
        rhelper.imogen = Imogen::instance;
    }

    return (void*)&rhelper;
}

void Imogen::ReadLine(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line_start)
{
    readHelper* userdata = (readHelper*)entry;

    if (userdata)
    {
        int active;

        if (sscanf(line_start, "ShowTimeline=%d", &active) == 1)
        {
            userdata->imogen->mbShowTimeline = active != 0;
        }
        else if (sscanf(line_start, "ShowLibrary=%d", &active) == 1)
        {
            userdata->imogen->mbShowLibrary = active != 0;
        }
        else if (sscanf(line_start, "ShowNodes=%d", &active) == 1)
        {
            userdata->imogen->mbShowNodes = active != 0;
        }
        else if (sscanf(line_start, "ShowLog=%d", &active) == 1)
        {
            userdata->imogen->mbShowLog = active != 0;
        }
        else if (sscanf(line_start, "ShowParameters=%d", &active) == 1)
        {
            userdata->imogen->mbShowParameters = active != 0;
        }
        else if (sscanf(line_start, "LibraryViewMode=%d", &active) == 1)
        {
            userdata->imogen->mLibraryViewMode = active;
        }
        else if (sscanf(line_start, "ShowMouseState=%d", &active) == 1)
        {
            userdata->imogen->mbShowMouseState = active;
        }
        else
        {
            for (auto& hotkey : mHotkeys)
            {
                unsigned int functionKeys;
                char tmps[128];
                sprintf(tmps, "%s=0x%%x", hotkey.functionName);
                if (sscanf(line_start, tmps, &functionKeys) == 1)
                {
                    hotkey.functionKeys = functionKeys;
                }
            }
        }
    }
}

void Imogen::WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    buf->appendf("[%s][Imogen]\n", handler->TypeName);
    buf->appendf("ShowTimeline=%d\n", instance->mbShowTimeline ? 1 : 0);
    buf->appendf("ShowLibrary=%d\n", instance->mbShowLibrary ? 1 : 0);
    buf->appendf("ShowNodes=%d\n", instance->mbShowNodes ? 1 : 0);
    buf->appendf("ShowLog=%d\n", instance->mbShowLog ? 1 : 0);
    buf->appendf("ShowParameters=%d\n", instance->mbShowParameters ? 1 : 0);
    buf->appendf("ShowMouseState=%d\n", instance->mbShowMouseState ? 1 : 0);
    buf->appendf("LibraryViewMode=%d\n", instance->mLibraryViewMode);

    for (const auto& hotkey : mHotkeys)
    {
        buf->appendf("%s=0x%x\n", hotkey.functionName, hotkey.functionKeys);
    }
}

Imogen::Imogen(GraphControler* nodeGraphControler) : mNodeGraphControler(nodeGraphControler)
{
    mSequence = new MySequence(*nodeGraphControler);
    mdConfig.userData = this;

    ImGuiContext& g = *GImGui;
    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "Imogen";
    ini_handler.TypeHash = ImHashStr("Imogen", 0, 0);
    ini_handler.ReadOpenFn = ReadOpen;
    ini_handler.ReadLineFn = ReadLine;
    ini_handler.WriteAllFn = WriteAll;
    g.SettingsHandlers.push_front(ini_handler);
    instance = this;
}

Imogen::~Imogen()
{
    delete mSequence;
    instance = nullptr;
}

void Imogen::ClearAll()
{
    mNodeGraphControler->Clear();
    GraphEditorClear();
    InitCallbackRects();
    ClearExtractedViews();
    mSequence->Clear();
}
