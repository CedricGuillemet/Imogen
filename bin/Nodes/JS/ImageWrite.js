
#include "Imogen.h"

typedef struct ImageWrite_t
{
	char filename[1024];
	int format;
	int quality;
	int width, height;
	int mode;
}ImageWrite;

function ImageWrite(parameters, evaluation, context)
{
	const stockImages = ["Stock/jpg-icon.png", "Stock/png-icon.png", "Stock/tga-icon.png", "Stock/bmp-icon.png", "Stock/hdr-icon.png", "Stock/dds-icon.png", "Stock/ktx-icon.png", "Stock/mp4-icon.png"];
	Image image;
	int imageWidth, imageHeight;
	
	image.bits = 0;
	// set info stock image
	if (ReadImage(context, stockImages[parameters.format], &image) == EVAL_OK)
	{
		if (SetEvaluationImage(context, evaluation.targetIndex, &image) == EVAL_OK)
		{
			FreeImage(&image);
		}
	}

    if (evaluation.inputIndices[0] == -1)
    {
        return EVAL_OK;
    }
	if (parameters.mode && GetEvaluationSize(context, evaluation.inputIndices[0], &imageWidth, &imageHeight) == EVAL_OK)
	{
		float ratio = (float)imageWidth / (float)imageHeight;
		if (parameters.mode == 1)
		{
			parameters.width = imageWidth;
			parameters.height = parameters.width / ratio;
		}
		else
		{
			parameters.width = parameters.height * ratio;
			parameters.height = imageHeight;
		}
	}
	
	if (!evaluation.forcedDirty)
		return EVAL_OK;
	
	image.bits = 0;
	if (Evaluate(context, evaluation.inputIndices[0], parameters.width, parameters.height, &image) == EVAL_OK)
	{
		if (WriteImage(context, parameters.filename, &image, parameters.format, parameters.quality) == EVAL_OK)
		{	
			FreeImage(&image);
			Log("Image %s saved.\n", parameters.filename);
			return EVAL_OK;
		}
		else
			Log("Unable to write image : %s\n", parameters.filename);
	}

	return EVAL_ERR;
}