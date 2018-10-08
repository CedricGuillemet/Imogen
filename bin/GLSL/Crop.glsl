layout (std140) uniform CropBlock
{
	vec4 quad;
} CropParam;

vec4 Crop()
{
	if (EvaluationParam.uiPass == 1)
	{
		vec4 q = vec4(min(CropParam.quad.x, CropParam.quad.z), 
			min(CropParam.quad.y, CropParam.quad.w),
			max(CropParam.quad.x, CropParam.quad.z), 
			max(CropParam.quad.y, CropParam.quad.w));
		float barx = min(step(q.x, vUV.x), step(vUV.x, q.z));
		float bary = min(step(q.y, vUV.y), step(vUV.y, q.w));
		float colFactor = min(barx, bary);
		return texture(Sampler0, vUV) * max(colFactor, 0.5);
	}
	
	vec4 q = CropParam.quad;
	vec2 uv = vec2(mix(min(q.x, q.z), max(q.x, q.z), vUV.x), mix(min(q.y, q.w), max(q.y, q.w), vUV.y));
	return texture(Sampler0, uv);
}