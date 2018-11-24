#include "Imogen.h"

typedef struct Paint2D_t
{
	int size;
} Paint2D;

int main(Paint2D *param, Evaluation *evaluation)
{
	SetEvaluationSize(evaluation->targetIndex, 256<<param->size, 256<<param->size);
	SetBlendingMode(evaluation->targetIndex, ONE, ONE_MINUS_SRC_ALPHA);
	return EVAL_OK;
}