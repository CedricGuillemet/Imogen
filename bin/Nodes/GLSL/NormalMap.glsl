layout (std140) uniform NormalMapBlock
{
	float spread;
	int invert;
} NormalMapParam;

vec2 stdNormalMap(vec2 uv) 
{
    float height = texture(Sampler0, uv).r;
	if (NormalMapParam.invert != 0)
	{
		height = 1.0 - height;
	}
    return -vec2(dFdx(height), dFdy(height));
}

vec4 NormalMap()
{
	return vec4(stdNormalMap(vUV) * NormalMapParam.spread + 0.5, 1., 1.);
}
