function ImageRead(parameters, evaluation, context)
{
	var i;
	const files = [parameters.XPosFilename, parameters.XNegFilename, parameters.YNegFilename, parameters.YPosFilename, parameters.ZPosFilename, parameters.ZNegFilename];
	if (!(evaluation.dirtyFlag & DirtyParameter))
    {
		return EVAL_OK;
	}

	if (parameters.filename.length)
	{
		SetProcessing(context, evaluation.targetIndex, 1);
        ReadImageAsync(context, parameters.filename, evaluation.targetIndex, -1);
	}
	else
	{
		for (i = 0; i < 6; i++)
		{
			if (!files[i].length)
            {
				return EVAL_OK;
            }
		}
		SetProcessing(context, evaluation.targetIndex, 1);
		for (i = 0;i<6;i++)
		{
            ReadImageAsync(context, files[i], evaluation.targetIndex, CUBEMAP_POSX + i);
		}		
	}

	return EVAL_OK;
}