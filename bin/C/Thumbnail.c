#include "Imogen.h"

int main(void *param, Evaluation *evaluation)
{
	Image image;
	if (!evaluation->forcedDirty)
		return EVAL_OK;

	if (GetEvaluationImage(evaluation->inputIndices[0], &image) == EVAL_OK)
	{
		if (SetThumbnailImage(&image) == EVAL_OK)
		{	
			FreeImage(&image);
			return EVAL_OK;
		}
	}

	return EVAL_ERR;
}
