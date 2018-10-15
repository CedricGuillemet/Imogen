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

typedef struct JobData_t
{
	char filename[1024];
	int target;
	int face;
	Image image;
} JobData;

int UploadJob(JobData *data)
{
	if (SetEvaluationImageCube(data->target, &data->image, data->face) == EVAL_OK)
	{
		FreeImage(&data->image);
	}
	return EVAL_OK;
}

int ReadJob(JobData *data)
{
	if (ReadImage(data->filename, &data->image) == EVAL_OK)
	{
		JobData dataUp = *data;
		JobMain(UploadJob, &dataUp, sizeof(JobData));
	}
	return EVAL_OK;
}

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
			JobData data;
			strcpy(data.filename, files[i]);
			data.target = evaluation->targetIndex;
			data.face = CUBEMAP_POSX + i;
			Job(ReadJob, &data, sizeof(JobData));
		}
	}
	return EVAL_OK;
}