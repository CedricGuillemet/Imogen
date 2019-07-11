function ReactionDiffusion(parameters, evaluation, context)
{
    if (!(evaluation.dirtyFlag & DirtyParameter))
    {
		return EVAL_OK;
    }
    
	SetEvaluationSize(context, evaluation.targetIndex, 256<<parameters.size, 256<<parameters.size);
	return EVAL_OK;
}