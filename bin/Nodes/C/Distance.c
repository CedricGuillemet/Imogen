#include "Imogen.h"

typedef struct Distance_t
{
	int PassCount;
} Distance;

int main(Distance *param, Evaluation *evaluation, void *context)
{
    int imgWidth, imgHeight;
	int source = evaluation->inputIndices[0];
	if (source != -1 && GetEvaluationSize(context, source, &imgWidth, &imgHeight) == EVAL_OK)
    {
        param->PassCount = log2(imgWidth)+3;
    }    
    return EVAL_OK;
}