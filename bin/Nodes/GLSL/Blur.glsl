layout (std140) uniform BlurBlock
{
	float angle;
	float strength;
} BlurParam;

vec4 Blur()
{
	float g[15];
	g[0] = 0.023089;
	g[1] = 0.034587;
	g[2] = 0.048689;
	g[3] = 0.064408;
	g[4] = 0.080066;
	g[5] = 0.093531;
	g[6] = 0.102673;
	g[7] = 0.105915;
	g[8] = 0.102673;
	g[9] = 0.093531;
	g[10] = 0.080066;
	g[11] = 0.064408;
	g[12] = 0.048689;
	g[13] = 0.034587;
	g[14] = 0.023089;

	vec2 dir = vec2(cos(BlurParam.angle), sin(BlurParam.angle));
	vec4 col = vec4(0.0);
	for(int i = 0;i<15;i++)
	{
		col += texture(Sampler0, vUV + dir * BlurParam.strength * float(i-7)) * g[i];
	}

	return col;
}