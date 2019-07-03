function FurDisplay(parameters, evaluation, context)
{
	const target = evaluation.targetIndex;
	EnableFrameClear(context, target, 1);
	EnableDepthBuffer(context, target, 1);
	return EVAL_OK;
}