
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
	void *context;
} JobData;


int UploadImageJob(JobData *data)
{
	SetEvaluationImage(data->context, data->targetIndex, &data->image);
	FreeImage(&data->image);
	SetProcessing(data->context, data->targetIndex, 0);
	return EVAL_OK;
}

int FilterJob(JobData *data)
{
	if (CubemapFilter(&data->image, 32<<data->param.faceSize, data->param.lightingModel, data->param.excludeBase, data->param.glossScale, data->param.glossBias) == EVAL_OK)
	{	
		JobData dataUp = *data;
		JobMain(data->context, UploadImageJob, &dataUp, sizeof(JobData));
	}
	return EVAL_OK;
}		
		
int main(CubemapFilterData *param, Evaluation *evaluation, void *context)
{
	Image image;
	
	if (GetEvaluationImage(context, evaluation->inputIndices[0], &image) == EVAL_OK)
	{
		JobData data;
		data.image = image;
		data.targetIndex = evaluation->targetIndex;
		data.param = *param;
		data.context = context;
		SetProcessing(context, evaluation->targetIndex, 1);
		Job(context, FilterJob, &data, sizeof(JobData));
	}

	return EVAL_ERR;
}