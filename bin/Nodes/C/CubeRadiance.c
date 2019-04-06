
#include "Imogen.h"

typedef struct CubeRadianceParam_t
{
	int size;
}CubeRadianceParam;

		
int main(CubeRadianceParam *param, Evaluation *evaluation, void *context)
{
	int size = 256 << param->size;
	SetEvaluationCubeSize(context, evaluation->targetIndex, size, (9 + param->size));

	return EVAL_OK;
}

