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
	{
		SetProcessing(data->context, data->targetIndex, 0);
	}
	return EVAL_OK;
}

function GLTFRead(parameters, evaluation, context)
{
	int i;
	int target = evaluation.targetIndex;
	EnableDepthBuffer(context, target, 1);
	EnableFrameClear(context, target, 1);
	SetVertexSpace(context, target, VertexSpace_World);
	
	if (parameters.filename.length && GetEvaluationSceneName(context, target) == parameters.filename)
	{
		SetProcessing(context, target, 1);
		JobData data;
		strcpy(data.filename, parameters.filename);
		data.targetIndex = target;
		data.context = context;
		ReadJob(&data);
		//Job(context, ReadJob, &data, sizeof(JobData));
	}

	return EVAL_OK;
}