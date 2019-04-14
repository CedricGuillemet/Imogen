#include "Imogen.h"

typedef struct FurGenerator_t
{
	int hairCount;
	float lengthFactor;
} FurGenerator;

int main(FurGenerator *param, Evaluation *evaluation, void *context)
{
	return AllocateComputeBuffer(context, evaluation->targetIndex, param->hairCount, 15 * 4 *sizeof(float) );
}