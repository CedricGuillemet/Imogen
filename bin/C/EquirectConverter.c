#include "Imogen.h"

typedef struct EquirectConverter_t
{
	int mode;
	int size;
} EquirectConverter;


int main(EquirectConverter *param, Evaluation *evaluation)
{
	int size = 256 << param->size;
	if (param->mode == 0)
	{
		SetEvaluationCubeSize(evaluation->targetIndex, size);
	}
	else
	{
		SetEvaluationSize(evaluation->targetIndex, size, size);
	}
	return EVAL_OK;
}
