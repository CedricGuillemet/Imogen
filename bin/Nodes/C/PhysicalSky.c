#include "Imogen.h"

typedef struct PhysicalSky_t
{
	float ambient[4];
	float lightdir[4], Kr[4];
	float rayleigh_brightness, mie_brightness, spot_brightness, scatter_strength, rayleigh_strength, mie_strength;
	float rayleigh_collection_power, mie_collection_power, mie_distribution;
	int size;
} PhysicalSky;


int main(PhysicalSky *param, Evaluation *evaluation, void *context)
{
	int size = 256 << param->size;
	SetEvaluationCubeSize(context, evaluation->targetIndex, size);
	return EVAL_OK;
}
