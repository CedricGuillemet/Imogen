#include "Imogen.h"

typedef struct SceneLoader_t
{
	char filename[1024];
} SceneLoader;

typedef struct JobData_t
{
	char filename[1024];
	int targetIndex;
	void *context;
} JobData;

int ReadSceneJob(JobData *data)
{
	void *scene;
	if (LoadScene(data->filename, &scene) == EVAL_OK)
	{
		SetEvaluationScene(data->context, data->targetIndex, scene);
	}
	SetProcessing(data->context, data->targetIndex, 0);
	return EVAL_OK;
}

int main(SceneLoader *param, Evaluation *evaluation, void *context)
{
	if (strlen(param->filename))
	{
		SetProcessing(context, evaluation->targetIndex, 1);
		JobData data;
		strcpy(data.filename, param->filename);
		data.targetIndex = evaluation->targetIndex;
		data.context = context;
		Job(context, ReadSceneJob, &data, sizeof(JobData));
	}
	return EVAL_OK;
}