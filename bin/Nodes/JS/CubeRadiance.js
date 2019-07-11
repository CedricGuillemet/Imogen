function CubeRadiance(parameters, evaluation, context)
{
	size = 128 << parameters.size;
	source = evaluation->inputIndices[0];
	const res = GetEvaluationSize(context, source);
	if (parameters.size == 0 && source != -1 && res[2] == EVAL_OK)
	{
		size = res[0];
	}
	
	if (parameters.mode == 0)
	{
		// radiance
		SetEvaluationCubeSize(context, evaluation.targetIndex, size, 1);
	}
	else
	{
		// irradiance
		SetEvaluationCubeSize(context, evaluation.targetIndex, size, Math.log2(size)+1);
	}
	return EVAL_OK;
}

