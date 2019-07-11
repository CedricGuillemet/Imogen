function PhysicalSky(parameters, evaluation, context)
{
	const size = 256 << parameters.size;
	SetEvaluationCubeSize(context, evaluation.targetIndex, size, 1);
	return EVAL_OK;
}
