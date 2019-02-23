#include "Imogen.h"

typedef struct SVG_t
{
	char filename[1024];
	float dpi;
} SVG;

int main(SVG *param, Evaluation *evaluation, void *context)
{
	Image image;
	image.bits = 0;
	if (param->dpi <= 1.f)
		param->dpi = 96.f;

	if (strlen(param->filename))
	{
		if (LoadSVG(param->filename, &image, param->dpi) == EVAL_OK)
		{
			SetEvaluationImage(context, evaluation->targetIndex, &image);
		}
	}
	return EVAL_OK;
}