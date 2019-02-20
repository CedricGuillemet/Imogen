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

#include <SDL.h>
#include "imgui.h"
#include "Imogen.h"
#include "TextEditor.h"
#include <fstream>
#include <streambuf>
#include "Evaluation.h"
#include "NodesDelegate.h"
#include "Library.h"
#include "TaskScheduler.h"
#include "stb_image.h"
#include "tinydir.h"
#include "stb_image.h"
#include "imgui_stdlib.h"
#include "ImSequencer.h"
#include "Evaluators.h"
#include "nfd.h"
#include "UI.h"

unsigned char *stbi_write_png_to_mem(unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len);
extern Evaluation gEvaluation;
int gEvaluationTime = 0;
extern enki::TaskScheduler g_TS;
extern bool gbIsPlaying;
void ClearAll(TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation);



struct ExtractedView
{
    size_t mNodeIndex;
    bool mFirstFrame;
};
std::vector<ExtractedView> mExtratedViews;
void AddExtractedView(size_t nodeIndex)
{
    mExtratedViews.push_back({ nodeIndex, true });
}
void ClearExtractedViews()
{
    mExtratedViews.clear();
}

TextEditor editor;

void Imogen::HandleEditor(TextEditor &editor, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
    static int currentShaderIndex = -1;

    if (currentShaderIndex == -1)
    {
        currentShaderIndex = 0;
        editor.SetText(gEvaluators.GetEvaluator(mEvaluatorFiles[currentShaderIndex].mFilename));
    }
    auto cpos = editor.GetCursorPosition();
    ImGui::BeginChild(13, ImVec2(250, 800));
    for (size_t i = 0; i < mEvaluatorFiles.size(); i++)
    {
        bool selected = i == currentShaderIndex;
        if (ImGui::Selectable(mEvaluatorFiles[i].mFilename.c_str(), &selected))
        {
            currentShaderIndex = int(i);
            editor.SetText(gEvaluators.GetEvaluator(mEvaluatorFiles[currentShaderIndex].mFilename));
            editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates());
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild(14);
    // save
    if (ImGui::IsKeyReleased(SDL_SCANCODE_F5))
    {
        auto textToSave = editor.GetText();

        std::ofstream t(mEvaluatorFiles[currentShaderIndex].mDirectory + mEvaluatorFiles[currentShaderIndex].mFilename, std::ofstream::out);
        t << textToSave;
        t.close();

        gEvaluators.SetEvaluators(mEvaluatorFiles);
        gCurrentContext->RunAll();
    }

    ImGui::SameLine();
    ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | F5 to save and update nodes", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
        editor.IsOverwrite() ? "Ovr" : "Ins",
        editor.CanUndo() ? "*" : " ",
        editor.GetLanguageDefinition().mName.c_str());
    editor.Render("TextEditor");
    ImGui::EndChild();

}

void RenderPreviewNode(int selNode, TileNodeEditGraphDelegate& nodeGraphDelegate, Evaluation& evaluation, bool forceUI = false)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF000000);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF000000);
    ImGui::PushStyleColor(ImGuiCol_Button, 0xFF000000);
    float w = ImGui::GetWindowContentRegionWidth();
    int imageWidth(1), imageHeight(1);

    // make 2 evaluation for node to get the UI pass image size
    if (selNode != -1 && nodeGraphDelegate.NodeHasUI(selNode))
    {
        gCurrentContext->AllocRenderTargetsForEditingPreview();
        EvaluationInfo evaluationInfo;
        evaluationInfo.forcedDirty = 1;
        evaluationInfo.uiPass = 1;
        gCurrentContext->RunSingle(selNode, evaluationInfo);
    }
    Evaluation::GetEvaluationSize(selNode, &imageWidth, &imageHeight);
    if (selNode != -1 && nodeGraphDelegate.NodeHasUI(selNode))
    {
        EvaluationInfo evaluationInfo;
        evaluationInfo.forcedDirty = 1;
        evaluationInfo.uiPass = 0;
        gCurrentContext->RunSingle(selNode, evaluationInfo);
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
            //draw_list->AddCallback((ImDrawCallback)(Evaluation::NodeUICallBack), (void*)(AddNodeUICallbackRect(CBUI_Node, rc, selNode)));
            AddUICustomDraw(draw_list, rc, Evaluation::DrawUISingle, selNode);

            mouseUVCoord = (io.MousePos - rc.Min) / rc.GetSize();
            mouseUVCoord.y = 1.f - mouseUVCoord.y;
        }
        else
        {
            if (selNode != -1 && nodeGraphDelegate.NodeIsCubemap(selNode))
            {
                ImGui::InvisibleButton("ImTheInvisibleMan", ImVec2(w, h));
            }
            else
            {
                displayedTexture = (ImTextureID)(int64_t)((selNode != -1) ? nodeGraphDelegate.mEditingContext.GetEvaluationTexture(selNode) : 0);
                if (displayedTexture)
                {
                    auto tgt = nodeGraphDelegate.mEditingContext.GetRenderTarget(selNode);
                    displayedTextureSize = ImVec2(float(tgt->mImage.mWidth), float(tgt->mImage.mHeight));
                }
                ImVec2 mouseUVPos = (io.MousePos - p) / ImVec2(w, h);
                mouseUVPos.y = 1.f - mouseUVPos.y;
                Vec4 mouseUVPosv(mouseUVPos.x, mouseUVPos.y);
                mouseUVCoord = ImVec2(mouseUVPosv.x, mouseUVPosv.y);

                Vec4 uva(0, 0), uvb(1, 1);
                Mat4x4* viewMatrix = nodeGraphDelegate.GetParameterViewMatrix(selNode);
                Camera *nodeCamera = nodeGraphDelegate.GetCameraParameter(selNode);
                if (viewMatrix && !nodeCamera)
                {
                    Mat4x4& res = *viewMatrix;
                    Mat4x4 tr, trp, sc;
                    static float scale = 1.f;
                    scale = ImLerp(scale, 1.f, 0.15f);
                    if (ImRect(p, p + ImVec2(w, h)).Contains(io.MousePos) && ImGui::IsWindowFocused())
                    {
                        scale -= io.MouseWheel*0.04f;
                        scale -= io.MouseDelta.y * 0.01f * ((io.KeyAlt&&io.MouseDown[1]) ? 1.f : 0.f);

                        ImVec2 pix2uv = ImVec2(1.f, 1.f) / ImVec2(w, h);
                        Vec4 localTranslate;
                        localTranslate = Vec4(-io.MouseDelta.x * pix2uv.x, io.MouseDelta.y * pix2uv.y) * ((io.KeyAlt&&io.MouseDown[2]) ? 1.f : 0.f) * res[0];
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

                ImGui::ImageButton(displayedTexture, ImVec2(w, h), ImVec2(uva.x, uvb.y), ImVec2(uvb.x, uva.y));
            }
            rc = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            if (selNode != -1 && nodeGraphDelegate.NodeIsCubemap(selNode))
            {
                AddUICustomDraw(draw_list, rc, Evaluation::DrawUICubemap, selNode);
            }
            else if (selNode != -1 && nodeGraphDelegate.NodeHasUI(selNode))
            {
                AddUICustomDraw(draw_list, rc, Evaluation::DrawUISingle, selNode);
            }
        }
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(1);

    static int lastSentExit = -1;
    if (rc.Contains(io.MousePos))
    {
        static Image pickerImage;
        if (io.KeyShift && io.MouseDown[0] && displayedTexture && selNode > -1 && mouseUVCoord.x >= 0.f && mouseUVCoord.y >= 0.f)
        {
            if (!pickerImage.GetBits())
            {
                Evaluation::GetEvaluationImage(selNode, &pickerImage);
                Log("Texel view Get image\n");
            }
            int width = pickerImage.mWidth;
            int height = pickerImage.mHeight;
            
            ImageZoomTooltip(width, height, pickerImage.GetBits(), mouseUVCoord, displayedTextureSize);
        }
        else if (ImGui::IsWindowFocused())
        {
            Evaluation::FreeImage(&pickerImage);
            ImVec2 ratio((io.MousePos.x - rc.Min.x) / rc.GetSize().x, (io.MousePos.y - rc.Min.y) / rc.GetSize().y);
            ImVec2 deltaRatio((io.MouseDelta.x) / rc.GetSize().x, (io.MouseDelta.y) / rc.GetSize().y);
            nodeGraphDelegate.SetMouse(ratio.x, ratio.y, deltaRatio.x, deltaRatio.y, io.MouseDown[0], io.MouseDown[1], io.MouseWheel);
        }
        lastSentExit = -1;
    }
    else
    {
        if (lastSentExit != selNode)
        {
            lastSentExit = selNode;
            nodeGraphDelegate.SetMouse(-9999.f, -9999.f, -9999.f, -9999.f, false, false, 0.f);
        }
    }
}

void NodeEdit(TileNodeEditGraphDelegate& nodeGraphDelegate, Evaluation& evaluation)
{
    ImGuiIO& io = ImGui::GetIO();

    int selNode = nodeGraphDelegate.mSelectedNodeIndex;
    if (ImGui::CollapsingHeader("Preview", 0, ImGuiTreeNodeFlags_DefaultOpen))
    {
        RenderPreviewNode(selNode, nodeGraphDelegate, evaluation);
    }

    if (selNode == -1)
        ImGui::CollapsingHeader("No Selection", 0, ImGuiTreeNodeFlags_DefaultOpen);
    else
        nodeGraphDelegate.EditNode();
}

template <typename T, typename Ty> struct SortedResource
{
    SortedResource() {}
    SortedResource(unsigned int index, const std::vector<T, Ty>* res) : mIndex(index), mRes(res) {}
    SortedResource(const SortedResource& other) : mIndex(other.mIndex), mRes(other.mRes) {}
    void operator = (const SortedResource& other) { mIndex = other.mIndex; mRes = other.mRes; }
    unsigned int mIndex;
    const std::vector<T, Ty>* mRes;
    bool operator < (const SortedResource& other) const
    {
        return (*mRes)[mIndex].mName<(*mRes)[other.mIndex].mName;
    }

    static void ComputeSortedResources(const std::vector<T, Ty>& res, std::vector<SortedResource>& sortedResources)
    {
        sortedResources.resize(res.size());
        for (unsigned int i = 0; i < res.size(); i++)
            sortedResources[i] = SortedResource<T, Ty>(i, &res);
        std::sort(sortedResources.begin(), sortedResources.end());
    }
};

struct PinnedTaskUploadImage : enki::IPinnedTask
{
    PinnedTaskUploadImage(Image *image, ASyncId identifier, bool isThumbnail)
        : enki::IPinnedTask(0) // set pinned thread to 0
        , mImage(image)
        , mIdentifier(identifier)
        , mbIsThumbnail(isThumbnail)
    {
    }

    virtual void Execute()
    {
        unsigned int textureId = Evaluation::UploadImage(mImage, 0);
        if (mbIsThumbnail)
        {
            Material* material = library.Get(mIdentifier);
            if (material)
                material->mThumbnailTextureId = textureId;
        }
        else
        {
            TileNodeEditGraphDelegate::ImogenNode *node = gNodeDelegate.Get(mIdentifier);
            size_t nodeIndex = node - gNodeDelegate.mNodes.data();
            if (node)
            {
                Evaluation::SetEvaluationImage(int(nodeIndex), mImage);
                gEvaluation.SetEvaluationParameters(nodeIndex, node->mParameters);
                gCurrentContext->StageSetProcessing(nodeIndex, false);
            }
            Evaluation::FreeImage(mImage);
        }
    }
    Image *mImage;
    ASyncId mIdentifier;
    bool mbIsThumbnail;
};

struct DecodeThumbnailTaskSet : enki::ITaskSet
{
    DecodeThumbnailTaskSet(std::vector<uint8_t> *src, ASyncId identifier) : enki::ITaskSet(), mIdentifier(identifier), mSrc(src)
    {
    }
    virtual void    ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum)
    {
        Image image;
        int components;
        unsigned char *data = stbi_load_from_memory(mSrc->data(), int(mSrc->size()), &image.mWidth, &image.mHeight, &components, 0);
        if (data)
        {
            image.SetBits(data, image.mWidth * image.mHeight * components);
            image.mNumFaces = 1;
            image.mNumMips = 1;
            image.mFormat = (components == 4) ? TextureFormat::RGBA8 : TextureFormat::RGB8;
            PinnedTaskUploadImage uploadTexTask(&image, mIdentifier, true);
            g_TS.AddPinnedTask(&uploadTexTask);
            g_TS.WaitforTask(&uploadTexTask);
            Evaluation::FreeImage(&image);
        }
        delete this;
    }
    ASyncId mIdentifier;
    std::vector<uint8_t> *mSrc;
};

struct EncodeImageTaskSet : enki::ITaskSet
{
    EncodeImageTaskSet(Image image, ASyncId materialIdentifier, ASyncId nodeIdentifier) : enki::ITaskSet(), mMaterialIdentifier(materialIdentifier), mNodeIdentifier(nodeIdentifier), mImage(image)
    {
    }
    virtual void    ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum)
    {
        int outlen;
        int components = 4;
        unsigned char *bits = stbi_write_png_to_mem((unsigned char*)mImage.GetBits(), mImage.mWidth * components, mImage.mWidth, mImage.mHeight, components, &outlen);
        if (bits)
        {
            Material *material = library.Get(mMaterialIdentifier);
            if (material)
            {
                MaterialNode *node = material->Get(mNodeIdentifier);
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

struct DecodeImageTaskSet : enki::ITaskSet
{
    DecodeImageTaskSet(std::vector<uint8_t> *src, ASyncId identifier) : enki::ITaskSet(), mIdentifier(identifier), mSrc(src)
    {
    }
    virtual void    ExecuteRange(enki::TaskSetPartition range, uint32_t threadnum)
    {
        Image image;
        int components;
        unsigned char *data = stbi_load_from_memory(mSrc->data(), int(mSrc->size()), &image.mWidth, &image.mHeight, &components, 0);
        if (data)
        {
            image.SetBits(data, image.mWidth * image.mHeight * components);
            image.mNumFaces = 1;
            image.mNumMips = 1;
            image.mFormat = (components == 3) ? TextureFormat::RGB8 : TextureFormat::RGBA8;
            PinnedTaskUploadImage uploadTexTask(&image, mIdentifier, false);
            g_TS.AddPinnedTask(&uploadTexTask);
            g_TS.WaitforTask(&uploadTexTask);
        }
        delete this;
    }
    ASyncId mIdentifier;
    std::vector<uint8_t> *mSrc;
};

void DecodeThumbnailAsync(Material * material)
{
    static unsigned int defaultTextureId = gEvaluation.GetTexture("Stock/thumbnail-icon.png");
    if (!material->mThumbnailTextureId)
    {
        material->mThumbnailTextureId = defaultTextureId;
        g_TS.AddTaskSetToPipe(new DecodeThumbnailTaskSet(&material->mThumbnail, std::make_pair(material-library.mMaterials.data(), material->mRuntimeUniqueId)));
    }
}

template <typename T, typename Ty> bool TVRes(std::vector<T, Ty>& res, const char *szName, int &selection, int index, Evaluation& evaluation, int viewMode)
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
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (selected ? ImGuiTreeNodeFlags_Selected : 0);

        std::string grp = GetGroup(res[indexInRes].mName);

        if ( grp != currentGroup)
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
        DecodeThumbnailAsync(&resource);
        bool clicked = false;
        switch (viewMode)
        {
        case 0:
            ImGui::TreeNodeEx(GetName(resource.mName).c_str(), node_flags);
            clicked |= ImGui::IsItemClicked();
            break;
        case 1:
            ImGui::Image((ImTextureID)(int64_t)(resource.mThumbnailTextureId), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
            clicked = ImGui::IsItemClicked();
            ImGui::SameLine();
            ImGui::TreeNodeEx(GetName(resource.mName).c_str(), node_flags);
            clicked |= ImGui::IsItemClicked();
            break;
        case 2:
            ImGui::Image((ImTextureID)(int64_t)(resource.mThumbnailTextureId), ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
            clicked = ImGui::IsItemClicked();
            break;
        case 3:
            ImGui::Image((ImTextureID)(int64_t)(resource.mThumbnailTextureId), ImVec2(128, 128), ImVec2(0, 1), ImVec2(1, 0));
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

static int selectedMaterial = -1;
int Imogen::GetCurrentMaterialIndex()
{
    return selectedMaterial;
}
void ValidateMaterial(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate, int materialIndex)
{
    if (materialIndex == -1)
        return;
    Material& material = library.mMaterials[materialIndex];
    material.mMaterialNodes.resize(nodeGraphDelegate.mNodes.size());

    for (size_t i = 0; i < nodeGraphDelegate.mNodes.size(); i++)
    {
        TileNodeEditGraphDelegate::ImogenNode srcNode = nodeGraphDelegate.mNodes[i];
        MaterialNode &dstNode = material.mMaterialNodes[i];
        MetaNode& metaNode = gMetaNodes[srcNode.mType];
        dstNode.mRuntimeUniqueId = GetRuntimeId();
        if (metaNode.mbSaveTexture)
        {
            Image image;
            if (Evaluation::GetEvaluationImage(int(i), &image) == EVAL_OK)
            {
                g_TS.AddTaskSetToPipe(new EncodeImageTaskSet(image, std::make_pair(materialIndex, material.mRuntimeUniqueId), std::make_pair(i, dstNode.mRuntimeUniqueId)));
            }
        }

        dstNode.mType = uint32_t(srcNode.mType);
        dstNode.mTypeName = metaNode.mName;
        dstNode.mParameters = srcNode.mParameters;
        dstNode.mInputSamplers = srcNode.mInputSamplers;
        ImVec2 nodePos = NodeGraphGetNodePos(i);
        dstNode.mPosX = int32_t(nodePos.x);
        dstNode.mPosY = int32_t(nodePos.y);
        dstNode.mFrameStart = srcNode.mStartFrame;
        dstNode.mFrameEnd = srcNode.mEndFrame;
    }
    auto links = NodeGraphGetLinks();
    material.mMaterialConnections.resize(links.size());
    for (size_t i = 0; i < links.size(); i++)
    {
        MaterialConnection& materialConnection = material.mMaterialConnections[i];
        materialConnection.mInputNode = links[i].InputIdx;
        materialConnection.mInputSlot = links[i].InputSlot;
        materialConnection.mOutputNode = links[i].OutputIdx;
        materialConnection.mOutputSlot = links[i].OutputSlot;
    }
    auto rugs = NodeGraphRugs();
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
    material.mAnimTrack = nodeGraphDelegate.GetAnimTrack();
    material.mFrameMin = nodeGraphDelegate.mFrameMin;
    material.mFrameMax = nodeGraphDelegate.mFrameMax;
    material.mPinnedParameters = nodeGraphDelegate.mPinnedParameters;
}

void UpdateNewlySelectedGraph(TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
    // set new
    if (selectedMaterial != -1)
    {
        ClearAll(nodeGraphDelegate, evaluation);

        Material& material = library.mMaterials[selectedMaterial];
        for (size_t i = 0; i < material.mMaterialNodes.size(); i++)
        {
            MaterialNode& node = material.mMaterialNodes[i];
            NodeGraphAddNode(&nodeGraphDelegate, node.mType, node.mParameters, node.mPosX, node.mPosY, node.mFrameStart, node.mFrameEnd);
            TileNodeEditGraphDelegate::ImogenNode& lastNode = nodeGraphDelegate.mNodes.back();
            if (!node.mImage.empty())
            {
                gCurrentContext->StageSetProcessing(i, true);
                g_TS.AddTaskSetToPipe(new DecodeImageTaskSet(&node.mImage, std::make_pair(i, lastNode.mRuntimeUniqueId)));
            }
            lastNode.mInputSamplers = node.mInputSamplers;
            gEvaluation.SetEvaluationSampler(i, node.mInputSamplers);
        }
        for (size_t i = 0; i < material.mMaterialConnections.size(); i++)
        {
            MaterialConnection& materialConnection = material.mMaterialConnections[i];
            NodeGraphAddLink(&nodeGraphDelegate, materialConnection.mInputNode, materialConnection.mInputSlot, materialConnection.mOutputNode, materialConnection.mOutputSlot);
        }
        for (size_t i = 0; i < material.mMaterialRugs.size(); i++)
        {
            MaterialNodeRug& rug = material.mMaterialRugs[i];
            NodeGraphAddRug(rug.mPosX, rug.mPosY, rug.mSizeX, rug.mSizeY, rug.mColor, rug.mComment);
        }
        NodeGraphUpdateEvaluationOrder(&nodeGraphDelegate);
        NodeGraphUpdateScrolling();
        gEvaluationTime = 0;
        gbIsPlaying = false;
        nodeGraphDelegate.SetAnimTrack(material.mAnimTrack);
        nodeGraphDelegate.mFrameMin = material.mFrameMin;
        nodeGraphDelegate.mFrameMax = material.mFrameMax;
        nodeGraphDelegate.mPinnedParameters = material.mPinnedParameters;
        nodeGraphDelegate.SetTime(gEvaluationTime, true);
        nodeGraphDelegate.ApplyAnimation(gEvaluationTime);
        nodeGraphDelegate.mEditingContext.RunAll();
    }
}

void LibraryEdit(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
    int previousSelection = selectedMaterial;
    if (ImGui::Button("New Graph"))
    {
        library.mMaterials.push_back(Material());
        Material& back = library.mMaterials.back();
        back.mName = "Name_Of_New_Graph";
        back.mThumbnailTextureId = 0;
        back.mRuntimeUniqueId = GetRuntimeId();
        
        if (previousSelection != -1)
        {
            ValidateMaterial(library, nodeGraphDelegate, previousSelection);
        }
        selectedMaterial = int(library.mMaterials.size()) - 1;
        ClearAll(nodeGraphDelegate, evaluation);
    }
    ImGui::SameLine();
    if (ImGui::Button("Import"))
    {
        nfdchar_t *outPath = NULL;
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
    }
    ImGui::SameLine();
    static int libraryViewMode = 1;
    unsigned int libraryViewTextureId = evaluation.GetTexture("Stock/library-view.png");
    static const ImVec2 iconSize(16.f, 16.f);
    for (int i = 0; i < 4; i++)
    {
        if (i)
            ImGui::SameLine();
        ImGui::PushID(99 + i);
        if (ImGui::ImageButton((ImTextureID)(int64_t)libraryViewTextureId, iconSize, ImVec2(float(i)*0.25f, 0.f), ImVec2(float(i + 1)*0.25f, 1.f), -1, ImVec4(0.f, 0.f, 0.f, 0.f)
            , (i == libraryViewMode) ? ImVec4(1.f, 0.3f, 0.1f, 1.f) : ImVec4(1.f, 1.f, 1.f, 1.f)))
            libraryViewMode = i;
        ImGui::PopID();
    }

    ImGui::BeginChild("TV");
    if (TVRes(library.mMaterials, "Graphs", selectedMaterial, 0, evaluation, libraryViewMode))
    {
        nodeGraphDelegate.mSelectedNodeIndex = -1;
        // save previous
        if (previousSelection != -1)
        {
            ValidateMaterial(library, nodeGraphDelegate, previousSelection);
        }
        UpdateNewlySelectedGraph(nodeGraphDelegate, evaluation);
    }
    ImGui::EndChild();
}

void SetExistingMaterialActive(const char * materialName)
{
    Material* libraryMaterial = library.GetByName(materialName);
    if (libraryMaterial)
    {
        auto materialIndex = libraryMaterial - library.mMaterials.data();
        SetExistingMaterialActive(int(materialIndex));
    }
}

void SetExistingMaterialActive(int materialIndex)
{
    if (materialIndex != selectedMaterial && selectedMaterial != -1)
    {
        ValidateMaterial(library, gNodeDelegate, selectedMaterial);
    }
    selectedMaterial = materialIndex;
    UpdateNewlySelectedGraph(gNodeDelegate, gEvaluation);
}

struct AnimCurveEdit : public ImCurveEdit::Delegate
{
    AnimCurveEdit(ImVec2& min, ImVec2& max, std::vector<AnimTrack>& animTrack, std::vector<bool>& visible, int nodeIndex) :
        mMin(min), mMax(max), mAnimTrack(animTrack), mbVisible(visible), mNodeIndex(nodeIndex)
    {
        size_t type = gNodeDelegate.mNodes[nodeIndex].mType;
        const MetaNode& metaNode = gMetaNodes[type];
        std::vector<bool> parameterAddressed(metaNode.mParams.size(), false);

        auto curveForParameter = [&](int parameterIndex, ConTypes type, AnimationBase * animation) {
            const std::string & parameterName = metaNode.mParams[parameterIndex].mName;
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
                    auto& node = gNodeDelegate.mNodes[mNodeIndex];
                    float value = gNodeDelegate.GetParameterComponentValue(nodeIndex, parameterIndex, int(curveIndex));
                    curvePts[0] = ImVec2(float(node.mStartFrame) + 0.5f, value);
                    curvePts[1] = ImVec2(float(node.mEndFrame) + 0.5f, value);
                }
                else
                {
                    size_t ptCount = animation->mFrames.size();
                    curvePts.resize(ptCount);
                    for (size_t i = 0; i < ptCount; i++)
                    {
                        curvePts[i] = ImVec2(float(animation->mFrames[i] + 0.5f), animation->GetFloatValue(uint32_t(i), int(curveIndex)));
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
        size_t type = gNodeDelegate.mNodes[mNodeIndex].mType;
        const MetaNode& metaNode = gMetaNodes[type];

        for (size_t curve = 0; curve < mPts.size(); )
        {
            AnimTrack &animTrack = *gNodeDelegate.GetAnimTrack(mNodeIndex, mParameterIndex[curve]);
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
        gNodeDelegate.ApplyAnimationForNode(mNodeIndex, gEvaluationTime);
    }

    virtual ImCurveEdit::CurveType GetCurveType(size_t curveIndex) const { return mCurveType[curveIndex]; }
    virtual ImVec2& GetMax() { return mMax; }
    virtual ImVec2& GetMin() { return mMin; }
    virtual unsigned int GetBackgroundColor() { return 0x80202060; }
    virtual bool IsVisible(size_t curveIndex) { return mbVisible[curveIndex]; }
    size_t GetCurveCount() { return mPts.size(); }
    size_t GetPointCount(size_t curveIndex) { return mPts[curveIndex].size(); }
    uint32_t GetCurveColor(size_t curveIndex) { return mColors[curveIndex]; }
    ImVec2* GetPoints(size_t curveIndex) { return mPts[curveIndex].data(); }

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
        //delete[] tmp;
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

    static std::vector<UndoRedo *> undoRedoEditCurves;
    virtual void BeginEdit(int /*index*/)
    {
        assert(gUndoRedoHandler.mCurrent == nullptr);
        URDummy urDummy;
       
        for (size_t curve = 0;curve< mParameterIndex.size();curve++)
        {
            uint32_t parameterIndex = mParameterIndex[curve];
            bool parameterFound = false;
            int index = 0;
            for (auto& track : gNodeDelegate.mAnimTrack)
            {
                if (track.mNodeIndex == gNodeDelegate.mSelectedNodeIndex && track.mParamIndex == parameterIndex)
                {
                    undoRedoEditCurves.push_back(new URChange<AnimTrack>(index, [](int index) { return &gNodeDelegate.mAnimTrack[index]; }));
                    parameterFound = true;
                    break;
                }
                index++;
            }
            if (!parameterFound)
            {
                undoRedoEditCurves.push_back(new URAdd<AnimTrack>(index, []() { return &gNodeDelegate.mAnimTrack; }));
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
        for (auto ur : undoRedoEditCurves)
            delete ur;
        undoRedoEditCurves.clear();
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

private:
    void SortValues(size_t curveIndex)
    {
        auto b = std::begin(mPts[curveIndex]);
        auto e = std::end(mPts[curveIndex]);
        std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });
        BakeValuesToAnimationTrack();
    }
};

std::vector<UndoRedo *> AnimCurveEdit::undoRedoEditCurves;

struct MySequence : public ImSequencer::SequenceInterface
{
    MySequence(TileNodeEditGraphDelegate &nodeGraphDelegate) : mNodeGraphDelegate(nodeGraphDelegate), setKeyFrameOrValue(FLT_MAX, FLT_MAX){}

    void Clear()
    {
        mbExpansions.clear();
        mbVisible.clear();
        mLastCustomDrawIndex = -1;
        setKeyFrameOrValue.x = setKeyFrameOrValue.y = FLT_MAX;
        mCurveMin = 0.f;
        mCurveMax = 1.f;
    }
    virtual int GetFrameMin() const { return gNodeDelegate.mFrameMin; }
    virtual int GetFrameMax() const { return gNodeDelegate.mFrameMax; }

    virtual void BeginEdit(int index)
    {
        assert(undoRedoChange == nullptr);
        undoRedoChange = new URChange<TileNodeEditGraphDelegate::ImogenNode>(index, [](int index) { return &gNodeDelegate.mNodes[index]; });
    }
    virtual void EndEdit()
    {
        delete undoRedoChange;
        undoRedoChange = NULL;
        gNodeDelegate.SetTime(gEvaluationTime, false);
    }

    virtual int GetItemCount() const { return (int)gEvaluation.GetStagesCount(); }

    virtual int GetItemTypeCount() const { return 0; }
    virtual const char *GetItemTypeName(int typeIndex) const { return NULL; }
    virtual const char *GetItemLabel(int index) const
    {
        size_t nodeType = gEvaluation.GetStageType(index);
        return gMetaNodes[nodeType].mName.c_str();
    }

    virtual void Get(int index, int** start, int** end, int *type, unsigned int *color)
    {
        size_t nodeType = gEvaluation.GetStageType(index);

        if (color)
            *color = gMetaNodes[nodeType].mHeaderColor;
        if (start)
            *start = &mNodeGraphDelegate.mNodes[index].mStartFrame;
        if (end)
            *end = &mNodeGraphDelegate.mNodes[index].mEndFrame;
        if (type)
            *type = int(nodeType);
    }
    virtual void Add(int type) { };
    virtual void Del(int index) { }
    virtual void Duplicate(int index) { }

    virtual size_t GetCustomHeight(int index) 
    { 
        if (index >= mbExpansions.size())
            return false;
        return mbExpansions[index]? 300 : 0;
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
        for (auto& item : mbExpansions)
            item = false;
        mbExpansions[index] = !mbExpansions[index];
    }

    virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect)
    {
        if (mLastCustomDrawIndex != index)
        {
            mLastCustomDrawIndex = index;
            mbVisible.clear();
        }

        ImVec2 curveMin(float(gNodeDelegate.mFrameMin), mCurveMin);
        ImVec2 curveMax(float(gNodeDelegate.mFrameMax), mCurveMax);
        AnimCurveEdit curveEdit(curveMin, curveMax, gNodeDelegate.mAnimTrack, mbVisible, index);
        mbVisible.resize(curveEdit.GetCurveCount(), true);
        draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);
        for (int i = 0; i < curveEdit.GetCurveCount(); i++)
        {
            ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
            ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i + 1) * 14.f);
			uint32_t curveColor = curveEdit.mColors[i];
            draw_list->AddText(pta, mbVisible[i] ? curveColor : ((curveColor&0xFFFFFF)+0x80000000), curveEdit.mLabels[i].c_str());
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
                AnimTrack* animTrack = gNodeDelegate.GetAnimTrack(gNodeDelegate.mSelectedNodeIndex, parameterIndex);
                //UndoRedo *undoRedo = nullptr;
                //undoRedo = new URChange<AnimTrack>(int(animTrack - gNodeDelegate.GetAnimTrack().data()), [](int index) { return &gNodeDelegate.mAnimTrack[index]; });

                if (setKeyFrameOrValue.x < FLT_MAX)
                {
                    keyValue.x = setKeyFrameOrValue.x;
                }
                else
                {
                    keyValue.y = setKeyFrameOrValue.y;
                }
                curveEdit.EditPoint(selPoint.curveIndex, selPoint.pointIndex, keyValue);
                //delete undoRedo;
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

    TileNodeEditGraphDelegate &mNodeGraphDelegate;
    std::vector<bool> mbExpansions;
    std::vector<bool> mbVisible;
    ImVector<ImCurveEdit::EditPoint> mSelectedCurvePoints;
    int mLastCustomDrawIndex;
    ImVec2 setKeyFrameOrValue;
    ImVec2 getKeyFrameOrValue;
    float mCurveMin, mCurveMax;
    URChange<TileNodeEditGraphDelegate::ImogenNode> *undoRedoChange;
};
MySequence mySequence(gNodeDelegate);

void Imogen::ShowAppMainMenuBar()
{
    if (ImGui::BeginPopup("Plugins"))
    {
        if (ImGui::MenuItem("Reload plugins")) {}
        ImGui::Separator();
        for (auto& plugin : mRegisteredPlugins)
        {
            if (ImGui::MenuItem(plugin.mName.c_str())) 
            {
                try
                {
                    pybind11::exec(plugin.mPythonCommand);
                }
                catch (pybind11::error_already_set& ex)
                {
                    Log(ex.what());
                }
            }
        }
        ImGui::Separator();
        unsigned int imogenLogo = gEvaluation.GetTexture("Stock/ImogenLogo.png");
        ImGui::Image((ImTextureID)(int64_t)imogenLogo, ImVec2(200,86), ImVec2(0,1), ImVec2(1,0));
        ImGui::EndPopup();
    }
}


static const ImVec2 deltaHeight = ImVec2(0, 32);
void Imogen::ShowTitleBar(Builder *builder)
{
    ImGuiIO& io = ImGui::GetIO();
    
    ImVec2 butSize = ImVec2(32, 32);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 0.f) + deltaHeight);

    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.f);// ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    ImGui::PushStyleColor(ImGuiCol_Button, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0x80808080);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
    ImGui::Begin("TitleBar", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PushID(149);
    if (ImGui::Button("", butSize))
    {
        ImGui::OpenPopup("Plugins");
    }
    ShowAppMainMenuBar();
    ImRect rcMenu(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    ImGui::PopID();
    ImGui::SameLine();
    // imogen info strings
    ImGui::BeginChildFrame(152, ImVec2(io.DisplaySize.x - butSize.x*4.f - 300, 32.f));
    ImGui::Text("Imogen 0.10");
    if (selectedMaterial != -1)
    {
        Material& material = library.mMaterials[selectedMaterial];
        ImGui::Text("%s", material.mName.c_str());
    }
    ImGui::EndChildFrame();
    ImGui::SameLine();

    // exporting frame / build
    unsigned int buildIcon = gEvaluation.GetTexture("Stock/Build.png");
    if (ImGui::ImageButton((ImTextureID)(int64_t)buildIcon, ImVec2(30,30), ImVec2(0,1), ImVec2(1,0)))
    {
        if (selectedMaterial != -1)
        {
            Material& material = library.mMaterials[selectedMaterial];
            builder->Add(material.mName.c_str());
        }
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Add current graph to the build list");
        ImGui::EndTooltip();
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
    if (ImGui::IsItemHovered() && (buildInfos.size()>1))
    {
        ImGui::BeginTooltip();
        for (auto& bi : buildInfos)
        {
            ImGui::Text(bi.mName.c_str());
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

void Imogen::Show(Builder *builder, Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
    ImGuiIO& io = ImGui::GetIO();

    ShowTitleBar(builder);

    ImGui::SetNextWindowPos(deltaHeight);
    ImGui::SetNextWindowSize(io.DisplaySize - deltaHeight);
    
    if (ImGui::Begin("Imogen", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
        ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);

        if (ImGui::Begin("Nodes"))
        {
            if (selectedMaterial != -1)
            {
                Material& material = library.mMaterials[selectedMaterial];
                ImGui::PushItemWidth(150);
                ImGui::InputText("Name", &material.mName);
                ImGui::SameLine();
                ImGui::PopItemWidth();


                ImGui::PushItemWidth(60);
                static int previewSize = 0;
                //ImGui::Combo("Preview size", &previewSize, "  128\0  256\0  512\0 1024\0 2048\0 4096\0");
                //ImGui::SameLine();
                if (ImGui::Button("Do exports"))
                {
                    nodeGraphDelegate.DoForce();
                }
                ImGui::SameLine();
                if (ImGui::Button("Save Graph"))
                {
                    nfdchar_t *outPath = NULL;
                    nfdresult_t result = NFD_SaveDialog("imogen", NULL, &outPath);

                    if (result == NFD_OKAY)
                    {
                        Library tempLibrary;
                        Material& material = library.mMaterials[selectedMaterial];
                        tempLibrary.mMaterials.push_back(material);
                        SaveLib(&tempLibrary, outPath);
                        Log("Graph %s saved at path %s\n", material.mName.c_str(), outPath);
                        free(outPath);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete Graph"))
                {
                    library.mMaterials.erase(library.mMaterials.begin() + selectedMaterial);
                    selectedMaterial = int(library.mMaterials.size()) - 1;
                    UpdateNewlySelectedGraph(nodeGraphDelegate, evaluation);
                }
                ImGui::PopItemWidth();
            }
            NodeGraph(&nodeGraphDelegate, selectedMaterial != -1);    
        }
        ImGui::End();

        if (ImGui::Begin("Shaders"))
        {
            HandleEditor(editor, nodeGraphDelegate, evaluation);
        }
        ImGui::End();

        if (ImGui::Begin("Library"))
        {
            LibraryEdit(library, nodeGraphDelegate, evaluation);
        }
        ImGui::End();

        if (ImGui::Begin("Pins"))
        {
            nodeGraphDelegate.PinnedEdit();
        }
        ImGui::End();

        ImGui::SetWindowSize(ImVec2(300, 300));
        if (ImGui::Begin("Parameters"))
        {
            const ImVec2 windowPos = ImGui::GetCursorScreenPos();
            const ImVec2 canvasSize = ImGui::GetContentRegionAvail();

            NodeEdit(nodeGraphDelegate, evaluation);
        }
        ImGui::End();

        if (ImGui::Begin("Logs"))
        {
            ImguiAppLog::Log->DrawEmbedded();
        }
        ImGui::End();

        if (ImGui::Begin("Timeline"))
        {
            int selectedEntry = nodeGraphDelegate.mSelectedNodeIndex;
            static int firstFrame = 0;
            int currentTime = gEvaluationTime;

            ImGui::PushItemWidth(80);
            ImGui::PushID(200);
            ImGui::InputInt("", &gNodeDelegate.mFrameMin, 0, 0);
            ImGui::PopID();
            ImGui::SameLine();
            if (ImGui::Button("|<"))
                currentTime = gNodeDelegate.mFrameMin;
            ImGui::SameLine();
            if (ImGui::Button("<"))
                currentTime--;
            ImGui::SameLine();
            
            ImGui::PushID(201);
            if (ImGui::InputInt("", &currentTime, 0, 0, 0))
            {
            }
            ImGui::PopID();
            
            ImGui::SameLine();
            if (ImGui::Button(">"))
                currentTime++;
            ImGui::SameLine();
            if (ImGui::Button(">|"))
                currentTime = gNodeDelegate.mFrameMax;
            ImGui::SameLine();
            extern bool gbIsPlaying;
            if (ImGui::Button(gbIsPlaying ? "Stop" : "Play"))
            {
                if (!gbIsPlaying)
                {
                    currentTime = gNodeDelegate.mFrameMin;
                }
                gbIsPlaying = !gbIsPlaying;
            }
            extern bool gPlayLoop;
            unsigned int playNoLoopTextureId = evaluation.GetTexture("Stock/PlayNoLoop.png");
            unsigned int playLoopTextureId = evaluation.GetTexture("Stock/PlayLoop.png");

            ImGui::SameLine();
            if (ImGui::ImageButton((ImTextureID)(uint64_t)(gPlayLoop ? playLoopTextureId : playNoLoopTextureId), ImVec2(16.f, 16.f)))
                gPlayLoop = !gPlayLoop;

            ImGui::SameLine();
            ImGui::PushID(202);
            ImGui::InputInt("", &gNodeDelegate.mFrameMax, 0, 0);
            ImGui::PopID();
            ImGui::SameLine();
            currentTime = ImMax(currentTime, 0);
            ImGui::SameLine(0, 40.f);
            if (ImGui::Button("Make Key") && selectedEntry != -1)
            {
                nodeGraphDelegate.MakeKey(currentTime, uint32_t(selectedEntry), 0);
            }

            ImGui::SameLine(0, 50.f);
            
            int setf = (mySequence.getKeyFrameOrValue.x<FLT_MAX)? int(mySequence.getKeyFrameOrValue.x):0;
            ImGui::PushID(203);
            if (ImGui::InputInt("", &setf, 0, 0))
            {
                mySequence.setKeyFrameOrValue.x = float(setf);
            }
            ImGui::SameLine();
            float setv = (mySequence.getKeyFrameOrValue.y < FLT_MAX) ? mySequence.getKeyFrameOrValue.y : 0.f;
            if (ImGui::InputFloat("Key", &setv))
            {
                mySequence.setKeyFrameOrValue.y = setv;
            }
            ImGui::PopID();
            ImGui::SameLine();
            int timeMask[2] = { 0,0 };
            if (selectedEntry != -1)
            {
                timeMask[0] = gNodeDelegate.mNodes[selectedEntry].mStartFrame;
                timeMask[1] = gNodeDelegate.mNodes[selectedEntry].mEndFrame;
            }
            ImGui::PushItemWidth(120);
            if (ImGui::InputInt2("Time Mask", timeMask) && selectedEntry != -1)
            {
                URChange<TileNodeEditGraphDelegate::ImogenNode> undoRedoChange(selectedEntry, [](int index) { return &gNodeDelegate.mNodes[index]; });
                timeMask[1] = ImMax(timeMask[1], timeMask[0]);
                timeMask[0] = ImMin(timeMask[1], timeMask[0]);
                gNodeDelegate.SetTimeSlot(selectedEntry, timeMask[0], timeMask[1]);
            }
            ImGui::PopItemWidth();
            ImGui::PopItemWidth();

            Sequencer(&mySequence, &currentTime, NULL, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_CHANGE_FRAME);
            if (selectedEntry != -1)
            {
                nodeGraphDelegate.mSelectedNodeIndex = selectedEntry;
                NodeGraphSelectNode(selectedEntry);
                auto& imoNode = nodeGraphDelegate.mNodes[selectedEntry];
                gEvaluation.SetStageLocalTime(selectedEntry, ImClamp(currentTime - imoNode.mStartFrame, 0, imoNode.mEndFrame - imoNode.mStartFrame), true);
            }
            if (currentTime != gEvaluationTime)
            {
                gEvaluationTime = currentTime;
                nodeGraphDelegate.SetTime(currentTime, true);
                nodeGraphDelegate.ApplyAnimation(currentTime);
            }
        }
        ImGui::End();

        // view extraction
        int index = 0;
        int removeExtractedView = -1;
        for (auto& extraction : mExtratedViews)
        {
            char tmps[512];
            sprintf(tmps, "%s_View_%03d", gMetaNodes[nodeGraphDelegate.mNodes[extraction.mNodeIndex].mType].mName.c_str(), index);
            bool open = true;
            if (extraction.mFirstFrame)
            {
                extraction.mFirstFrame = false;
                ImGui::SetNextWindowSize(ImVec2(256, 256));
            }
            //ImGui::SetNextWindowFocus();
            if (ImGui::Begin(tmps, &open))
            {
                RenderPreviewNode(int(extraction.mNodeIndex), nodeGraphDelegate, evaluation, false);
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
        // --
    }
}

void Imogen::ValidateCurrentMaterial(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate)
{
    ValidateMaterial(library, nodeGraphDelegate, selectedMaterial);
}

void Imogen::DiscoverNodes(const char *extension, const char *directory, EVALUATOR_TYPE evaluatorType, std::vector<EvaluatorFile>& files)
{
    tinydir_dir dir;
    tinydir_open(&dir, directory);

    while (dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&dir, &file);

        if (!file.is_dir && !strcmp(file.extension, extension))
        {
            files.push_back({ directory, file.name, evaluatorType });
        }

        tinydir_next(&dir);
    }

    tinydir_close(&dir);
}

Imogen::Imogen()
{
}

Imogen::~Imogen()
{

}

void Imogen::Init()
{
    SetStyle();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());

    DiscoverNodes("glsl", "Nodes/GLSL/", EVALUATOR_GLSL, mEvaluatorFiles);
    DiscoverNodes("c", "Nodes/C/", EVALUATOR_C, mEvaluatorFiles);
    DiscoverNodes("py", "Nodes/Python/", EVALUATOR_PYTHON, mEvaluatorFiles);
    DiscoverNodes("glsl", "Nodes/GLSLCompute/", EVALUATOR_GLSLCOMPUTE, mEvaluatorFiles);
    DiscoverNodes("glslc", "Nodes/GLSLCompute/", EVALUATOR_GLSLCOMPUTE, mEvaluatorFiles);
}

void Imogen::Finish()
{

}

void ClearAll(TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation)
{
    nodeGraphDelegate.Clear();
    evaluation.Clear();
    NodeGraphClear();
    InitCallbackRects();
    ClearExtractedViews();
    gUndoRedoHandler.Clear();
    mySequence.Clear();
}


