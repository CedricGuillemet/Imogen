
#include "Imogen.h"

typedef struct CubemapFilter_t
{
	int lightingModel;
	int excludeBase;
	int glossScale;
	int glossBias;
	int faceSize;
}CubemapFilterData;

typedef struct JobData_t
{
	int targetIndex;
	Image image;
	CubemapFilterData param;
} JobData;


int UploadImageJob(JobData *data)
{
	SetEvaluationImage(data->targetIndex, &data->image);
	FreeImage(&data->image);
	SetProcessing(data->targetIndex, 0);
	return EVAL_OK;
}

int FilterJob(JobData *data)
{
	if (CubemapFilter(&data->image, 32<<data->param.faceSize, data->param.lightingModel, data->param.excludeBase, data->param.glossScale, data->param.glossBias) == EVAL_OK)
	{	
		JobData dataUp = *data;
		JobMain(UploadImageJob, &dataUp, sizeof(JobData));
	}
	return EVAL_OK;
}		
		
int main(CubemapFilterData *param, Evaluation *evaluation)
{
	Image image;
	
	if (GetEvaluationImage(evaluation->inputIndices[0], &image) == EVAL_OK)
	{
		JobData data;
		data.image = image;
		data.targetIndex = evaluation->targetIndex;
		data.param = *param;
		SetProcessing(evaluation->targetIndex, 1);
		Job(FilterJob, &data, sizeof(JobData));
	}

	return EVAL_ERR;
}