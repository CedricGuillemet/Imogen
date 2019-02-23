#include "Imogen.h"

int main(void *param, Evaluation *evaluation, void *context)
{
	EnableDepthBuffer(context, evaluation->targetIndex, 1);
	return EVAL_OK;
}