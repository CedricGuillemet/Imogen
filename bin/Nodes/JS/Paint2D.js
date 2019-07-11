function Paint2D(parameters, evaluation, context)
{
	SetEvaluationSize(context, evaluation.targetIndex, 256<<parameters.size, 256<<parameters.size);
	SetBlendingMode(context, evaluation.targetIndex, ONE, ONE_MINUS_SRC_ALPHA);
	return EVAL_OK;
}