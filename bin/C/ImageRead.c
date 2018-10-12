#include "Imogen.h"

typedef struct ImageRead_t
{
	char filename[1024];
	
	char posxfile[1024];
	char negxfile[1024];
	char posyfile[1024];
	char negyfile[1024];
	char poszfile[1024];
	char negzfile[1024];
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
		else
		{
			return EVAL_ERR;
		}
	}
	else
	{
		char *files[6] = {param->posxfile, param->negxfile, param->posyfile, param->negyfile, param->poszfile, param->negzfile};
		int i;
		
		for (i = 0;i<6;i++)
		{
			if (ReadImage(files[i], &image) == EVAL_OK)
			{
				if (SetEvaluationImageCube(evaluation->targetIndex, &image, CUBEMAP_POSX + i) == EVAL_OK)
				{
					FreeImage(&image);
				}
			}
		}
	}
	return EVAL_OK;
}