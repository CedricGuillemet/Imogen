function GradientBuilder(parameters, evaluation, context)
{
	SetEvaluationSize(context, evaluation.targetIndex, 512, 64);
	return EVAL_OK;
}