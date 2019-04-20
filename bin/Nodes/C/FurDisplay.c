#include "Imogen.h"

int main(void *param, Evaluation *evaluation, void *context)
{
	int target = evaluation->targetIndex;
	EnableFrameClear(context, target, 1);
	EnableDepthBuffer(context, target, 1);
	return EVAL_OK;
}