#include "Imogen.h"

typedef struct SVG_t
{
	char filename[1024];
	int size;
} SVG;


int main(SVG *param, Evaluation *evaluation)
{
	Image image;
	int dim = 256<<param->size;
	SetEvaluationSize(evaluation->targetIndex, dim, dim);
	if (LoadSVG(param->filename, &image, dim, dim) == EVAL_ERR)
	{
		SetEvaluationImage(evaluation->targetIndex, &image);
	}
	return EVAL_OK;
}