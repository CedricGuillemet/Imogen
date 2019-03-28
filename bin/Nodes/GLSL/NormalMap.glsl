layout (std140) uniform NormalMapBlock
{
	float spread;
} NormalMapParam;

vec2 stdNormalMap(vec2 uv) 
{
    float height = texture(Sampler0, uv).r;
    return -vec2(dFdx(height), dFdy(height));
}

vec4 NormalMap()
{
	return vec4(stdNormalMap(vUV) * NormalMapParam.spread + 0.5, 1., 1.);
}
