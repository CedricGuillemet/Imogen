#include "Imogen.h"

int main(void *param, Evaluation *evaluation)
{
	EnableDepthBuffer(evaluation->targetIndex, 1);
	return EVAL_OK;
}