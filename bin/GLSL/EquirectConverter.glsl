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
	return vec4(0.0);
}

vec4 EquirectConverter()
{
	if (EquirectConverterParam.mode == 0)
		return EquirectToCubemap();
	else
		return CubemapToEquirect();
}
