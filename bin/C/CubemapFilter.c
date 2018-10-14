
#include "Imogen.h"

typedef struct CubemapFilter_t
{
	int lightingModel;
	int excludeBase;
	int glossScale;
	int glossBias;
	int faceSize;
}CubemapFilterData;

int main(CubemapFilterData *param, Evaluation *evaluation)
{
	Image image;
	
	if (GetEvaluationImage(evaluation->inputIndices[0], &image) == EVAL_OK)
	{
		if (CubemapFilter(&image, 32<<param->faceSize, param->lightingModel, param->excludeBase, param->glossScale, param->glossBias) == EVAL_OK)
		{	
			SetEvaluationImage(evaluation->targetIndex, &image);
			FreeImage(&image);
			return EVAL_OK;
		}
	}

	return EVAL_ERR;
}