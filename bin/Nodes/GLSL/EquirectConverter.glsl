layout (std140) uniform EquirectConverterBlock
{
	int mode;
	int size;
} EquirectConverterParam;

vec4 EquirectToCubemap()
{
	vec3 dir = (EvaluationParam.viewRot * vec4(vUV * 2.0 - 1.0, 1.0, 0.0)).xyz;
	vec2 uv = envMapEquirect(normalize(dir));
	vec4 tex = texture(Sampler0, vec2(uv.x, 1.0-uv.y));
	return tex;
}

vec4 CubemapToEquirect()
{
	vec2 uv = vUV * 2.0 - 1.0;
	vec2 ng = uv * PI * vec2(1.0, 0.5);
	vec2 a = cos(ng);
	vec2 b = sin(ng);
	return texture(CubeSampler0, normalize(vec3(a.x*a.y, -b.y, b.x*a.y))); 
}

vec4 EquirectConverter()
{
    vec4 res;
	if (EquirectConverterParam.mode == 0)
		res = EquirectToCubemap();
	else
		res = CubemapToEquirect();
    res.a = 1.0;
    return res;
}
