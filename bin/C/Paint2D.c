#include "Imogen.h"

typedef struct Paint2D_t
{
	float x,y;
} Paint2D;

int main(Paint2D *param, Evaluation *evaluation)
{
	SetBlendingMode(evaluation->targetIndex, SRC_COLOR, ONE_MINUS_SRC_ALPHA);
	return EVAL_OK;
}