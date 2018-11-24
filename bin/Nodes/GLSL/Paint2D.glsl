layout (std140) uniform Paint2DBlock
{
	int size; // 1<<(size+8)
} Paint2DParam;

vec4 brushSample(vec2 uv, float radius)
{
	vec2 nuv = (uv) / radius + vec2(0.5);
	float alphaMul = min(min(step(0.0, nuv.x), step(nuv.x, 1.0)), min(step(0.0, nuv.y), step(nuv.y, 1.0)));
	return texture(Sampler0, nuv) * alphaMul;
}
vec4 Paint2D()
{
	vec4 res = vec4(0.0);
	float brushRadius = 0.25;
	vec4 brush = brushSample(vUV-EvaluationParam.mouse.xy, brushRadius);
	if (EvaluationParam.uiPass == 1)
	{
		res = brush;
    }
	// paint pass
	if (EvaluationParam.mouse.z > 0.0)
	{
		res = brush;
	}
	if (EvaluationParam.mouse.w > 0.0)
	{
		res = brush;
		res.xyz = vec3(0.0);
	}
	
	return vec4(res.xyz*res.w, res.w);	
}