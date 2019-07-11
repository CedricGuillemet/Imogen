#include "Imogen.h"

typedef struct PathTracer_t
{
	int mode;
} PathTracer;

function PathTracer(parameters, evaluation, context)
{
	void *scene;
	void *renderer;
	if (evaluation.inputIndices[0] == -1)
		return EVAL_OK;
	if (GetEvaluationRTScene(context, evaluation.inputIndices[0], &scene) != EVAL_OK)
		return EVAL_ERR;
	if (!scene)
		return EVAL_OK;
	if (GetEvaluationRenderer(context, evaluation.targetIndex, &renderer) != EVAL_OK)
		return EVAL_ERR;
	if (!renderer)
	{
		if (InitRenderer(context, evaluation.targetIndex, 0, scene) != EVAL_OK)
			return EVAL_ERR;
	}
	SetEvaluationSize(context, evaluation.targetIndex, 1024, 1024);
	SetProcessing(context, evaluation.targetIndex, 2);
	
	return UpdateRenderer(context, evaluation.targetIndex);
}