
#include "Imogen.h"

typedef struct CubeRadianceParam_t
{
	int mode; // radiance, irradiance
	int size;
	int sampleCount;
}CubeRadianceParam;

		
int main(CubeRadianceParam *param, Evaluation *evaluation, void *context)
{
	int imgWidth, imgHeight;
	int size = 128 << param->size;
	int source = evaluation->inputIndices[0];
	if (param->size == 0 && source != -1 && GetEvaluationSize(context, source, &imgWidth, &imgHeight) == EVAL_OK)
	{
		size = imgWidth;
	}
	
	if (param->mode == 0)
	{
		// radiance
		SetEvaluationCubeSize(context, evaluation->targetIndex, size, 1);
	}
	else
	{
		// irradiance
		SetEvaluationCubeSize(context, evaluation->targetIndex, size, log2(size)+1);
	}
	return EVAL_OK;
}

