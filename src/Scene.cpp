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
#include "Scene.h"
#include "Evaluators.h"

std::weak_ptr<Scene> Scene::mDefaultScene;

void Scene::Mesh::Primitive::Draw(bgfx::ViewId viewId, bgfx::ProgramHandle program) const
{
    bgfx::setIndexBuffer(mIbh);
    for (auto i = 0; i < mStreams.size(); i++)
    {
        bgfx::setVertexBuffer(i, mStreams[i]);
    }
    
    assert(program.idx);
    bgfx::submit(viewId, program);
}


Scene::Mesh::Primitive::~Primitive()
{
    for (auto i = 0; i < mStreams.size(); i++)
    {
        bgfx::destroy(mStreams[i]);
    }
    if (mIbh.idx != bgfx::kInvalidHandle)
    {
        bgfx::destroy(mIbh);
    }
}

void Scene::Mesh::Primitive::AddBuffer(const void* data, unsigned int format, unsigned int stride, unsigned int count)
{
    mDecl.begin();
    switch (format)
    {
    case Format::UV:
        mDecl.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float);
		/*if (!bgfx::getCaps()->originBottomLeft)
		{
			float *pv = (float*)data;
			for (unsigned int i = 0;i<count;i++)
			{
				const int idx = i * 2 + 1;
				pv[idx] = 1.f - pv[idx];
			}
		}*/
        break;
    case Format::COL:
        mDecl.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float);
        break;
    case Format::POS:
        mDecl.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
        break;
    case Format::NORM:
        mDecl.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float);
        break;
    default:
        // skip unsupported attributes. 
        return;
    }
    mDecl.end();

    mStreams.push_back(bgfx::createVertexBuffer(bgfx::copy(data, stride * count), mDecl));
    mVertexCount = count;
}

void Scene::Mesh::Primitive::AddIndexBuffer(const void* data, unsigned int stride, unsigned int count)
{
    if (mIbh.idx != bgfx::kInvalidHandle)
    {
        bgfx::destroy(mIbh);
    }
    mIbh = bgfx::createIndexBuffer(bgfx::copy(data, stride* count), (stride == 4)?BGFX_BUFFER_INDEX32:0);
    mIndexCount = count;
}

void Scene::Mesh::Draw(bgfx::ViewId viewId, bgfx::ProgramHandle program) const
{
    for (auto& prim : mPrimitives)
    {
        prim.Draw(viewId, program);
    }
}

void Scene::Draw(EvaluationInfo& evaluationInfo, bgfx::ViewId viewId, bgfx::ProgramHandle program) const
{
    for (unsigned int i = 0; i < mMeshIndex.size(); i++)
    {
        int index = mMeshIndex[i];
        if (index == -1)
        {
            continue;
        }
        memcpy(evaluationInfo.world, mWorldTransforms[i], sizeof(Mat4x4));
        FPU_MatrixF_x_MatrixF(evaluationInfo.world, evaluationInfo.viewProjection, evaluationInfo.worldViewProjection);
        gEvaluators.ApplyEvaluationInfo(evaluationInfo);
        mMeshes[index].Draw(viewId, program);
    }
}

Scene::~Scene()
{
}

std::shared_ptr<Scene> Scene::BuildDefaultScene()
{
    if (!mDefaultScene.expired())
    {
        return mDefaultScene.lock();
    }
    auto defaultScene = std::make_shared<Scene>();
    defaultScene->mMeshes.resize(1);
    defaultScene->mBounds.resize(1);
    auto& mesh = defaultScene->mMeshes.back();
    mesh.mPrimitives.resize(1);
    auto& prim = mesh.mPrimitives.back();
    static float fsUVInv[] = { 0.f, 1.f,
        1.f, 1.f,
        0.f, 0.f,
        1.f, 0.f};
	static float fsUV[] = { 0.f, 0.f,
		1.f, 0.f,
		0.f, 1.f,
		1.f, 1.f };
	static float fsPos[] = { 0.f, 0.f, 0.f,
		1.f, 0.f, 0.f,
		0.f, 1.f, 0.f,
		1.f, 1.f, 0.f };

	float *puv = bgfx::getCaps()->originBottomLeft ? fsUVInv : fsUV;
    auto& bounds = defaultScene->mBounds.back();
    bounds.AddPoint(*(Vec3*)&fsPos[0]);
    bounds.AddPoint(*(Vec3*)&fsPos[1]);
    bounds.AddPoint(*(Vec3*)&fsPos[2]);
    static const uint16_t fsIdx[] = { 0, 1, 2, 1, 3, 2 };
    prim.AddBuffer(puv, Scene::Mesh::Format::UV, 2 * sizeof(float), 4);
	prim.AddBuffer(fsPos, Scene::Mesh::Format::POS, 3 * sizeof(float), 4);
    prim.AddIndexBuffer(fsIdx, 2, 6);

    // add node and transform
    defaultScene->mWorldTransforms.resize(1);
    defaultScene->mWorldTransforms[0].Identity();
    defaultScene->mMeshIndex.resize(1, 0);
    mDefaultScene = defaultScene;
    return defaultScene;
}

Bounds Scene::ComputeBounds() const
{
    Bounds bounds;
    for (unsigned int i = 0; i < mMeshIndex.size(); i++)
    {
        int index = mMeshIndex[i];
        if (index == -1)
        {
            continue;
        }
        bounds.AddBounds(mBounds[index], mWorldTransforms[index]);
    }
    return bounds;
}