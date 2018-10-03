layout (std140) uniform Paint2DBlock
{
	vec2 pos;
} Paint2DParam;

float Paint2D()
{
	return Circle(vUV-Paint2DParam.pos+vec2(0.5), 0.1, 0.0);
}