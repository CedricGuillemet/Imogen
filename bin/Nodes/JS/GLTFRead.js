function GLTFRead(parameters, evaluation, context)
{
	var i;
	const target = evaluation.targetIndex;
	EnableDepthBuffer(context, target, 1);
	EnableFrameClear(context, target, 1);
	SetVertexSpace(context, target, VertexSpace_World);
	
	if (parameters.filename.length && GetEvaluationSceneName(context, target) == parameters.filename)
	{
		GLTFReadAsync(context, parameters.filename, target);
	}

	return EVAL_OK;
}