#include "Imogen.h"

int main(void *param, Evaluation *evaluation)
{
	SetEvaluationSize(evaluation->targetIndex, 512, 64);
	return EVAL_OK;
}