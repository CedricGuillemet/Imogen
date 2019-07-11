function Distance(parameters, evaluation, context)
{
	const source = evaluation.inputIndices[0];
	const res = GetEvaluationSize(context, source);
	if (source != -1 && res == EVAL_OK)
    {
        parameters.passCount = Math.log2(imgWidth)+3;
    }    
    return EVAL_OK;
}