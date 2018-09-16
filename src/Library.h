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
};
struct Library
{
	std::vector<Material> mMaterials;
};

void LoadLib(Library *library, const char *szFilename);
void SaveLib(Library *library, const char *szFilename);
