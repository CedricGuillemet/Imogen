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

#include "Nodes.h"
#include "Evaluation.h"
#include "curve.h"
#include "Library.h"
#include "nfd.h"

struct TileNodeEditGraphDelegate : public NodeGraphDelegate
{
	TileNodeEditGraphDelegate(Evaluation& evaluation) : mEvaluation(evaluation), mbMouseDragging(false)
	{
		mCategoriesCount = 8;
		static const char *categories[] = {
			"Transform",
			"Generator",
			"Material",
			"Blend",
			"Filter",
			"Noise",
			"File",
			"Paint"};
		mCategories = categories;
		assert(!mInstance);
		mInstance = this;
	}
	

	Evaluation& mEvaluation;
	struct ImogenNode
	{
		size_t mType;
		size_t mEvaluationTarget;
		void *mParameters;
		size_t mParametersSize;
		unsigned int mRuntimeUniqueId;
		std::vector<InputSampler> mInputSamplers;
		bool mbProcessing;
	};

	std::vector<ImogenNode> mNodes;

	enum ConTypes
	{
		Con_Float,
		Con_Float2,
		Con_Float3,
		Con_Float4,
		Con_Color4,
		Con_Int,
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
		Con_Any,
	};

	void Clear()
	{
		mSelectedNodeIndex = -1;
		mNodes.clear();
	}

	virtual unsigned char *GetParamBlock(size_t index, size_t& paramBlockSize)
	{
		const ImogenNode & node = mNodes[index];
		paramBlockSize = ComputeParamMemSize(node.mType);
		return (unsigned char*)node.mParameters;
	}
	virtual void SetParamBlock(size_t index, unsigned char* parameters)
	{
		const ImogenNode & node = mNodes[index];
		memcpy(node.mParameters, parameters, ComputeParamMemSize(node.mType));
		mEvaluation.SetEvaluationParameters(node.mEvaluationTarget, parameters, node.mParametersSize);
		mEvaluation.SetEvaluationSampler(node.mEvaluationTarget, node.mInputSamplers);
	}

	virtual bool AuthorizeConnexion(int typeA, int typeB)
	{
		return true;
	}

	virtual unsigned int GetNodeTexture(size_t index)
	{
		return mEvaluation.GetEvaluationTexture(mNodes[index].mEvaluationTarget);
	}
	virtual void AddNode(size_t type)
	{
		int metaNodeCount;
		const MetaNode* metaNodes = GetMetaNodes(metaNodeCount);

		size_t index = mNodes.size();
		ImogenNode node;
		node.mEvaluationTarget = mEvaluation.AddEvaluation(type, metaNodes[type].mName);
		node.mRuntimeUniqueId = GetRuntimeId();
		node.mbProcessing = false;
		node.mType = type;
		size_t paramsSize = ComputeParamMemSize(type);
		node.mParameters = malloc(paramsSize);
		node.mParametersSize = paramsSize;
		memset(node.mParameters, 0, paramsSize);
		size_t inputCount = 0;
		for (int i = 0; i < MaxCon; i++)
			if (metaNodes[type].mInputs[i].mName != NULL)
				inputCount++;
		node.mInputSamplers.resize(inputCount);
		mNodes.push_back(node);

		mEvaluation.SetEvaluationParameters(node.mEvaluationTarget, node.mParameters, node.mParametersSize);
		mEvaluation.SetEvaluationSampler(node.mEvaluationTarget, node.mInputSamplers);
	}

	void AddLink(int InputIdx, int InputSlot, int OutputIdx, int OutputSlot)
	{
		mEvaluation.AddEvaluationInput(OutputIdx, OutputSlot, InputIdx);
	}

	virtual void DelLink(int index, int slot)
	{
		mEvaluation.DelEvaluationInput(index, slot);
	}

	virtual void DeleteNode(size_t index)
	{
		mEvaluation.DelEvaluationTarget(index);
		free(mNodes[index].mParameters);
		mNodes.erase(mNodes.begin() + index);
		for (auto& node : mNodes)
		{
			if (node.mEvaluationTarget > index)
				node.mEvaluationTarget--;
		}
	}

	virtual const MetaNode* GetMetaNodes(int &metaNodeCount)
	{
		static const uint32_t hcTransform = IM_COL32(200, 200, 200, 255);
		static const uint32_t hcGenerator = IM_COL32(150, 200, 150, 255);
		static const uint32_t hcMaterial = IM_COL32(150, 150, 200, 255);
		static const uint32_t hcBlend = IM_COL32(200, 150, 150, 255);
		static const uint32_t hcFilter = IM_COL32(200, 200, 150, 255);
		static const uint32_t hcNoise = IM_COL32(150, 250, 150, 255);
		static const uint32_t hcPaint = IM_COL32(100, 250, 180, 255);


		metaNodeCount = 29;

		static const MetaNode metaNodes[29] = {

			{
				"Circle", hcGenerator, 1
				,{ {} }
			,{ { "", (int)Con_Float4 } }
			,{ { "Radius", (int)Con_Float, -.5f,0.5f,0.f,0.f },{ "T", (int)Con_Float } }
			}
			,
			{
				"Transform", hcTransform, 0
				,{ { "", (int)Con_Float4 } }
			,{ { "", (int)Con_Float4 } }
			,{ { "Translate", (int)Con_Float2, 1.f,0.f,1.f,0.f, true },{ "Scale", (int)Con_Float2 },{ "Rotation", (int)Con_Angle } }
			}
			,
			{
				"Square", hcGenerator, 1
				,{ { } }
			,{ { "", (int)Con_Float4 } }
			,{ { "Width", (int)Con_Float, -.5f,0.5f,0.f,0.f } }
			}
			,
			{
				"Checker", hcGenerator, 1
				,{ {} }
			,{ { "", (int)Con_Float4 } }
			,{  }
			}
			,
			{
				"Sine", hcGenerator, 1
				,{ { "", (int)Con_Float4 } }
			,{ { "", (int)Con_Float4 } }
			,{ { "Frequency", (int)Con_Float },{ "Angle", (int)Con_Angle } }
			}

			,
			{
				"SmoothStep", hcFilter, 4
				,{ { "", (int)Con_Float4 } }
			,{ { "", (int)Con_Float4 } }
			,{ { "Low", (int)Con_Float },{ "High", (int)Con_Float } }
			}

			,
			{
				"Pixelize", hcTransform, 0
				,{ { "", (int)Con_Float4 } }
			,{ { "", (int)Con_Float4 } }
			,{ { "scale", (int)Con_Float } }
			}

			,
			{
				"Blur", hcFilter, 4
				,{ { "", (int)Con_Float4 } }
			,{ { "", (int)Con_Float4 } }
			,{ { "angle", (int)Con_Float },{ "strength", (int)Con_Float } }
			}

			,
			{
				"NormalMap", hcFilter, 4
				,{ { "", (int)Con_Float4 } }
			,{ { "", (int)Con_Float4 } }
			,{ { "spread", (int)Con_Float } }
			}

			,
			{
				"LambertMaterial", hcMaterial, 2
				,{ { "Diffuse", (int)Con_Float4 },{ "Equirect sky", (int)Con_Float4 } }
			,{ { "", (int)Con_Float4 } }
			,{ { "view", (int)Con_Float2, 1.f,0.f,0.f,1.f } }
			}

			,
			{
				"MADD", hcBlend, 3
				,{ { "", (int)Con_Float4 } }
			,{ { "", (int)Con_Float4 } }
			,{ { "Mul Color", (int)Con_Color4 }, {"Add Color", (int)Con_Color4} }
			}
			
			,
			{
				"Hexagon", hcGenerator, 1
				,{  }
			,{ { "", (int)Con_Float4 } }
			,{  }
			}

			,
			{
				"Blend", hcBlend, 3
				,{ { "", (int)Con_Float4 },{ "", (int)Con_Float4 } }
			,{ { "", (int)Con_Float4 } }
			,{ {"A", (int)Con_Float4 },{ "B", (int)Con_Float4 },{ "Operation", (int)Con_Enum, 0.f,0.f,0.f,0.f, false, false, "Add\0Multiply\0Darken\0Lighten\0Average\0Screen\0Color Burn\0Color Dodge\0Soft Light\0Subtract\0Difference\0Inverse Difference\0Exclusion\0" } }
			}

			,
			{
				"Invert", hcFilter, 4
				,{ { "", (int)Con_Float4 } }
			,{ { "", (int)Con_Float4 } }
			,{}
			}

			,
			{
				"CircleSplatter", hcGenerator, 1
				,{ { "", (int)Con_Float4 } }
			,{ { "", (int)Con_Float4 } }
			,{ { "Distance", (int)Con_Float2 },{ "Radius", (int)Con_Float2 },{ "Angle", (int)Con_Angle2 },{ "Count", (int)Con_Float } }
			}

			,
			{
				"Ramp", hcFilter, 4
				,{ { "", (int)Con_Float4 } }
			,{ { "", (int)Con_Float4 } }
			,{ { "Ramp", (int)Con_Ramp } }
			}

			,
			{
				"Tile", hcTransform, 0
				,{ { "", (int)Con_Float4 } }
			,{ { "", (int)Con_Float4 } }
			,{ { "Scale", (int)Con_Float },{ "Offset 0", (int)Con_Float2 },{ "Offset 1", (int)Con_Float2 },{ "Overlap", (int)Con_Float2 } }
			}

				,
				{
					"Color", hcGenerator, -1
					,{  }
				,{ { "", (int)Con_Float4 } }
				,{ { "Color", (int)Con_Color4 } }
				}


				,
				{
					"NormalMapBlending", hcBlend, 3
					,{ { "", (int)Con_Float4 },{ "", (int)Con_Float4 } }
				,{ { "Out", (int)Con_Float4 } }
				,{ { "Technique", (int)Con_Enum, 0.f,0.f,0.f,0.f, false, false, "RNM\0Partial Derivatives\0Whiteout\0UDN\0Unity\0Linear\0Overlay\0" } }
				}

				,
				{
					"iqnoise", hcNoise, 5
					,{ }
				,{ { "", (int)Con_Float4 } }
				,{ { "Size", (int)Con_Float }, { "U", (int)Con_Float, 0.f,1.f,0.f,0.f},{ "V", (int)Con_Float, 0.f,0.f,0.f,1.f } }
				}

				,
				{
					"PBR", hcMaterial, 2
					,{ { "Diffuse", (int)Con_Float4 },{ "Normal", (int)Con_Float4 },{ "Roughness", (int)Con_Float4 },{ "Displacement", (int)Con_Float4 }, { "Equirect sky", (int)Con_Float4 } }
				,{ { "", (int)Con_Float4 } }
				,{ { "view", (int)Con_Float2, 1.f,0.f,0.f,1.f, true } }
				}

				,

			{
				"PolarCoords", hcTransform, 0
				,{ { "", (int)Con_Float4 } }
				,{ { "", (int)Con_Float4 } }
				,{ { "Type", (int)Con_Enum, 0.f,0.f,0.f,0.f,false, false, "Linear to polar\0Polar to linear\0" } }
			}

      ,
				{
					"Clamp", hcFilter, 4
					,{ { "", (int)Con_Float4 } }
				,{ { "", (int)Con_Float4 } }
				,{ { "Min", (int)Con_Float4}, { "Max", (int)Con_Float4 } }
				}

				,
				{
					"ImageRead", hcFilter, 6
					,{  }
				,{ { "", (int)Con_Float4 } }
				,{ { "File name", (int)Con_FilenameRead }
			,{ "+X File name", (int)Con_FilenameRead }
			,{ "-X File name", (int)Con_FilenameRead }
			,{ "+Y File name", (int)Con_FilenameRead }
			,{ "-Y File name", (int)Con_FilenameRead }
			,{ "+Z File name", (int)Con_FilenameRead }
			,{ "-ZFile name", (int)Con_FilenameRead }}
				}

				,
				{
					"ImageWrite", hcFilter, 6
					,{ { "", (int)Con_Float4 } }
				,{  }
				,{ { "File name", (int)Con_FilenameWrite },{ "Format", (int)Con_Enum, 0.f,0.f,0.f,0.f, false, false, "JPEG\0PNG\0TGA\0BMP\0HDR\0"}
						,{ "Quality", (int)Con_Enum, 0.f,0.f,0.f,0.f, false, false, " 0 .. Best\0 1\0 2\0 3\0 4\0 5 .. Medium\0 6\0 7\0 8\0 9 .. Lowest\0" }
						,{ "Width", (int)Con_Enum, 0.f,0.f,0.f,0.f, false, false, "  256\0  512\0 1024\0 2048\0 4096\0" }
						,{ "Height", (int)Con_Enum, 0.f,0.f,0.f,0.f, false, false, "  256\0  512\0 1024\0 2048\0 4096\0" }
						,{ "Export", (int)Con_ForceEvaluate } }
				}

				,
				{
					"Thumbnail", hcFilter, 6
					,{ { "", (int)Con_Float4 } }
				,{}
				,{ { "Make", (int)Con_ForceEvaluate } }
				}

				,
				{
					"Paint2D", hcPaint, 7
					,{ { "Brush", (int)Con_Float4 } }
				,{ { "", (int)Con_Float4 } }
				,{ { "Size", (int)Con_Enum, 0.f,0.f,0.f,0.f, false, false, "  256\0  512\0 1024\0 2048\0 4096\0" } }
				, true
				, true
				}
				,
				{
					"Swirl", hcTransform, 0
					,{ { "", (int)Con_Float4 } }
				,{ { "", (int)Con_Float4 } }
				,{ { "Angles", (int)Con_Angle2 } }
				}
				,
				{
					"Crop", hcTransform, 0
					,{ { "", (int)Con_Float4 } }
				,{ { "", (int)Con_Float4 } }
				,{ { "Quad", (int)Con_Float4, 0.f,1.f,0.f,1.f, false, true } }
				, true
				}
			};

		return metaNodes;
	}
	
	const float PI = 3.14159f;
	float RadToDeg(float a) { return a * 180.f / PI; }
	float DegToRad(float a) { return a / 180.f * PI; }
	void EditNode()
	{
		size_t index = mSelectedNodeIndex;

		int metaNodeCount;
		const MetaNode* metaNodes = GetMetaNodes(metaNodeCount);
		bool dirty = false;
		bool forceEval = false;
		bool samplerDirty = false;
		ImogenNode& node = mNodes[index];
		const MetaNode& currentMeta = metaNodes[node.mType];
		
		if (ImGui::CollapsingHeader("Samplers", 0))
		{
			for (size_t i = 0; i < node.mInputSamplers.size();i++)
			{
				InputSampler& inputSampler = node.mInputSamplers[i];
				static const char *wrapModes = { "REPEAT\0CLAMP_TO_EDGE\0CLAMP_TO_BORDER\0MIRRORED_REPEAT" };
				static const char *filterModes = { "LINEAR\0NEAREST" };
				ImGui::PushItemWidth(150);
				ImGui::Text("Sampler %d", i);
				samplerDirty |= ImGui::Combo("Wrap U", (int*)&inputSampler.mWrapU, wrapModes);
				samplerDirty |= ImGui::Combo("Wrap V", (int*)&inputSampler.mWrapV, wrapModes);
				samplerDirty |= ImGui::Combo("Filter Min", (int*)&inputSampler.mFilterMin, filterModes);
				samplerDirty |= ImGui::Combo("Filter Mag", (int*)&inputSampler.mFilterMag, filterModes);
				ImGui::PopItemWidth();
			}
			if (samplerDirty)
			{
				mEvaluation.SetEvaluationSampler(node.mEvaluationTarget, node.mInputSamplers);
			}

		}
		if (!ImGui::CollapsingHeader(currentMeta.mName, 0, ImGuiTreeNodeFlags_DefaultOpen))
			return;

		const NodeGraphDelegate::Con * param = currentMeta.mParams;
		unsigned char *paramBuffer = (unsigned char*)node.mParameters;
		for (int i = 0; i < MaxCon; i++, param++)
		{
			if (!param->mName)
				break;
			ImGui::PushID(667889 + i);
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
			case Con_Color4:
				dirty |= ImGui::ColorPicker4(param->mName, (float*)paramBuffer);
				break;
			case Con_Int:
				dirty |= ImGui::InputInt(param->mName, (int*)paramBuffer);
				break;
			case Con_Ramp:
				{
					ImVec2 points[8];
					
					for (int k = 0; k < 8; k++)
					{
						points[k] = ImVec2(((float*)paramBuffer)[k * 2], ((float*)paramBuffer)[k * 2 + 1]);
						if (k && points[k - 1].x > points[k].x)
							points[k] = ImVec2(1.f, 1.f);
					}

					if (ImGui::Curve("Ramp", ImVec2(250, 150), 8, points))
					{
						for (int k = 0; k < 8; k++)
						{
							((float*)paramBuffer)[k * 2] = points[k].x;
							((float*)paramBuffer)[k * 2 + 1] = points[k].y;
						}
						dirty = true;
					}
				}
				break;
			case Con_Angle:
				((float*)paramBuffer)[0] = RadToDeg(((float*)paramBuffer)[0]);
				dirty |= ImGui::InputFloat(param->mName, (float*)paramBuffer);
				((float*)paramBuffer)[0] = DegToRad(((float*)paramBuffer)[0]);
				break;
			case Con_Angle2:
				((float*)paramBuffer)[0] = RadToDeg(((float*)paramBuffer)[0]);
				((float*)paramBuffer)[1] = RadToDeg(((float*)paramBuffer)[1]);
				dirty |= ImGui::InputFloat2(param->mName, (float*)paramBuffer);
				((float*)paramBuffer)[0] = DegToRad(((float*)paramBuffer)[0]);
				((float*)paramBuffer)[1] = DegToRad(((float*)paramBuffer)[1]);
				break;
			case Con_Angle3:
				((float*)paramBuffer)[0] = RadToDeg(((float*)paramBuffer)[0]);
				((float*)paramBuffer)[1] = RadToDeg(((float*)paramBuffer)[1]);
				((float*)paramBuffer)[2] = RadToDeg(((float*)paramBuffer)[2]);
				dirty |= ImGui::InputFloat3(param->mName, (float*)paramBuffer);
				((float*)paramBuffer)[0] = DegToRad(((float*)paramBuffer)[0]);
				((float*)paramBuffer)[1] = DegToRad(((float*)paramBuffer)[1]);
				((float*)paramBuffer)[2] = DegToRad(((float*)paramBuffer)[2]);
				break;
			case Con_Angle4:
				((float*)paramBuffer)[0] = RadToDeg(((float*)paramBuffer)[0]);
				((float*)paramBuffer)[1] = RadToDeg(((float*)paramBuffer)[1]);
				((float*)paramBuffer)[2] = RadToDeg(((float*)paramBuffer)[2]);
				((float*)paramBuffer)[3] = RadToDeg(((float*)paramBuffer)[3]);
				dirty |= ImGui::InputFloat4(param->mName, (float*)paramBuffer);
				((float*)paramBuffer)[0] = DegToRad(((float*)paramBuffer)[0]);
				((float*)paramBuffer)[1] = DegToRad(((float*)paramBuffer)[1]);
				((float*)paramBuffer)[2] = DegToRad(((float*)paramBuffer)[2]);
				((float*)paramBuffer)[3] = DegToRad(((float*)paramBuffer)[3]);
				break;
			case Con_FilenameWrite:
			case Con_FilenameRead:
				dirty |= ImGui::InputText("", (char*)paramBuffer, 1024);
				ImGui::SameLine();
				if (ImGui::Button("..."))
				{
					nfdchar_t *outPath = NULL;
					nfdresult_t result = (param->mType == Con_FilenameRead) ? NFD_OpenDialog(NULL, NULL, &outPath) : NFD_SaveDialog(NULL, NULL, &outPath);

					if (result == NFD_OKAY) 
					{
						strcpy((char*)paramBuffer, outPath);
						free(outPath);
						dirty = true;
					}
				}
				ImGui::SameLine();
				ImGui::Text(param->mName);
				break;
			case Con_Enum:
				dirty |= ImGui::Combo(param->mName, (int*)paramBuffer, param->mEnumList);
				break;
			case Con_ForceEvaluate:
				if (ImGui::Button(param->mName))
				{
					dirty = true;
					forceEval = true;
				}
				break;
			}
			ImGui::PopID();
			paramBuffer += ComputeParamMemSize(param->mType);
		}
		
		if (dirty)
			mEvaluation.SetEvaluationParameters(node.mEvaluationTarget, node.mParameters, node.mParametersSize);
		if (forceEval)
		{
			EvaluationInfo evaluationInfo;
			evaluationInfo.forcedDirty = 1;
			evaluationInfo.uiPass = 0;
			mEvaluation.PerformEvaluationForNode(node.mEvaluationTarget, 256, 256, true, evaluationInfo);
		}
	}

	virtual void DoForce()
	{
		int metaNodeCount;
		const MetaNode* metaNodes = GetMetaNodes(metaNodeCount);
		bool dirty = false;
		bool forceEval = false;
		bool samplerDirty = false;
		for (size_t index = 0; index < mNodes.size(); index++)
		{
			ImogenNode& node = mNodes[index];
			const MetaNode& currentMeta = metaNodes[node.mType];

			const NodeGraphDelegate::Con * param = currentMeta.mParams;
			unsigned char *paramBuffer = (unsigned char*)node.mParameters;
			for (int i = 0; i < MaxCon; i++, param++)
			{
				if (!param->mName)
					break;
				if (param->mType == Con_ForceEvaluate)
				{
					dirty = true;
					forceEval = true;
				}
			}
			if (forceEval)
			{
				EvaluationInfo evaluationInfo;
				evaluationInfo.forcedDirty = 1;
				evaluationInfo.uiPass = 0;
				mEvaluation.PerformEvaluationForNode(node.mEvaluationTarget, 256, 256, true, evaluationInfo);
			}
		}
	}

	void InvalidateParameters()
	{
		for (auto& node : mNodes)
			mEvaluation.SetEvaluationParameters(node.mEvaluationTarget, node.mParameters, node.mParametersSize);
	}

	template<typename T> static inline T nmin(T lhs, T rhs) { return lhs >= rhs ? rhs : lhs; }

	bool mbMouseDragging;
	void SetMouse(float rx, float ry, float dx, float dy, bool lButDown, bool rButDown)
	{
		if (mSelectedNodeIndex == -1)
			return;

		if (!lButDown)
			mbMouseDragging = false;
		int metaNodeCount;
		const MetaNode* metaNodes = GetMetaNodes(metaNodeCount);
		size_t res = 0;
		const MetaNode& metaNode = metaNodes[mNodes[mSelectedNodeIndex].mType];
		const NodeGraphDelegate::Con * param = metaNode.mParams;
		unsigned char *paramBuffer = (unsigned char*)mNodes[mSelectedNodeIndex].mParameters;
		bool parametersUseMouse = false;
		if (lButDown)
		{
			for (int i = 0; i < MaxCon; i++, param++)
			{
				if (!param->mName)
					break;
				float *paramFlt = (float*)paramBuffer;

				if (param->mbQuadSelect && param->mType == Con_Float4)
				{
					if (!mbMouseDragging)
					{
						paramFlt[2] = paramFlt[0] = rx;
						paramFlt[3] = paramFlt[1] = 1.f - ry;
						mbMouseDragging = true;
					}
					else
					{
						paramFlt[2] = rx;
						paramFlt[3] = 1.f - ry;
					}
					continue;
				}

				if (param->mRangeMinX != 0.f || param->mRangeMaxX != 0.f)
				{
					if (param->mbRelative)
					{
						paramFlt[0] += ImLerp(param->mRangeMinX, param->mRangeMaxX, dx);
						paramFlt[0] = fmodf(paramFlt[0], fabsf(param->mRangeMaxX - param->mRangeMinX)) + nmin(param->mRangeMinX, param->mRangeMaxX);
					}
					else
					{
						paramFlt[0] = ImLerp(param->mRangeMinX, param->mRangeMaxX, rx);
					}
				}
				if (param->mRangeMinY != 0.f || param->mRangeMaxY != 0.f)
				{
					if (param->mbRelative)
					{
						paramFlt[1] += ImLerp(param->mRangeMinY, param->mRangeMaxY, dy);
						paramFlt[1] = fmodf(paramFlt[1], fabsf(param->mRangeMaxY - param->mRangeMinY)) + nmin(param->mRangeMinY, param->mRangeMaxY);
					}
					else
					{
						paramFlt[1] = ImLerp(param->mRangeMinY, param->mRangeMaxY, ry);
					}
				}
				paramBuffer += ComputeParamMemSize(param->mType);
				parametersUseMouse = true;
			}
		}
		if (metaNode.mbHasUI || parametersUseMouse)
		{
			mEvaluation.SetMouse(mSelectedNodeIndex, rx, ry, lButDown, rButDown);
			mEvaluation.SetEvaluationParameters(mNodes[mSelectedNodeIndex].mEvaluationTarget, mNodes[mSelectedNodeIndex].mParameters, mNodes[mSelectedNodeIndex].mParametersSize);
		}
	}

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
	bool NodeHasUI(size_t nodeIndex)
	{
		int metaNodeCount;
		const MetaNode* metaNodes = GetMetaNodes(metaNodeCount);
		return metaNodes[mNodes[nodeIndex].mType].mbHasUI;
	}
	virtual bool NodeIsProcesing(size_t nodeIndex)
	{
		return mNodes[nodeIndex].mbProcessing;
	}
	virtual bool NodeIsCubemap(size_t nodeIndex)
	{
		RenderTarget *target = mEvaluation.GetRenderTarget(nodeIndex);
		if (target)
			return target->mImage.mNumFaces == 6;
		return false;
	}
	size_t ComputeParamMemSize(int paramType)
	{
		size_t res = 0;
		switch (paramType)
		{
		case Con_Angle:
		case Con_Float:
			res += sizeof(float);
			break;
		case Con_Angle2:
		case Con_Float2:
			res += sizeof(float) * 2;
			break;
		case Con_Angle3:
		case Con_Float3:
			res += sizeof(float) * 3;
			break;
		case Con_Angle4:
		case Con_Color4:
		case Con_Float4:
			res += sizeof(float) * 4;
			break;
		case Con_Ramp:
			res += sizeof(float) * 2 * 8;
			break;
		case Con_Enum:
		case Con_Int:
			res += sizeof(int);
			break;
		case Con_FilenameRead:
		case Con_FilenameWrite:
			res += 1024;
			break;
		case Con_ForceEvaluate:
			res += 0;
			break;
		}
		return res;
	}

	virtual void UpdateEvaluationList(const std::vector<size_t> nodeOrderList)
	{
		mEvaluation.SetEvaluationOrder(nodeOrderList);
	}

	virtual ImVec2 GetEvaluationSize(size_t nodeIndex)
	{
		int imageWidth(1), imageHeight(1);
		mEvaluation.GetEvaluationSize(int(nodeIndex), &imageWidth, &imageHeight);
		return ImVec2(float(imageWidth), float(imageHeight));
	}
	static TileNodeEditGraphDelegate *GetInstance() { return mInstance; }
	ImogenNode* Get(ASyncId id) { return GetByAsyncId(id, mNodes); }
protected:
	static TileNodeEditGraphDelegate *mInstance;
};

