#include "Imogen.h"

typedef struct SceneLoader_t
{
	char filename[1024];
} SceneLoader;

int main(SceneLoader *param, Evaluation *evaluation)
{
	void *scene;
	if (strlen(param->filename))
	{
		if (LoadScene(param->filename, &scene) == EVAL_OK)
		{
			SetEvaluationScene(evaluation->targetIndex, scene);
		}
	}
	return EVAL_OK;
}