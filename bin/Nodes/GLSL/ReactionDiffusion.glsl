// adapted from https://www.shadertoy.com/view/4sccRj
layout (std140) uniform ReactionDiffusionBlock
{
	float boost;
	float divisor;
	float colorStep;
	int PassCount;
};

vec4 getSample(vec2 offset)
{
	return vec4(texture(Sampler0, (gl_FragCoord.xy + offset) / EvaluationParam.viewport.xy).xyz, 1.0);
}

vec4 T(float u, float v)
{
	return getSample(vec2(u,v));
}

float f(float i)
{
    return exp(-i*i/4.)/sqrt(4.*PI);
}

#define F(i) (f(i))

#define GH(i) T(i, 0)*F(float(i))
#define GV(i) T(0, i)*F(float(i))
#define BLURH (GH(-4) + GH(-3) + GH(-2) + GH(-1) + GH(0) + GH(1) + GH(2) + GH(3) + GH(4))
#define BLURV (GV(-4) + GV(-3) + GV(-2) + GV(-1) + GV(0) + GV(1) + GV(2) + GV(3) + GV(4))

vec4 pass(vec2 uv)
{
	vec4 col = T(-1, -1)*0.05 + T( 0, -1)*0.20 + T( 1, -1)*0.05 +
               T(-1,  0)*0.20 - T( 0,  0)*1.00 + T( 1,  0)*0.20 +
               T(-1,  1)*0.05 + T( 0,  1)*0.20 + T( 1,  1)*0.05;
    col /= 8.0;
    col = T(0, 0) - col*50.0;
    col -= 0.5;
    col = col*boost;
    col = smoothstep(vec4(-0.5), vec4(0.5), col);
    col = mix(col, vec4(length(col.xyz)/sqrt(divisor)), colorStep);
	return col;
}
	
vec4 ReactionDiffusion()
{
	int ip = EvaluationParam.passNumber%3;
	if (ip == 0)
	{
		vec4 col = BLURH;
		col /= col.w;
		return col;
	}
	if (ip == 1)
	{
		vec4 col = BLURV;
		col /= col.w;
		return col;
	}
	return pass(vUV);
}
