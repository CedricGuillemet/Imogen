#include "Imogen.h"

typedef struct GLTFRead_t
{
	char filename[1024];
} GLTFRead;

int main(GLTFRead *param, Evaluation *evaluation, void *context)
{
	int i;
	int target = evaluation->targetIndex;
	EnableDepthBuffer(context, target, 1);
	EnableFrameClear(context, target, 1);
	SetVertexSpace(context, target, vertexSpace_World);
	
	if (strlen(param->filename) && strcmp(GetEvaluationSceneName(context, target), param->filename))
	{
        GLTFReadAsync(context, param->filename, target);
	}

	return EVAL_OK;
}