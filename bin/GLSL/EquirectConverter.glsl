layout (std140) uniform EquirectConverterBlock
{
	int mode;
} EquirectConverterParam;

vec4 EquirectToCubemap()
{
	return vec4(0.0);
}

vec4 CubemapToEquirect()
{
	vec2 uv = (vUV - 0.5) * 2.0;
	vec2 ng = uv * vec2(3.14159265, 1.57079633);
	vec2 a = cos(ng);
	vec2 b = sin(ng);
	return texture(CubeSampler0, normalize(vec3(a.x*a.y, -b.y, b.x*a.y))); 
}

vec4 EquirectConverter()
{
	if (EquirectConverterParam.mode == 0)
		return EquirectToCubemap();
	else
		return CubemapToEquirect();
}
