function Distance(parameters, evaluation, context)
{
	const source = evaluation.inputIndices[0];
	const res = GetEvaluationSize(context, source);
	if (source != -1 && res[2] == EVAL_OK)
    {
        SetParameter(context, evaluation.targetIndex, "passCount", Math.log2(res[0])+1);
    }    
    return EVAL_OK;
}