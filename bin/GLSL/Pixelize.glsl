layout (std140) uniform PixelizeBlock
{
	float scale;
} PixelizeParam;

vec4 Pixelize()
{
	vec4 tex = texture(Sampler0, floor(vUV*PixelizeParam.scale)/PixelizeParam.scale);
	return tex;
}
