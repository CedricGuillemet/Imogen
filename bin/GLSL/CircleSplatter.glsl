layout (std140) uniform CircleSplatterBlock
{
	vec2 distToCenter;
	vec2 radii;
	vec2 angles;
	float count;
} CircleSplatterParam;

vec4 CircleSplatter()
{
	vec4 col = vec4(0.0);
	for (float i = 0.0 ; i < CircleSplatterParam.count ; i += 1.0)
	{
		float t = i/(CircleSplatterParam.count-0.0);
		vec2 dist = vec2(mix(CircleSplatterParam.distToCenter.x, CircleSplatterParam.distToCenter.y, t), 0.0);
		dist = Rotate2D(dist, mix(CircleSplatterParam.angles.x, CircleSplatterParam.angles.y, t));
		float radius = mix(CircleSplatterParam.radii.x, CircleSplatterParam.radii.y, t);
		col = max(col, vec4(Circle(vUV-dist, radius, 0.0)));
	}
	return col;
}