#include "Imogen.h"

typedef struct GLTFRead_t
{
	char filename[1024];
} GLTFRead;

typedef struct JobData_t
{
	char filename[1024];
	int targetIndex;
	void *context;
	void *scene;
} JobData;

int UploadMeshJob(JobData *data)
{
	SetEvaluationScene(data->context, data->targetIndex, data->scene);
	SetProcessing(data->context, data->targetIndex, 0);
	return EVAL_OK;
}

int ReadJob(JobData *data)
{
	if (ReadGLTF(data->context, data->filename, &data->scene) == EVAL_OK)
	{
		JobData dataUp = *data;
		JobMain(data->context, UploadMeshJob, &dataUp, sizeof(JobData));
	}
	else
		SetProcessing(data->context, data->targetIndex, 0);
	return EVAL_OK;
}

int main(GLTFRead *param, Evaluation *evaluation, void *context)
{
	int i;
	
	if (strlen(param->filename))
	{
		SetProcessing(context, evaluation->targetIndex, 1);
		JobData data;
		strcpy(data.filename, param->filename);
		data.targetIndex = evaluation->targetIndex;
		data.context = context;
		ReadJob(&data);
		//Job(context, ReadJob, &data, sizeof(JobData));
	}

	return EVAL_OK;
}