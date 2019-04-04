#include "Imogen.h"

typedef struct EquirectConverter_t
{
	int mode;
	int size;
} EquirectConverter;


int main(EquirectConverter *param, Evaluation *evaluation, void *context)
{
	int size = 256 << param->size;
	if (param->mode == 0)
	{
		SetEvaluationCubeSize(context, evaluation->targetIndex, size, 1);
	}
	else
	{
		SetEvaluationSize(context, evaluation->targetIndex, size, size);
	}
	return EVAL_OK;
}
