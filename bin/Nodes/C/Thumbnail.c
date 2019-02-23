#include "Imogen.h"

int main(void *param, Evaluation *evaluation, void *context)
{
	Image image;
	image.bits = 0;
	if (ReadImage("Stock/thumbnail-icon.png", &image) == EVAL_OK)
	{
		if (SetEvaluationImage(context, evaluation->targetIndex, &image) == EVAL_OK)
		{
			FreeImage(&image);
		}
	}
	
	if (!evaluation->forcedDirty)
		return EVAL_OK;

	image.bits = 0;
	if (Evaluate(context, evaluation->inputIndices[0], 256, 256, &image) == EVAL_OK)
	{
		if (SetThumbnailImage(context, &image) == EVAL_OK)
		{	
			FreeImage(&image);
			return EVAL_OK;
		}
	}

	return EVAL_ERR;
}
