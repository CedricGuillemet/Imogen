layout (std140) uniform SmoothStepBlock
{
	vec4 clampMin;
	vec4 clampMax;
};

vec4 Clamp()
{
	vec4 tex = texture(Sampler0, vUV);
	return clamp(tex, clampMin, clampMax);
}
