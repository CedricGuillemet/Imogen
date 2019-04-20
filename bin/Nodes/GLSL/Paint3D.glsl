layout (std140) uniform Paint3DBlock
{
	int size; // 1<<(size+8)
} Paint3DParam;

vec4 brushSample(vec2 uv, float radius)
{
	vec2 nuv = (uv) / radius + vec2(0.5);
	float alphaMul = min(min(step(0.0, nuv.x), step(nuv.x, 1.0)), min(step(0.0, nuv.y), step(nuv.y, 1.0)));
	return texture(Sampler1, nuv) * alphaMul;
}

vec4 brushSampleMouse()
{
	vec4 wPos = EvaluationParam.viewProjection * vec4(vWorldPosition, 1.0);
	mat4 projMat = mat4(
		vec4(1.0, 0.0, 0.0, 0.0),
		vec4(0.0, 1.0, 0.0, 0.0),
		vec4(0.0, 0.0, 1.0, 0.0),
		vec4(-(EvaluationParam.mouse.xy * 2.0 - 1.0), 0.0, 1.0));
	
	vec4 projUV = projMat * wPos;
	return brushSample(projUV.xy/projUV.w, 0.3);
}

vec4 Paint3D()
{
	if (EvaluationParam.uiPass == 1)
	{
		vec3 lightdir = normalize(vec3(1.0));
		float dt = max(dot(lightdir, normalize(vWorldNormal)), 0.5);
		vec4 tex = texture(Sampler0, vUV) * dt;
		
		
		return vec4(tex.xyz, 1.0) + brushSampleMouse() * vec4(1.0, 0.0, 0.0, 1.0);
	}	
	else
	{
		vec4 res = vec4(0.0);
		if (EvaluationParam.keyModifier.x != 0 || EvaluationParam.keyModifier.y != 0 || EvaluationParam.keyModifier.z != 0)
		{
			return res;
		}
		// paint pass
		if (EvaluationParam.mouse.z > 0.0)
		{
			res = brushSampleMouse();
		}
		if (EvaluationParam.mouse.w > 0.0)
		{
			res = brushSampleMouse();
			res.xyz = vec3(0.0);
		}
		return vec4(res.xyz*res.w, res.w);	
	}
}