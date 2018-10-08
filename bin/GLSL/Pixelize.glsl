layout (std140) uniform PixelizeBlock
{
	float scale;
} PixelizeParam;

vec4 Pixelize()
{
	vec4 tex = texture(Sampler0, floor(vUV*PixelizeParam.scale+0.5)/PixelizeParam.scale);
	return tex;
}
