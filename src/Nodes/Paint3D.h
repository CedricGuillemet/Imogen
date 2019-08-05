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

DECLARE_NODE(Paint3D)
{
	if (evaluation->uiPass == 1)
	{
		SetBlendingMode(context, evaluation->targetIndex, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ZERO);
		OverrideInput(context, evaluation->targetIndex, 0, evaluation->targetIndex);
		SetVertexSpace(context, evaluation->targetIndex, 1/*VertexSpace_World*/);
		EnableDepthBuffer(context, evaluation->targetIndex, 1);
		EnableFrameClear(context, evaluation->targetIndex, 1);
	}
	else
	{
		SetBlendingMode(context, evaluation->targetIndex, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);
		OverrideInput(context, evaluation->targetIndex, 0, -1); // remove override
		SetVertexSpace(context, evaluation->targetIndex, 0/*VertexSpace_UV*/);
		EnableDepthBuffer(context, evaluation->targetIndex, 0);
		EnableFrameClear(context, evaluation->targetIndex, 0);
	}
	
	// use scene from input node
	void *scene;
	if (GetEvaluationScene(context, evaluation->inputIndices[0], &scene) == EVAL_OK)
	{
		SetEvaluationScene(context, evaluation->targetIndex, scene);
	}
	return EVAL_OK;
}