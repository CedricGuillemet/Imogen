
layout (std140) uniform CircleBlock
{
	float radius;
	float t;
} CircleParam;

float Circle()
{
    return 0.3;//Circle(vUV, CircleParam.radius, CircleParam.t);
}
