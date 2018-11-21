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
#include "ImCurveEdit.h"
#include "Library.h"
#include "nfd.h"
#include "EvaluationContext.h"

struct RampEdit : public ImCurveEdit::Delegate
{
	RampEdit()
	{
	}
	size_t GetCurveCount()
	{
		return 1;
	}

	size_t GetPointCount(size_t curveIndex)
	{
		return mPointCount;
	}

	uint32_t GetCurveColor(size_t curveIndex)
	{
		return 0xFFAAAAAA;
	}
	ImVec2* GetPoints(size_t curveIndex)
	{
		return mPts;
	}

	virtual size_t EditPoint(size_t curveIndex, size_t pointIndex, ImVec2 value)
	{
		mPts[pointIndex] = value;
		SortValues(curveIndex);
		for (size_t i = 0; i < GetPointCount(curveIndex); i++)
		{
			if (mPts[i].x == value.x)
				return i;
		}
		return pointIndex;
	}
	virtual void AddPoint(size_t curveIndex, ImVec2 value)
	{
		if (mPointCount >= 8)
			return;
		mPts[mPointCount++] = value;
		SortValues(curveIndex);
	}

	virtual void DelPoint(size_t curveIndex, size_t pointIndex)
	{
		mPts[pointIndex].x = FLT_MAX;
		SortValues(curveIndex);
		mPointCount--;
		mPts[mPointCount].x = -1.f; // end marker
	}
	ImVec2 mPts[8];
	size_t mPointCount;

private:
	void SortValues(size_t curveIndex)
	{
		auto b = std::begin(mPts);
		auto e = std::begin(mPts) + GetPointCount(curveIndex);
		std::sort(b, e, [](ImVec2 a, ImVec2 b) { return a.x < b.x; });
	}
};

struct TileNodeEditGraphDelegate : public NodeGraphDelegate
{
	TileNodeEditGraphDelegate(Evaluation& evaluation) : mEvaluation(evaluation), mbMouseDragging(false), mEditingContext(evaluation, false, 1024, 1024)
	{
		mCategoriesCount = 9;
		static const char *categories[] = {
			"Transform",
			"Generator",
			"Material",
			"Blend",
			"Filter",
			"Noise",
			"File",
			"Paint",
			"Cubemap"};
		mCategories = categories;
		assert(!mInstance);
		mInstance = this;
		gCurrentContext = &mEditingContext;
	}

	Evaluation& mEvaluation;
	EvaluationContext mEditingContext;

	struct ImogenNode
	{
#ifdef _DEBUG
		std::string mNodeTypename;
#endif
		size_t mType;
		size_t mEvaluationTarget;
		void *mParameters;
		size_t mParametersSize;
		unsigned int mRuntimeUniqueId;
		int mStartFrame, mEndFrame;
		std::vector<InputSampler> mInputSamplers;
	};

	std::vector<ImogenNode> mNodes;

	void Clear()
	{
		mSelectedNodeIndex = -1;
		mNodes.clear();
	}

	virtual unsigned char *GetParamBlock(size_t index, size_t& paramBlockSize)
	{
		const ImogenNode & node = mNodes[index];
		paramBlockSize = ComputeNodeParametersSize(node.mType);
		return (unsigned char*)node.mParameters;
	}
	virtual void SetParamBlock(size_t index, unsigned char* parameters)
	{
		const ImogenNode & node = mNodes[index];
		memcpy(node.mParameters, parameters, ComputeNodeParametersSize(node.mType));
		mEvaluation.SetEvaluationParameters(node.mEvaluationTarget, parameters, node.mParametersSize);
		mEvaluation.SetEvaluationSampler(node.mEvaluationTarget, node.mInputSamplers);
	}

	virtual bool AuthorizeConnexion(int typeA, int typeB)
	{
		return true;
	}

	virtual unsigned int GetNodeTexture(size_t index)
	{
		return mEditingContext.GetEvaluationTexture(mNodes[index].mEvaluationTarget);
	}

	virtual void AddNode(size_t type)
	{
		const size_t index			= mNodes.size();
		const size_t paramsSize		= ComputeNodeParametersSize(type);
		const size_t inputCount		= gMetaNodes[type].mInputs.size();

		ImogenNode node;
		node.mEvaluationTarget		= mEvaluation.AddEvaluation(type, gMetaNodes[type].mName);
		node.mRuntimeUniqueId		= GetRuntimeId();
		node.mType					= type;
		node.mParameters			= malloc(paramsSize);
		node.mParametersSize		= paramsSize;
		node.mStartFrame			= 0;
		node.mEndFrame				= 0;
#ifdef _DEBUG
		node.mNodeTypename			= gMetaNodes[type].mName;
#endif
		memset(node.mParameters, 0, paramsSize);
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
		mEditingContext.RunAll();
	}
	
	const float PI = 3.14159f;
	float RadToDeg(float a) { return a * 180.f / PI; }
	float DegToRad(float a) { return a / 180.f * PI; }
	void EditNode()
	{
		size_t index = mSelectedNodeIndex;

		const MetaNode* metaNodes = gMetaNodes.data();
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
		if (!ImGui::CollapsingHeader(currentMeta.mName.c_str(), 0, ImGuiTreeNodeFlags_DefaultOpen))
			return;

		unsigned char *paramBuffer = (unsigned char*)node.mParameters;
		int i = 0;
		for(const MetaParameter& param : currentMeta.mParams)
		{
			ImGui::PushID(667889 + i++);
			switch (param.mType)
			{
			case Con_Float:
				dirty |= ImGui::InputFloat(param.mName.c_str(), (float*)paramBuffer);
				break;
			case Con_Float2:
				dirty |= ImGui::InputFloat2(param.mName.c_str(), (float*)paramBuffer);
				break;
			case Con_Float3:
				dirty |= ImGui::InputFloat3(param.mName.c_str(), (float*)paramBuffer);
				break;
			case Con_Float4:
				dirty |= ImGui::InputFloat4(param.mName.c_str(), (float*)paramBuffer);
				break;
			case Con_Color4:
				dirty |= ImGui::ColorPicker4(param.mName.c_str(), (float*)paramBuffer);
				break;
			case Con_Int:
				dirty |= ImGui::InputInt(param.mName.c_str(), (int*)paramBuffer);
				break;
			case Con_Int2:
				dirty |= ImGui::InputInt2(param.mName.c_str(), (int*)paramBuffer);
				break;
			case Con_Ramp:
				{
					//ImVec2 points[8];
					
					RampEdit curveEditDelegate;
					curveEditDelegate.mPointCount = 0;
					for (int k = 0; k < 8; k++)
					{
						curveEditDelegate.mPts[k] = ImVec2(((float*)paramBuffer)[k * 2], ((float*)paramBuffer)[k * 2 + 1]);
						if (k && curveEditDelegate.mPts[k-1].x > curveEditDelegate.mPts[k].x)
							break;
						curveEditDelegate.mPointCount++;
					}
					float regionWidth = ImGui::GetWindowContentRegionWidth();
					if (ImCurveEdit::Edit(curveEditDelegate, ImVec2(regionWidth, regionWidth)))
					{
						for (size_t k = 0; k < curveEditDelegate.mPointCount; k++)
						{
							((float*)paramBuffer)[k * 2] = curveEditDelegate.mPts[k].x;
							((float*)paramBuffer)[k * 2 + 1] = curveEditDelegate.mPts[k].y;
						}
						((float*)paramBuffer)[0] = 0.f;
						((float*)paramBuffer)[(curveEditDelegate.mPointCount - 1) * 2] = 1.f;
						for (size_t k = curveEditDelegate.mPointCount; k < 8; k++)
						{
							((float*)paramBuffer)[k * 2] = -1.f;
						}
						dirty = true;
					}
				}
				break;
			case Con_Angle:
				((float*)paramBuffer)[0] = RadToDeg(((float*)paramBuffer)[0]);
				dirty |= ImGui::InputFloat(param.mName.c_str(), (float*)paramBuffer);
				((float*)paramBuffer)[0] = DegToRad(((float*)paramBuffer)[0]);
				break;
			case Con_Angle2:
				((float*)paramBuffer)[0] = RadToDeg(((float*)paramBuffer)[0]);
				((float*)paramBuffer)[1] = RadToDeg(((float*)paramBuffer)[1]);
				dirty |= ImGui::InputFloat2(param.mName.c_str(), (float*)paramBuffer);
				((float*)paramBuffer)[0] = DegToRad(((float*)paramBuffer)[0]);
				((float*)paramBuffer)[1] = DegToRad(((float*)paramBuffer)[1]);
				break;
			case Con_Angle3:
				((float*)paramBuffer)[0] = RadToDeg(((float*)paramBuffer)[0]);
				((float*)paramBuffer)[1] = RadToDeg(((float*)paramBuffer)[1]);
				((float*)paramBuffer)[2] = RadToDeg(((float*)paramBuffer)[2]);
				dirty |= ImGui::InputFloat3(param.mName.c_str(), (float*)paramBuffer);
				((float*)paramBuffer)[0] = DegToRad(((float*)paramBuffer)[0]);
				((float*)paramBuffer)[1] = DegToRad(((float*)paramBuffer)[1]);
				((float*)paramBuffer)[2] = DegToRad(((float*)paramBuffer)[2]);
				break;
			case Con_Angle4:
				((float*)paramBuffer)[0] = RadToDeg(((float*)paramBuffer)[0]);
				((float*)paramBuffer)[1] = RadToDeg(((float*)paramBuffer)[1]);
				((float*)paramBuffer)[2] = RadToDeg(((float*)paramBuffer)[2]);
				((float*)paramBuffer)[3] = RadToDeg(((float*)paramBuffer)[3]);
				dirty |= ImGui::InputFloat4(param.mName.c_str(), (float*)paramBuffer);
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
					nfdresult_t result = (param.mType == Con_FilenameRead) ? NFD_OpenDialog(NULL, NULL, &outPath) : NFD_SaveDialog(NULL, NULL, &outPath);

					if (result == NFD_OKAY) 
					{
						strcpy((char*)paramBuffer, outPath);
						free(outPath);
						dirty = true;
					}
				}
				ImGui::SameLine();
				ImGui::Text(param.mName.c_str());
				break;
			case Con_Enum:
				dirty |= ImGui::Combo(param.mName.c_str(), (int*)paramBuffer, param.mEnumList);
				break;
			case Con_ForceEvaluate:
				if (ImGui::Button(param.mName.c_str()))
				{
					EvaluationInfo evaluationInfo;
					evaluationInfo.forcedDirty = 1;
					evaluationInfo.uiPass = 0;
					mEditingContext.RunSingle(node.mEvaluationTarget, evaluationInfo);
				}
				break;
			case Con_Bool:
			{
				bool checked = (*(int*)paramBuffer) != 0;
				if (ImGui::Checkbox(param.mName.c_str(), &checked))
				{
					*(int*)paramBuffer = checked ? 1 : 0;
					dirty = true;
				}
			}
			break;
			}
			ImGui::PopID();
			paramBuffer += GetParameterTypeSize(param.mType);
		}
		
		if (dirty)
		{
			mEvaluation.SetEvaluationParameters(node.mEvaluationTarget, node.mParameters, node.mParametersSize);
			mEditingContext.SetTargetDirty(node.mEvaluationTarget);
		}
	}
	virtual void SetTimeSlot(size_t index, int frameStart, int frameEnd)
	{
		ImogenNode & node = mNodes[index];
		node.mStartFrame = frameStart;
		node.mEndFrame = frameEnd;
	}

	void SetTimeDuration(size_t index, int duration)
	{
		ImogenNode & node = mNodes[index];
		node.mEndFrame = node.mStartFrame + duration;
	}

	void SetTime(int time, bool updateDecoder)
	{
		gEvaluationTime = time;
		for (const ImogenNode& node : mNodes)
		{
			mEvaluation.SetStageLocalTime(node.mEvaluationTarget, ImClamp(time - node.mStartFrame, 0, node.mEndFrame - node.mStartFrame), updateDecoder);
		}
	}

	size_t ComputeTimelineLength() const
	{
		int len = 0;
		for (const ImogenNode& node : mNodes)
		{
			len = ImMax(len, node.mEndFrame);
			len = ImMax(len, int(node.mStartFrame + mEvaluation.GetEvaluationImageDuration(node.mEvaluationTarget)));
		}
		return size_t(len);
	}

	virtual void DoForce()
	{
		int currentTime = gEvaluationTime;
		//mEvaluation.BeginBatch();
		for (ImogenNode& node : mNodes)
		{
			const MetaNode& currentMeta = gMetaNodes[node.mType];
			bool forceEval = false;
			for(auto& param : currentMeta.mParams)
			{
				if (!param.mName.c_str())
					break;
				if (param.mType == Con_ForceEvaluate)
				{
					forceEval = true;
					break;
				}
			}
			if (forceEval)
			{
				EvaluationContext writeContext(mEvaluation, true, 1024, 1024);
				gCurrentContext = &writeContext;
				for (int frame = node.mStartFrame; frame <= node.mEndFrame; frame++)
				{
					SetTime(frame, false);
					EvaluationInfo evaluationInfo;
					evaluationInfo.forcedDirty = 1;
					evaluationInfo.uiPass = 0;
					writeContext.RunSingle(node.mEvaluationTarget, evaluationInfo);
				}
				gCurrentContext = &mEditingContext;
			}
		}
		//mEvaluation.EndBatch();
		SetTime(currentTime, true);
		InvalidateParameters();
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

		const MetaNode* metaNodes = gMetaNodes.data();
		size_t res = 0;
		const MetaNode& metaNode = metaNodes[mNodes[mSelectedNodeIndex].mType];
		unsigned char *paramBuffer = (unsigned char*)mNodes[mSelectedNodeIndex].mParameters;
		bool parametersUseMouse = false;
		if (lButDown)
		{
			for(auto& param : metaNode.mParams)
			{
				float *paramFlt = (float*)paramBuffer;
				if (param.mbQuadSelect && param.mType == Con_Float4)
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

				if (param.mRangeMinX != 0.f || param.mRangeMaxX != 0.f)
				{
					if (param.mbRelative)
					{
						paramFlt[0] += ImLerp(param.mRangeMinX, param.mRangeMaxX, dx);
						paramFlt[0] = fmodf(paramFlt[0], fabsf(param.mRangeMaxX - param.mRangeMinX)) + nmin(param.mRangeMinX, param.mRangeMaxX);
					}
					else
					{
						paramFlt[0] = ImLerp(param.mRangeMinX, param.mRangeMaxX, rx);
					}
				}
				if (param.mRangeMinY != 0.f || param.mRangeMaxY != 0.f)
				{
					if (param.mbRelative)
					{
						paramFlt[1] += ImLerp(param.mRangeMinY, param.mRangeMaxY, dy);
						paramFlt[1] = fmodf(paramFlt[1], fabsf(param.mRangeMaxY - param.mRangeMinY)) + nmin(param.mRangeMinY, param.mRangeMaxY);
					}
					else
					{
						paramFlt[1] = ImLerp(param.mRangeMinY, param.mRangeMaxY, ry);
					}
				}
				paramBuffer += GetParameterTypeSize(param.mType);
				parametersUseMouse = true;
			}
		}
		if (metaNode.mbHasUI || parametersUseMouse)
		{
			mEvaluation.SetMouse(mSelectedNodeIndex, rx, ry, lButDown, rButDown);
			mEvaluation.SetEvaluationParameters(mNodes[mSelectedNodeIndex].mEvaluationTarget, mNodes[mSelectedNodeIndex].mParameters, mNodes[mSelectedNodeIndex].mParametersSize);
			mEditingContext.SetTargetDirty(mNodes[mSelectedNodeIndex].mEvaluationTarget);
		}
	}

	size_t ComputeNodeParametersSize(size_t nodeTypeIndex)
	{
		size_t res = 0;
		for(auto& param : gMetaNodes[nodeTypeIndex].mParams)
		{
			res += GetParameterTypeSize(param.mType);
		}
		return res;
	}
	bool NodeHasUI(size_t nodeIndex)
	{
		return gMetaNodes[mNodes[nodeIndex].mType].mbHasUI;
	}
	virtual bool NodeIsProcesing(size_t nodeIndex)
	{
		return mEditingContext.StageIsProcessing(nodeIndex);
	}
	virtual bool NodeIsCubemap(size_t nodeIndex)
	{
		RenderTarget *target = mEditingContext.GetRenderTarget(nodeIndex);
		if (target)
			return target->mImage.mNumFaces == 6;
		return false;
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

