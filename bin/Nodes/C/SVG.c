#include "Imogen.h"

typedef struct SVG_t
{
	char filename[1024];
	int size;
} SVG;

int main(SVG *param, Evaluation *evaluation)
{
	Image image;
	image.bits = 0;
	int dim = 256<<param->size;
	if (strlen(param->filename))
	{
		//SetEvaluationSize(evaluation->targetIndex, dim, dim);
		if (LoadSVG(param->filename, &image, dim, dim) == EVAL_OK)
		{
			SetEvaluationImage(evaluation->targetIndex, &image);
		}
	}
	return EVAL_OK;
}