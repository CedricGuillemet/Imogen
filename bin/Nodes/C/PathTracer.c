#include "Imogen.h"

typedef struct PathTracer_t
{
	int mode;
} PathTracer;

int main(PathTracer *param, Evaluation *evaluation)
{
	void *scene;
	void *renderer;
	if (evaluation->inputIndices[0] == -1)
		return EVAL_OK;
	if (GetEvaluationScene(evaluation->inputIndices[0], &scene) != EVAL_OK)
		return EVAL_ERR;
	if (!scene)
		return EVAL_OK;
	if (GetEvaluationRenderer(evaluation->targetIndex, &renderer) != EVAL_OK)
		return EVAL_ERR;
	if (!renderer)
	{
		if (InitRenderer(evaluation->targetIndex, 0, scene) != EVAL_OK)
			return EVAL_ERR;
		SetEvaluationSize(evaluation->targetIndex, 1024, 1024);
	}
	if (UpdateRenderer(evaluation->targetIndex) != EVAL_OK)
		return EVAL_ERR;

	return EVAL_DIRTY;
}