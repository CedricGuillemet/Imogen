function ImageWrite(parameters, evaluation, context)
{
	const stockImages = ["Stock/jpg-icon.png", "Stock/png-icon.png", "Stock/tga-icon.png", "Stock/bmp-icon.png", "Stock/hdr-icon.png", "Stock/dds-icon.png", "Stock/ktx-icon.png", "Stock/mp4-icon.png"];
	var image = new Image;
	
	// set info stock image
	if (ReadImage(context, stockImages[parameters.format], image) == EVAL_OK)
	{
		if (SetEvaluationImage(context, evaluation.targetIndex, image) == EVAL_OK)
		{
		}
	}

    if (evaluation.inputIndices[0] == -1)
    {
        return EVAL_OK;
    }
    const res = GetEvaluationSize(context, evaluation.inputIndices[0]);
    const imageWidth = res[0];
    const imageHeight = res[1];
	if (parameters.mode && res[2] == EVAL_OK)
	{
		const ratio = imageWidth / imageHeight;
		if (parameters.mode == 1)
		{
			parameters.width = imageWidth;
			parameters.height = parameters.width / ratio;
		}
		else
		{
			parameters.width = parameters.height * ratio;
			parameters.height = imageHeight;
		}
	}
	
	if (!evaluation.forcedDirty)
    {
		return EVAL_OK;
	}
	
	if (Evaluate(context, evaluation.inputIndices[0], parameters.width, parameters.height, image) == EVAL_OK)
	{
		if (WriteImage(context, parameters.filename, parameters.format, parameters.quality, image) == EVAL_OK)
		{	
			print("Image " + parameters.filename + " saved.\n");
			return EVAL_OK;
		}
		else
        {
			print("Unable to write image : " + parameters.filename + "\n");
        }
	}

	return EVAL_ERR;
}