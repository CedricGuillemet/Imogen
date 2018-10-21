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
	Image image;
} JobData;

int UploadImageJob(JobData *data)
{
	if (data->isCube)
	{
		SetEvaluationImageCube(data->targetIndex, &data->image, data->face);
	}
	else
	{
		SetEvaluationImage(data->targetIndex, &data->image);
	}
	FreeImage(&data->image);
	SetProcessing(data->targetIndex, 0);
	return EVAL_OK;
}

int ReadJob(JobData *data)
{
	if (ReadImage(data->filename, &data->image) == EVAL_OK)
	{
		JobData dataUp = *data;
		JobMain(UploadImageJob, &dataUp, sizeof(JobData));
	}
	return EVAL_OK;
}

int main(ImageRead *param, Evaluation *evaluation)
{
	int i;
	Image image;
	char *files[6] = {param->posxfile, param->negxfile, param->posyfile, param->negyfile, param->poszfile, param->negzfile};
	
	if (strlen(param->filename))
	{
		SetProcessing(evaluation->targetIndex, 1);
		JobData data;
		strcpy(data.filename, param->filename);
		data.targetIndex = evaluation->targetIndex;
		data.face = 0;
		data.isCube = 0;
		Job(ReadJob, &data, sizeof(JobData));
	}
	else
	{
		for (i = 0;i<6;i++)
		{
			if (!strlen(files[i]))
				return EVAL_OK;
		}
		SetProcessing(evaluation->targetIndex, 1);
		for (i = 0;i<6;i++)
		{
			JobData data;
			strcpy(data.filename, files[i]);
			data.targetIndex = evaluation->targetIndex;
			data.face = CUBEMAP_POSX + i;
			data.isCube = 1;
			Job(ReadJob, &data, sizeof(JobData));
		}		
	}

	return EVAL_OK;
}