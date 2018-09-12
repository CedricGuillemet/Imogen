layout (std140) uniform ColorBlock
{
	vec4 color;
} ColorParam;

vec4 Color()
{
	return ColorParam.color;
}
