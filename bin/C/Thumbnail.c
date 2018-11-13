#include "Imogen.h"

int main(void *param, Evaluation *evaluation)
{
	Image image;
	
	if (ReadImage("Stock/thumbnail-icon.png", &image) == EVAL_OK)
	{
		if (SetEvaluationImage(evaluation->targetIndex, &image) == EVAL_OK)
		{
			FreeImage(&image);
		}
	}
	
	if (!evaluation->forcedDirty)
		return EVAL_OK;

	if (Evaluate(evaluation->inputIndices[0], 256, 256, &image) == EVAL_OK)
	{
		if (SetThumbnailImage(&image) == EVAL_OK)
		{	
			FreeImage(&image);
			return EVAL_OK;
		}
	}

	return EVAL_ERR;
}
