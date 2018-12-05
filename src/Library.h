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
#include <map>
#include "Utils.h"

// used to retrieve structure in library. left is index. right is uniqueId
// if item at index doesn't correspond to uniqueid, then a search is done
// based on the unique id
typedef std::pair<size_t, unsigned int> ASyncId;
template<typename T> T* GetByAsyncId(ASyncId id, std::vector<T>& items)
{
	if (items.size() > id.first && items[id.first].mRuntimeUniqueId == id.second)
	{
		return &items[id.first];
	}
	else
	{
		// find identifier
		for (auto& item : items)
		{
			if (item.mRuntimeUniqueId == id.second)
			{
				return &item;
			}
		}
	}
	return NULL;
}

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
	std::string mTypeName;
	int32_t mPosX;
	int32_t mPosY;
	std::vector<InputSampler> mInputSamplers;
	std::vector<uint8_t> mParameters;
	std::vector<uint8_t> mImage;

	uint32_t mFrameStart;
	uint32_t mFrameEnd;

	// runtime
	unsigned int mRuntimeUniqueId;
};

struct MaterialNodeRug
{
	int32_t mPosX;
	int32_t mPosY;
	int32_t mSizeX;
	int32_t mSizeY;
	uint32_t mColor;
	std::string mComment;
};

struct MaterialConnection
{
	uint32_t mInputNode;
	uint32_t mOutputNode;
	uint8_t mInputSlot;
	uint8_t mOutputSlot;
};

struct AnimationBase 
{
	std::vector<uint32_t> mFrames;

	virtual void Allocate(size_t elementCount) = 0;
	virtual void* GetData() = 0;
	virtual void GetByteLength() const = 0;
};

template<typename T> struct Animation : public AnimationBase
{
	std::vector<T> mValues;

	virtual void Allocate(size_t elementCount) 
	{ 
		mFrames.resize(elementCount); 
		mValues.resize(elementCount);
	}
	virtual void* GetData()
	{
		return mValues.data();
	}
	virtual void GetValuesByteLength() const
	{
		return mValues.size() * sizeof(T);
	}
};

struct AnimTrack
{
	uint32_t mNodeIndex;
	uint32_t mParamIndex;
	uint32_t mValueType; // Con_
	AnimationBase *mAnimation;
};

struct Material
{
	std::string mName;
	std::string mComment;
	std::vector<MaterialNode> mMaterialNodes;
	std::vector<MaterialNodeRug> mMaterialRugs;
	std::vector<MaterialConnection> mMaterialConnections;
	std::vector<uint8_t> mThumbnail;

	std::vector<AnimTrack> mAnimTrack;


	MaterialNode* Get(ASyncId id) { return GetByAsyncId(id, mMaterialNodes); }

	//run time
	unsigned int mThumbnailTextureId;
	unsigned int mRuntimeUniqueId;
};

struct Library
{
	std::vector<Material> mMaterials;
	Material* Get(ASyncId id) { return GetByAsyncId(id, mMaterials); }
};

void LoadLib(Library *library, const char *szFilename);
void SaveLib(Library *library, const char *szFilename);

enum ConTypes
{
	Con_Float,
	Con_Float2,
	Con_Float3,
	Con_Float4,
	Con_Color4,
	Con_Int,
	Con_Int2,
	Con_Ramp,
	Con_Angle,
	Con_Angle2,
	Con_Angle3,
	Con_Angle4,
	Con_Enum,
	Con_Structure,
	Con_FilenameRead,
	Con_FilenameWrite,
	Con_ForceEvaluate,
	Con_Bool,
	Con_Ramp4,
	Con_Camera,
	Con_Any,
};

struct Camera
{
	Vec4 mPosition;
	Vec4 mDirection;
	Vec4 mUp;
	Vec4 mLens; // fov,....
};

size_t GetParameterTypeSize(ConTypes paramType);

struct MetaCon
{
	std::string mName;
	int mType;
};

struct MetaParameter
{
	std::string mName;
	ConTypes mType;
	float mRangeMinX, mRangeMaxX;
	float mRangeMinY, mRangeMaxY;
	bool mbRelative;
	bool mbQuadSelect;
	const char* mEnumList;
};

struct MetaNode
{
	std::string mName;
	uint32_t mHeaderColor;
	int mCategory;
	std::vector<MetaCon> mInputs;
	std::vector<MetaCon> mOutputs;
	std::vector<MetaParameter> mParams;
	bool mbHasUI;
	bool mbSaveTexture;
};

extern std::vector<MetaNode> gMetaNodes;
size_t GetMetaNodeIndex(const std::string& metaNodeName);
void LoadMetaNodes();

unsigned int GetRuntimeId();
extern Library library;

