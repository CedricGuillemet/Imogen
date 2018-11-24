layout (std140) uniform SquareBlock
{
	float width;
} SquareParam;
float Square()
{
	vec2 nuv = vUV - vec2(0.5);
    return 1.0-smoothstep(SquareParam.width-0.001, SquareParam.width, max(abs(nuv.x), abs(nuv.y)));
}
