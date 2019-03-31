#include "Imogen.h"

typedef struct Paint3D_t
{
	int size;
} Paint3D;

int main(Paint3D *param, Evaluation *evaluation, void *context)
{
	if (evaluation->uiPass == 1)
	{
		SetBlendingMode(context, evaluation->targetIndex, ONE, ZERO);
		OverrideInput(context, evaluation->targetIndex, 0, evaluation->targetIndex);
		SetVertexSpace(context, evaluation->targetIndex, vertexSpace_World);
		EnableDepthBuffer(context, evaluation->targetIndex, 1);
		EnableFrameClear(context, evaluation->targetIndex, 1);
	}
	else
	{
		SetBlendingMode(context, evaluation->targetIndex, ONE, ONE_MINUS_SRC_ALPHA);
		OverrideInput(context, evaluation->targetIndex, 0, -1); // remove override
		SetVertexSpace(context, evaluation->targetIndex, vertexSpace_UV);
		EnableDepthBuffer(context, evaluation->targetIndex, 0);
		EnableFrameClear(context, evaluation->targetIndex, 0);
	}
	//SetEvaluationSize(context, evaluation->targetIndex, 256<<param->size, 256<<param->size);
	//SetBlendingMode(context, evaluation->targetIndex, ONE, ONE_MINUS_SRC_ALPHA);
	
	// use scene from input node
	void *scene;
	if (GetEvaluationScene(context, evaluation->inputIndices[0], &scene) == EVAL_OK)
	{
		SetEvaluationScene(context, evaluation->targetIndex, scene);
	}
	return EVAL_OK;
}