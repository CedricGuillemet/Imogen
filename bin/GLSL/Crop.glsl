layout (std140) uniform CropBlock
{
	vec4 quad;
} CropParam;

vec4 Crop()
{
	if (EvaluationParam.uiPass == 1)
		return vec4(1.0, 0.0, 0.0, 1.0);
	vec2 uv = vec2(mix(CropParam.quad.x, CropParam.quad.z, vUV.x), mix(CropParam.quad.y, CropParam.quad.w, vUV.y));
	return texture(Sampler0, uv);
}