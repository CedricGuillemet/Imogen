#pragma once

#include "Nodes.h"
#include "Evaluation.h"
#if 0
enum ComponentType
{
	ComponentType_Circle,
	ComponentType_Transform,
	/*ComponentType_Env,
	ComponentType_Puzzle,
	ComponentType_Prefab,
	ComponentType_Waypoint,
	ComponentType_Dialog,
	ComponentType_Plant,
	ComponentType_Sound,
	ComponentType_Portal,
	ComponentType_Particle,
	ComponentType_Trigger,
	ComponentType_Interest,
	ComponentType_Sequence,
	ComponentType_Decal,
	ComponentType_Jumper,
	ComponentType_SubMap,
	ComponentType_Crane,
	ComponentType_JumperVolume,
	ComponentType_Value,
	ComponentType_Lerp,
	ComponentType_Input,
	ComponentType_Output,
	ComponentType_Threshold,
	ComponentType_Mul,
	ComponentType_Mesh,
	ComponentType_Light,
	ComponentType_Accumulator,
	*/
	ComponentType_Count,
};
#endif
struct TileNodeEditGraphDelegate : public NodeGraphDelegate
{
	struct ImogenNode
	{
		size_t mType;
		unsigned int mEvaluationTexture;
		void *mParams;
	};

	std::vector<ImogenNode> mNodes;

	enum ConTypes
	{
		Con_Float,
		Con_Float2,
		Con_Float3,
		Con_Float4,
		Con_Structure,
		Con_Any,
	};
	virtual unsigned char *GetParamBlock(int index, size_t& paramBlockSize)
	{
		const ImogenNode & node = mNodes[index];
		paramBlockSize = ComputeParamMemSize(node.mType);
		return (unsigned char*)node.mParams;
	}
	virtual void SetParamBlock(int index, unsigned char* paramBlock)
	{
		const ImogenNode & node = mNodes[index];
		memcpy(node.mParams, paramBlock, ComputeParamMemSize(node.mType));
		SetEvaluationCall(node.mEvaluationTexture, ComputeFunctionCall(index));
	}

	virtual bool AuthorizeConnexion(int typeA, int typeB)
	{
		return true;
	}

	virtual unsigned int GetNodeTexture(size_t index)
	{
		return GetEvaluationTexture(mNodes[index].mEvaluationTexture);
	}
	virtual void AddNode(size_t type)
	{
		size_t index = mNodes.size();
		ImogenNode node;
		node.mEvaluationTexture = AddEvaluationTarget();
		node.mType = type;
		size_t paramsSize = ComputeParamMemSize(type);
		node.mParams = malloc(paramsSize);
		memset(node.mParams, 0, paramsSize);
		mNodes.push_back(node);

		SetEvaluationCall(node.mEvaluationTexture, ComputeFunctionCall(index));
	}

	void AddLink(int InputIdx, int InputSlot, int OutputIdx, int OutputSlot)
	{
		AddEvaluationInput(OutputIdx, OutputSlot, InputIdx);
	}

	virtual void DelLink(int index, int slot)
	{
		DelEvaluationInput(index, slot);
	}

	virtual void DeleteNode(size_t index)
	{
		DelEvaluationTarget(index);
		free(mNodes[index].mParams);
		mNodes.erase(mNodes.begin() + index);
		for (auto& node : mNodes)
		{
			if (node.mEvaluationTexture > index)
				node.mEvaluationTexture--;
		}
	}
	virtual const MetaNode* GetMetaNodes(int &metaNodeCount)
	{
		metaNodeCount = 8;

		static const MetaNode metaNodes[8] = {
			{
				"Circle"
				,{ {} }
			,{ { "Out", (int)Con_Float4 } }
			,{ { "Radius", (int)Con_Float } }
			}
			,
			{
				"Transform"
				,{ { "In", (int)Con_Float4 } }
			,{ { "Out", (int)Con_Float4 } }
			,{ { "Translate", (int)Con_Float2 },{ "Rotation", (int)Con_Float },{ "Scale", (int)Con_Float } }
			}
			,
			{
				"Square"
				,{ { } }
			,{ { "Out", (int)Con_Float4 } }
			,{ { "Width", (int)Con_Float } }
			}
			,
			{
				"Checker"
				,{ {} }
			,{ { "Out", (int)Con_Float4 } }
			,{  }
			}
			,
			{
				"Sine"
				,{ { "In", (int)Con_Float4 } }
			,{ { "Out", (int)Con_Float4 } }
			,{ { "Frequency", (int)Con_Float },{ "Angle", (int)Con_Float } }
			}

			,
			{
				"SmoothStep"
				,{ { "In", (int)Con_Float4 } }
			,{ { "Out", (int)Con_Float4 } }
			,{ { "Low", (int)Con_Float },{ "High", (int)Con_Float } }
			}

			,
			{
				"Pixelize"
				,{ { "In", (int)Con_Float4 } }
			,{ { "Out", (int)Con_Float4 } }
			,{ { "scale", (int)Con_Float } }
			}


			,
			{
				"Blur"
				,{ { "In", (int)Con_Float4 } }
			,{ { "Out", (int)Con_Float4 } }
			,{ { "angle", (int)Con_Float },{ "strength", (int)Con_Float } }
			}

			};

		return metaNodes;
	}

	std::string ComputeFunctionCall(size_t index)
	{
		int metaNodeCount;
		const MetaNode* metaNodes = GetMetaNodes(metaNodeCount);
		const MetaNode &metaNode = metaNodes[mNodes[index].mType];
		std::string call(metaNode.mName);
		call += "(vUV";


		const NodeGraphDelegate::Con * param = metaNodes[mNodes[index].mType].mParams;
		unsigned char *paramBuffer = (unsigned char*)mNodes[index].mParams;
		char tmps[512];
		for (int i = 0; i < MaxCon; i++, param++)
		{
			if (!param->mName)
				break;
			switch (param->mType)
			{
			case Con_Float:
				sprintf(tmps, ",%f", *(float*)paramBuffer);
				break;
			case Con_Float2:
				//ImGui::InputFloat2(param->mName, (float*)paramBuffer);
				sprintf(tmps, ",vec2(%f, %f)", ((float*)paramBuffer)[0], ((float*)paramBuffer)[1]);
				break;
			case Con_Float3:
				//ImGui::InputFloat3(param->mName, (float*)paramBuffer);
				break;
			case Con_Float4:
				//ImGui::InputFloat4(param->mName, (float*)paramBuffer);
				break;
			}
			call += tmps;
			paramBuffer += ComputeParamMemSize(param->mType);
		}
		call += ")";
		return call;
	}

	void EditNode()
	{
		size_t index = mSelectedNodeIndex;

		int metaNodeCount;
		const MetaNode* metaNodes = GetMetaNodes(metaNodeCount);
		bool dirty = false;
		const MetaNode& currentMeta = metaNodes[mNodes[index].mType];
		if (!ImGui::CollapsingHeader(currentMeta.mName, 0, ImGuiTreeNodeFlags_DefaultOpen))
			return;

		const NodeGraphDelegate::Con * param = currentMeta.mParams;
		unsigned char *paramBuffer = (unsigned char*)mNodes[index].mParams;
		for (int i = 0; i < MaxCon; i++, param++)
		{
			if (!param->mName)
				break;
			switch (param->mType)
			{
			case Con_Float:
				dirty |= ImGui::InputFloat(param->mName, (float*)paramBuffer);
				break;
			case Con_Float2:
				dirty |= ImGui::InputFloat2(param->mName, (float*)paramBuffer);
				break;
			case Con_Float3:
				dirty |= ImGui::InputFloat3(param->mName, (float*)paramBuffer);
				break;
			case Con_Float4:
				dirty |= ImGui::InputFloat4(param->mName, (float*)paramBuffer);
				break;
			}
			paramBuffer += ComputeParamMemSize(param->mType);
		}
		
		//ImGui::End();
		if (dirty)
			SetEvaluationCall(mNodes[index].mEvaluationTexture, ComputeFunctionCall(index));
	}

	// helpers
	/*
	size_t ComputeParamsMemSize()
	{
		int metaNodeCount;
		const MetaNode* metaNodes = GetMetaNodes(metaNodeCount);
		size_t res = 0;
		for (size_t typeIndex = 0; typeIndex < metaNodeCount; typeIndex++)
		{
			res += ComputeParamMemSize(typeIndex);
		}
		return res;
	}
	*/
	size_t ComputeParamMemSize(size_t typeIndex)
	{
		int metaNodeCount;
		const MetaNode* metaNodes = GetMetaNodes(metaNodeCount);
		size_t res = 0;
		const NodeGraphDelegate::Con * param = metaNodes[typeIndex].mParams;
		for (int i = 0; i < MaxCon; i++, param++)
		{
			if (!param->mName)
				break;
			res += ComputeParamMemSize(param->mType);
		}
		return res;
	}
	size_t ComputeParamMemSize(int paramType)
	{
		size_t res = 0;
		switch (paramType)
		{
		case Con_Float:
			res += sizeof(float);
			break;
		case Con_Float2:
			res += sizeof(float) * 2;
			break;
		case Con_Float3:
			res += sizeof(float) * 3;
			break;
		case Con_Float4:
			res += sizeof(float) * 4;
			break;
		}
		return res;
	}

	virtual void UpdateEvaluationList(const std::vector<int> nodeOrderList)
	{
		SetEvaluationOrder(nodeOrderList);
	}
};

