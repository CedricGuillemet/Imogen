function GradientBuilder(parameters, evaluation, context)
{
	image = new Image;
	if (ReadImage(context, "Stock/thumbnail-icon.png", image) == EVAL_OK)
	{
		if (SetEvaluationImage(context, evaluation.targetIndex, image) == EVAL_OK)
		{
			return EVAL_OK;
		}
	}
	
	if (!evaluation.forcedDirty)
    {
		return EVAL_OK;
    }
    
	if (Evaluate(context, evaluation.inputIndices[0], 256, 256, image) == EVAL_OK)
	{
		if (SetThumbnailImage(context, image) == EVAL_OK)
		{	
			return EVAL_OK;
		}
	}

	return EVAL_ERR;
}
