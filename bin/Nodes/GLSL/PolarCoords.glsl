layout (std140) uniform PolarCoordsBlock
{
	int op;
} PolarCoordsParam;

vec4 PolarCoords()
{
    vec2 uvin = vUV - vec2(0.5,0.5);
	vec2 uv;
	if (PolarCoordsParam.op == 1)
	{
		uv.x = cos(uvin.x * PI * 2 + PI / 2) * (1 - (uvin.y + 0.5)) / 2 + 0.5;
		uv.y = sin(uvin.x * PI * 2 + PI / 2) * (1 - (uvin.y + 0.5)) / 2 + 0.5;
	}
	else
	{
		uv.x = atan(uvin.y, uvin.x);
		uv.x += PI / 2;
		if (uv.x < 0)
			uv.x += PI * 2;
		uv.x /= PI * 2;
		uv.y = 1 - length(uvin) * 2;
	}
	vec4 tex = texture(Sampler0, uv);

	return tex;
}
