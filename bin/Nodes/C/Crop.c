#include "Imogen.h"

typedef struct Crop_t
{
	float quad[4];
} Crop;

int main(Crop *param, Evaluation *evaluation)
{
	int imageWidth, imageHeight;
	int croppedWidth = 256, croppedHeight = 256;
	if (GetEvaluationSize(evaluation->inputIndices[0], &imageWidth, &imageHeight) == EVAL_OK)
	{
		croppedWidth = imageWidth * fabsf(param->quad[2] - param->quad[0]);
		croppedHeight = imageHeight * fabsf(param->quad[3] - param->quad[1]);
	}
	//if (croppedWidth<8) { croppedWidth = 8; }
	//if (croppedHeight<8) { croppedHeight = 8; }
	
	if (evaluation->uiPass)
		SetEvaluationSize(evaluation->targetIndex, imageWidth, imageHeight);
	else
		SetEvaluationSize(evaluation->targetIndex, croppedWidth, croppedHeight);
	
	return EVAL_OK;
}