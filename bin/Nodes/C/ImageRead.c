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

int main(ImageRead *param, Evaluation *evaluation, void *context)
{
	int i;
	char *files[6] = {param->posxfile, param->negxfile, param->negyfile, param->posyfile, param->poszfile, param->negzfile};
	
	if (!(evaluation->dirtyFlag & DirtyParameter))
    {
		return EVAL_OK;
	}
	if (strlen(param->filename))
	{
		SetProcessing(context, evaluation->targetIndex, 1);
        ReadImageAsync(context, param->filename, evaluation->targetIndex, 0);
	}
	else
	{
		for (i = 0;i<6;i++)
		{
			if (!strlen(files[i]))
				return EVAL_OK;
		}
		SetProcessing(context, evaluation->targetIndex, 1);
		for (i = 0;i<6;i++)
		{
            ReadImageAsync(context, param->filename, evaluation->targetIndex, CUBEMAP_POSX + i);
		}		
	}

	return EVAL_OK;
}