#include "Imogen.h"

typedef struct SceneLoader_t
{
	char filename[1024];
} SceneLoader;

typedef struct JobData_t
{
	char filename[1024];
	int targetIndex;
} JobData;

int ReadSceneJob(JobData *data)
{
	void *scene;
	if (LoadScene(data->filename, &scene) == EVAL_OK)
	{
		SetEvaluationScene(data->targetIndex, scene);
	}
	SetProcessing(data->targetIndex, 0);
	return EVAL_OK;
}

int main(SceneLoader *param, Evaluation *evaluation)
{
	if (strlen(param->filename))
	{
		SetProcessing(evaluation->targetIndex, 1);
		JobData data;
		strcpy(data.filename, param->filename);
		data.targetIndex = evaluation->targetIndex;
		Job(ReadSceneJob, &data, sizeof(JobData));
	}
	return EVAL_OK;
}