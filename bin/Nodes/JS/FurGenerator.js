function FurGenerator(parameters, evaluation, context)
{
	return AllocateComputeBuffer(context, evaluation.targetIndex, parameters.hairCount, 15 * 4 * 4);
}