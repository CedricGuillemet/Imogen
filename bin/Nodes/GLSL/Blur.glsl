layout (std140) uniform BlurBlock
{
	int type;
	float angle;
	float strength;
	int passCount;
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

	vec4 col = vec4(0.0);
	if (BlurParam.type == 0)
	{
		vec2 dir = vec2(cos(BlurParam.angle), sin(BlurParam.angle));
		for(int i = 0;i<15;i++)
		{
			col += texture(Sampler0, vUV + dir * BlurParam.strength * float(i-7)) * g[i];
		}
	}
	else
	{
		// box
		float sum = 0.;
		for (int j = 0;j<15;j++)
		{
			for(int i = 0;i<15;i++)
			{
				float w = g[i] * g[j];
				col += texture(Sampler0, vUV + vec2(float(i-7), float(j-7)) * BlurParam.strength) * w;
				sum += w;
			}
		}
		col /= sum;
		
	}

	return col;
}