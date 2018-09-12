layout (std140) uniform BlurBlock
{
	float angle;
	float strength;
} BlurParam;

vec4 Blur()
{
	vec2 dir = vec2(cos(BlurParam.angle), sin(BlurParam.angle));
	vec4 col = vec4(0.0);
	for(float i = -5.0;i<=5.0;i += 1.0)
	{
		col += texture(Sampler0, vUV + dir * BlurParam.strength * i);
	}
	col /= 11.0;
	return col;
}