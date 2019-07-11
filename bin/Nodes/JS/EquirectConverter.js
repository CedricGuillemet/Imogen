function EquirectConverter(parameters, evaluation, context)
{
	const size = 256 << parameters.size;
	if (parameters.mode == 0)
	{
		SetEvaluationCubeSize(context, evaluation.targetIndex, size, 1);
	}
	else
	{
		SetEvaluationSize(context, evaluation.targetIndex, size, size);
	}
	return EVAL_OK;
}
