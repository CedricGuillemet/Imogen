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

void Scene::Mesh::Primitive::Draw(ProgramHandle program) const
{
	for (auto i = 0; i < mStreams.size(); i++)
	{
		bgfx::setVertexBuffer(i, mStreams[i]);
	}
	bgfx::setIndexBuffer(mIbh);
	bgfx::submit(1, program);
}


Scene::Mesh::Primitive::~Primitive()
{
	for (auto i = 0; i < mStreams.size(); i++)
	{
		bgfx::destroy(mStreams[i]);
	}
	if (mIbh.idx)
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
	}
	mDecl.end();

	mStreams.push_back(bgfx::createVertexBuffer(bgfx::copy(data, stride * count), mDecl));
	mVertexCount = count;
}

void Scene::Mesh::Primitive::AddIndexBuffer(const void* data, unsigned int stride, unsigned int count)
{
	if (mIbh.idx)
	{
		bgfx::destroy(mIbh);
	}
	mIbh = bgfx::createIndexBuffer(bgfx::copy(data, stride* count), (stride == 4)?BGFX_BUFFER_INDEX32:0);
	mIndexCount = count;
}

void Scene::Mesh::Draw(ProgramHandle program) const
{
    for (auto& prim : mPrimitives)
    {
        prim.Draw(program);
    }
}

void Scene::Draw(EvaluationInfo& evaluationInfo, ProgramHandle program) const
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
        mMeshes[index].Draw(program);
    }
}

Scene::~Scene()
{
    // todo : clear ia/va
}

std::shared_ptr<Scene> Scene::BuildDefaultScene()
{
	if (!mDefaultScene.expired())
	{
		return mDefaultScene.lock();
	}
    auto defaultScene = std::make_shared<Scene>();
    defaultScene->mMeshes.resize(1);
    auto& mesh = defaultScene->mMeshes.back();
    mesh.mPrimitives.resize(1);
    auto& prim = mesh.mPrimitives.back();
	static const float fsVts[] = { 0.f, 0.f, 2.f, 0.f, 0.f, 2.f};
	static const uint16_t fsIdx[] = { 0, 1, 2 };
    prim.AddBuffer(fsVts, Scene::Mesh::Format::UV, 2 * sizeof(float), 3);
	prim.AddIndexBuffer(fsIdx, 2, 3);
    // add node and transform
    defaultScene->mWorldTransforms.resize(1);
    defaultScene->mWorldTransforms[0].Identity();
    defaultScene->mMeshIndex.resize(1, 0);
	mDefaultScene = defaultScene;
    return defaultScene;
}