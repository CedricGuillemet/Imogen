#include "Imogen.h"

typedef struct ImageRead_t
{
	char filename[1024];
} ImageRead;

int main(ImageRead *param, Evaluation *evaluation)
{
	Image image;
	if (ReadImage(param->filename, &image) == EVAL_OK)
	{
		if (SetEvaluationImage(evaluation->targetIndex, &image) == EVAL_OK)
		{
			FreeImage(&image);
			return EVAL_OK;
		}
	}
	return EVAL_ERR;
}