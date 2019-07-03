function Crop(parameters, evaluation, context)
{
	croppedWidth = 256;
	croppedHeight = 256;
	const res = GetEvaluationSize(context, evaluation.inputIndices[0]);
	if (res[2] == EVAL_OK)
	{
		croppedWidth = res[0] * Math.abs(parameters.quad[2] - parameters.quad[0]);
		croppedHeight = res[1] * Math.abs(parameters.quad[3] - parameters.quad[1]);
	}
	//if (croppedWidth<8) { croppedWidth = 8; }
	//if (croppedHeight<8) { croppedHeight = 8; }
	
	if (evaluation.uiPass)
	{
		SetEvaluationSize(context, evaluation.targetIndex, res[0], res[1]);
	}
	else
	{
		SetEvaluationSize(context, evaluation.targetIndex, croppedWidth, croppedHeight);
	}
	
	return EVAL_OK;
}