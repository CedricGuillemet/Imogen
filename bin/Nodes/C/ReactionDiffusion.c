#include "Imogen.h"

typedef struct ReactionDiffusion_t
{
	float boost;
	float divisor;
	float colorStep;
	int PassCount;
	int size;
} ReactionDiffusion;

int main(ReactionDiffusion *param, Evaluation *evaluation, void *context)
{
	SetEvaluationSize(context, evaluation->targetIndex, 256<<param->size, 256<<param->size);
	return EVAL_OK;
}