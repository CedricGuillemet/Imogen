layout (std140) uniform SmoothStepBlock
{
	float low;
	float high;
} SmoothStepParam;

vec4 SmoothStep()
{
	vec4 tex = texture(Sampler0, vUV);
	return smoothstep(SmoothStepParam.low, SmoothStepParam.high, tex);
}
