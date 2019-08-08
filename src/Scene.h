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

#pragma once
#include <vector>
#include "Utils.h"

struct EvaluationContext;
struct EvaluationInfo;

struct Scene
{
    Scene()
    {
    }
    virtual ~Scene();
    static std::shared_ptr<Scene> BuildDefaultScene();
    struct Mesh
    {
        struct Format
        {
            enum
            {
                POS = 1 << 0,
                NORM = 1 << 1,
                COL = 1 << 2,
                UV = 1 << 3,
            };
        };
		/*
        struct Buffer
        {
            unsigned int id;
            unsigned int format;
            unsigned int stride;
            unsigned int count;
        };
        struct IndexBuffer
        {
            unsigned int id;
            unsigned int stride;
            unsigned int count;
        };
		*/
        struct Primitive
        {
			~Primitive();
			std::vector<bgfx::VertexBufferHandle> mStreams;
			bgfx::IndexBufferHandle mIbh;
			bgfx::VertexDecl mDecl;

			uint32_t mVertexCount;
			uint32_t mIndexCount;
            void AddBuffer(const void* data, unsigned int format, unsigned int stride, unsigned int count);
            void AddIndexBuffer(const void* data, unsigned int stride, unsigned int count);
            void Draw(bgfx::ViewId viewId, ProgramHandle program) const;
        };
        std::vector<Primitive> mPrimitives;
        void Draw(bgfx::ViewId viewId, ProgramHandle program) const;
    };
    std::vector<Mesh> mMeshes;
    std::vector<Mat4x4> mWorldTransforms;
    std::vector<int> mMeshIndex;
    std::string mName;
    void Draw(EvaluationInfo& evaluationInfo, bgfx::ViewId viewId, ProgramHandle program) const;
protected:
	static std::weak_ptr<Scene> mDefaultScene;
};