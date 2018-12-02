layout (std140) uniform WarpBlock
{
	float strength;
	int mode;
} WarpParam;

vec4 Warp()
{
	vec4 texOffset = texture(Sampler1, vUV);
	vec2 nuv = vUV;
	if (WarpParam.mode == 0)
		nuv += (texOffset.xy-0.5) * WarpParam.strength;
	else
		nuv += vec2(cos(texOffset.x * PI * 2.0), sin(texOffset.x * PI * 2.0)) * WarpParam.strength;
	vec4 tex = texture(Sampler0, nuv);

	return tex;
}
