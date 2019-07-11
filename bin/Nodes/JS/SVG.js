function SVG(parameters, evaluation, context)
{
	image = new Image;
	if (parameters.dpi <= 1.0)
	{
		parameters.dpi = 96.0;
	}
	if (!(evaluation.dirtyFlag & DirtyParameter))
	{
		return EVAL_OK;
	}
	if (parameters.filename.length)
	{
		if (LoadSVG(parameters.filename, image, parameters.dpi) == EVAL_OK)
		{
			SetEvaluationImage(context, evaluation.targetIndex, image);
		}
	}
	return EVAL_OK;
}