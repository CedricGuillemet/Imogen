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
#include "ImGradient.h"
#include "Library.h"
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


struct GradientEdit : public ImGradient::Delegate
{
	GradientEdit()
	{
		mPointCount = 0;
	}

	size_t GetPointCount()
	{
		return mPointCount;
	}

	ImVec4* GetPoints()
	{
		return mPts;
	}

	virtual int EditPoint(int pointIndex, ImVec4 value)
	{
		mPts[pointIndex] = value;
		SortValues();
		for (size_t i = 0; i < GetPointCount(); i++)
		{
			if (mPts[i].w == value.w)
				return int(i);
		}
		return pointIndex;
	}
	virtual void AddPoint(ImVec4 value)
	{
		if (mPointCount >= 8)
			return;
		mPts[mPointCount++] = value;
		SortValues();
	}
	virtual ImVec4 GetPoint(float t)
	{
		if (GetPointCount() == 0)
			return ImVec4(1.f, 1.f, 1.f, 1.f);
		if (GetPointCount() == 1 || t <= mPts[0].w)
			return mPts[0];

		for (size_t i = 0; i < GetPointCount() - 1; i++)
		{
			if (mPts[i].w <= t && mPts[i + 1].w >= t)
			{
				float r = (t - mPts[i].w) / (mPts[i + 1].w - mPts[i].w);
				return ImLerp(mPts[i], mPts[i + 1], r);
			}
		}
		return mPts[GetPointCount() - 1];
	}
	ImVec4 mPts[8];
	size_t mPointCount;

private:
	void SortValues()
	{
		auto b = std::begin(mPts);
		auto e = std::begin(mPts) + GetPointCount();
		std::sort(b, e, [](ImVec4 a, ImVec4 b) { return a.w < b.w; });

	}
};


struct TileNodeEditGraphDelegate : public NodeGraphDelegate
{
	TileNodeEditGraphDelegate();

	void Clear();

	virtual void AddSingleNode(size_t type);
	virtual void UserAddNode(size_t type);
	virtual void AddLink(int InputIdx, int InputSlot, int OutputIdx, int OutputSlot) { gEvaluation.AddEvaluationInput(OutputIdx, OutputSlot, InputIdx);	}
	virtual void DelLink(int index, int slot) { gEvaluation.DelEvaluationInput(index, slot); }
	virtual void UserDeleteNode(size_t index);
	virtual void SetParamBlock(size_t index, const std::vector<unsigned char>& parameters);

	virtual unsigned int GetNodeTexture(size_t index) { return mEditingContext.GetEvaluationTexture(index); }

	void EditNode();

	virtual void SetTimeSlot(size_t index, int frameStart, int frameEnd);
	void SetTimeDuration(size_t index, int duration);
	void SetTime(int time, bool updateDecoder);
	size_t ComputeTimelineLength() const;

	virtual void DoForce();
	void InvalidateParameters();

	void SetMouse(float rx, float ry, float dx, float dy, bool lButDown, bool rButDown, float wheel);

	size_t ComputeNodeParametersSize(size_t nodeTypeIndex);
	bool NodeHasUI(size_t nodeIndex) { return gMetaNodes[mNodes[nodeIndex].mType].mbHasUI; }
	virtual bool NodeIsProcesing(size_t nodeIndex) { return mEditingContext.StageIsProcessing(nodeIndex); }
	virtual bool NodeIsCubemap(size_t nodeIndex);
	virtual void UpdateEvaluationList(const std::vector<size_t> nodeOrderList) { gEvaluation.SetEvaluationOrder(nodeOrderList);	}
	virtual ImVec2 GetEvaluationSize(size_t nodeIndex);
	
	virtual void CopyNodes(const std::vector<size_t> nodes);
	virtual void CutNodes(const std::vector<size_t> nodes);
	virtual void PasteNodes();

	EvaluationContext mEditingContext;

	struct ImogenNode
	{
#ifdef _DEBUG
		std::string mNodeTypename;
#endif
		size_t mType;
		std::vector<unsigned char> mParameters;
		unsigned int mRuntimeUniqueId;
		int mStartFrame, mEndFrame;
		std::vector<InputSampler> mInputSamplers;
	};

	std::vector<ImogenNode> mNodes;
	std::vector<ImogenNode> mNodesClipboard;
	bool mbMouseDragging;

	ImogenNode* Get(ASyncId id) { return GetByAsyncId(id, mNodes); }
protected:
	void InitDefault(ImogenNode& node);
};

extern TileNodeEditGraphDelegate gNodeDelegate;