#include "Imogen.h"

typedef struct ImageWrite_t
{
	char filename[1024];
	int format;
	int quality;
}ImageWrite;

int main(ImageWrite *param, Evaluation *evaluation)
{
	if (!evaluation->forcedDirty)
		return EVAL_OK;
	
	Image image;
	if (GetEvaluationImage(evaluation->inputIndices[0], &image) == EVAL_OK)
	{
		if (WriteImage(param->filename, &image, param->format, param->quality) == EVAL_OK)
		{	
			FreeImage(&image);
			Log("Image %s saved.\n");
			return EVAL_OK;
		}
	}
	return EVAL_ERR;
}