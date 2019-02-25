#include "Imogen.h"

int main(void *param, Evaluation *evaluation, void *context)
{
	SetEvaluationSize(context, evaluation->targetIndex, 512, 64);
	return EVAL_OK;
}