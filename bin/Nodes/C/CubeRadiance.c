
#include "Imogen.h"

typedef struct CubeRadianceParam_t
{
	int size;
}CubeRadianceParam;

		
int main(CubeRadianceParam *param, Evaluation *evaluation, void *context)
{
	int size = 256 << param->size;
	SetEvaluationCubeSize(context, evaluation->targetIndex, size, (8 << param->size)+1);

	return EVAL_OK;
}

