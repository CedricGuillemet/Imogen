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
	int targetIndex;
	int face;
	int isCube;
	void *context;
	Image image;
} JobData;

int UploadImageJob(JobData *data)
{
	if (data->isCube)
	{
		SetEvaluationImageCube(data->context, data->targetIndex, &data->image, data->face);
	}
	else
	{
		SetEvaluationImage(data->context, data->targetIndex, &data->image);
	}
	FreeImage(&data->image);
	SetProcessing(data->context, data->targetIndex, 0);
	return EVAL_OK;
}

int ReadJob(JobData *data)
{
	if (ReadImage(data->filename, &data->image) == EVAL_OK)
	{
		JobData dataUp = *data;
		JobMain(data->context, UploadImageJob, &dataUp, sizeof(JobData));
	}
	else
		SetProcessing(data->context, data->targetIndex, 0);
	return EVAL_OK;
}

int main(ImageRead *param, Evaluation *evaluation, void *context)
{
	int i;
	char *files[6] = {param->posxfile, param->negxfile, param->negyfile, param->posyfile, param->poszfile, param->negzfile};
	
	if (strlen(param->filename))
	{
		SetProcessing(data->context, evaluation->targetIndex, 1);
		JobData data;
		strcpy(data.filename, param->filename);
		data.targetIndex = evaluation->targetIndex;
		data.face = 0;
		data.isCube = 0;
		data.image.bits = 0;
		data.context = context;
		Job(context, ReadJob, &data, sizeof(JobData));
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
			JobData data;
			strcpy(data.filename, files[i]);
			data.targetIndex = evaluation->targetIndex;
			data.face = CUBEMAP_POSX + i;
			data.isCube = 1;
			data.image.bits = 0;
			data.context = context;
			Job(context, ReadJob, &data, sizeof(JobData));
		}		
	}

	return EVAL_OK;
}