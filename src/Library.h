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

#pragma once

#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <string>
struct InputSampler
{
	InputSampler() : mWrapU(0), mWrapV(0), mFilterMin(0), mFilterMag(0) 
	{
	}
	uint32_t mWrapU;
	uint32_t mWrapV;
	uint32_t mFilterMin;
	uint32_t mFilterMag;
};
struct MaterialNode
{
	uint32_t mType;
	uint32_t mPosX;
	uint32_t mPosY;
	std::vector<InputSampler> mInputSamplers;
	std::vector<uint8_t> mParameters;
	std::vector<uint8_t> mImage;
};
struct MaterialConnection
{
	uint32_t mInputNode;
	uint32_t mOutputNode;
	uint8_t mInputSlot;
	uint8_t mOutputSlot;
};
struct Material
{
	std::string mName;
	std::string mComment;
	std::vector<MaterialNode> mMaterialNodes;
	std::vector<MaterialConnection> mMaterialConnections;
	std::vector<uint8_t> mThumbnail;

	//run time
	unsigned int mThumbnailTextureId;
	unsigned int mRuntimeUniqueId;
};
struct Library
{
	std::vector<Material> mMaterials;
};

void LoadLib(Library *library, const char *szFilename);
void SaveLib(Library *library, const char *szFilename);

unsigned int GetRuntimeId();
extern Library library;