
#include "Imogen.h"

typedef struct ImageWrite_t
{
	char filename[1024];
	int format;
	int quality;
	int width, height;
	int mode;
}ImageWrite;

int main(ImageWrite *param, Evaluation *evaluation, void *context)
{
	char *stockImages[8] = {"Stock/jpg-icon.png", "Stock/png-icon.png", "Stock/tga-icon.png", "Stock/bmp-icon.png", "Stock/hdr-icon.png", "Stock/dds-icon.png", "Stock/ktx-icon.png", "Stock/mp4-icon.png"};
	Image image;
	int imageWidth, imageHeight;
	
	image.bits = 0;
	// set info stock image
	if (ReadImage(context, stockImages[param->format], &image) == EVAL_OK)
	{
		if (SetEvaluationImage(context, evaluation->targetIndex, &image) == EVAL_OK)
		{
			FreeImage(&image);
		}
	}

    if (evaluation->inputIndices[0] == -1)
    {
        return EVAL_OK;
    }
	if (param->mode && GetEvaluationSize(context, evaluation->inputIndices[0], &imageWidth, &imageHeight) == EVAL_OK)
	{
		float ratio = (float)imageWidth / (float)imageHeight;
		if (param->mode == 1)
		{
			param->width = imageWidth;
			param->height = param->width / ratio;
		}
		else
		{
			param->width = param->height * ratio;
			param->height = imageHeight;
		}
	}
	
	if (!evaluation->forcedDirty)
		return EVAL_OK;
	
	image.bits = 0;
	if (Evaluate(context, evaluation->inputIndices[0], param->width, param->height, &image) == EVAL_OK)
	{
		if (WriteImage(context, param->filename, &image, param->format, param->quality) == EVAL_OK)
		{	
			FreeImage(&image);
			Log("Image %s saved.\n", param->filename);
			return EVAL_OK;
		}
		else
			Log("Unable to write image : %s\n", param->filename);
	}

	return EVAL_ERR;
}