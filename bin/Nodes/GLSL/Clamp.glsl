layout (std140) uniform ClampBlock
{
	vec4 clampMin;
	vec4 clampMax;
} ClampParam;

vec4 Clamp()
{
	vec4 tex = texture(Sampler0, vUV);
	return clamp(tex, ClampParam.clampMin, ClampParam.clampMax);
}
