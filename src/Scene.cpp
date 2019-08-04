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
#include "EvaluationContext.h"

void Scene::Mesh::Primitive::Draw() const
{
    /*
	todogl
	unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    for (auto& buffer : mBuffers)
    {
        glBindBuffer(GL_ARRAY_BUFFER, buffer.id);
        switch (buffer.format)
        {
        case Format::UV:
            glVertexAttribPointer(SemUV0, 2, GL_FLOAT, GL_FALSE, 8, 0);
            glEnableVertexAttribArray(SemUV0);
            break;
        case Format::COL:
            glVertexAttribPointer(SemUV0 + 1, 4, GL_FLOAT, GL_FALSE, 16, 0);
            glEnableVertexAttribArray(SemUV0 + 1);
            break;
        case Format::POS:
            glVertexAttribPointer(SemUV0 + 2, 3, GL_FLOAT, GL_FALSE, 12, 0);
            glEnableVertexAttribArray(SemUV0 + 2);
            break;
        case Format::NORM:
            glVertexAttribPointer(SemUV0 + 3, 3, GL_FLOAT, GL_FALSE, 12, 0);
            glEnableVertexAttribArray(SemUV0 + 3);
            break;
        }
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    glBindVertexArray(vao);

    if (!mIndexBuffer.id)
    {
        glDrawArrays(GL_TRIANGLES, 0, mBuffers[0].count);
    }
    else
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer.id);
        glDrawElements(GL_TRIANGLES,
            mIndexBuffer.count,
            (mIndexBuffer.stride == 4) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
            (void*)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDeleteVertexArrays(1, &vao);
	*/
}


Scene::Mesh::Primitive::~Primitive()
{
	bgfx::destroy(mVbh);
	bgfx::destroy(mIbh);
}
void Scene::Mesh::Primitive::AddBuffer(const void* data, unsigned int format, unsigned int stride, unsigned int count)
{
    /* todogl
	unsigned int va;
    glGenBuffers(1, &va);
    glBindBuffer(GL_ARRAY_BUFFER, va);
    glBufferData(GL_ARRAY_BUFFER, stride * count, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    mBuffers.push_back({ va, format, stride, count });
	*/
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

	mVbh = bgfx::createVertexBuffer(bgfx::copy(data, stride * count), mDecl);
	mVertexCount = count;
}

void Scene::Mesh::Primitive::AddIndexBuffer(const void* data, unsigned int stride, unsigned int count)
{
    /* todogl
	unsigned int ia;
    glGenBuffers(1, &ia);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ia);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, stride * count, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    mIndexBuffer = { ia, stride, count };
	*/

	mIbh = bgfx::createIndexBuffer(bgfx::copy(data, stride* count), (stride == 4)?BGFX_BUFFER_INDEX32:0);
	mIndexCount = count;
}

void Scene::Mesh::Draw() const
{
    for (auto& prim : mPrimitives)
    {
        prim.Draw();
    }
}
void Scene::Draw(EvaluationContext* context, EvaluationInfo& evaluationInfo) const
{
    for (unsigned int i = 0; i < mMeshIndex.size(); i++)
    {
        int index = mMeshIndex[i];
        if (index == -1)
            continue;

        /*todogl
		glBindBuffer(GL_UNIFORM_BUFFER, context->mEvaluationStateGLSLBuffer);
        memcpy(evaluationInfo.model, mWorldTransforms[i], sizeof(Mat4x4));
        FPU_MatrixF_x_MatrixF(evaluationInfo.model, evaluationInfo.viewProjection, evaluationInfo.modelViewProjection);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(EvaluationInfo), &evaluationInfo, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
		*/
        mMeshes[index].Draw();
    }
}

Scene::~Scene()
{
    // todo : clear ia/va
}

std::shared_ptr<Scene> Scene::BuildDefaultScene()
{
    auto defaultScene = std::make_shared<Scene>();
    defaultScene->mMeshes.resize(1);
    auto& mesh = defaultScene->mMeshes.back();
    mesh.mPrimitives.resize(1);
    auto& prim = mesh.mPrimitives.back();
    //static const float fsVts[] = { 0.f, 0.f, 0.f, 2.f, 0.f, 0.f, 0.f, 2.f, 0.f };
	static const float fsVts[] = { 0.f, 0.f, 2.f, 0.f, 0.f, 2.f};
	static const uint16_t fsIdx[] = { 0, 1, 2 };
    prim.AddBuffer(fsVts, Scene::Mesh::Format::UV, 2 * sizeof(float), 3);
	prim.AddIndexBuffer(fsIdx, 2, 3);
    // add node and transform
    defaultScene->mWorldTransforms.resize(1);
    defaultScene->mWorldTransforms[0].Identity();
    defaultScene->mMeshIndex.resize(1, 0);
    return defaultScene;
}