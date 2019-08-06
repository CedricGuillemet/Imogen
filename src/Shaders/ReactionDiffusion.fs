$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

// adapted from https://www.shadertoy.com/view/4sccRj
float u_boost;
float u_divisor;
float u_colorStep;
int u_PassCount;
int u_size;

vec4 getSample(vec2 offset, vec2 fragCoord)
{
	return vec4(texture2D(Sampler0, (fragCoord.xy + offset) / u_viewport.xy).xyz, 1.0);
}

vec4 T(float u, float v, vec2 fragCoord)
{
	return getSample(vec2(u,v), fragCoord);
}

float f(float i)
{
    return exp(-i*i/4.)/sqrt(4.*PI);
}

#define F(i) (f(i))

#define GH(i) T(float(i), 0., fc)*F(float(i))
#define GV(i) T(0., float(i), fc)*F(float(i))
#define BLURH (GH(-4) + GH(-3) + GH(-2) + GH(-1) + GH(0) + GH(1) + GH(2) + GH(3) + GH(4))
#define BLURV (GV(-4) + GV(-3) + GV(-2) + GV(-1) + GV(0) + GV(1) + GV(2) + GV(3) + GV(4))

vec4 dopass(vec2 uv, vec2 fc)
{
	vec4 col = T(-1., -1., fc)*0.05 + T( 0., -1., fc)*0.20 + T( 1., -1., fc)*0.05 +
               T(-1.,  0., fc)*0.20 - T( 0.,  0., fc)*1.00 + T( 1.,  0., fc)*0.20 +
               T(-1.,  1., fc)*0.05 + T( 0.,  1., fc)*0.20 + T( 1.,  1., fc)*0.05;
    col /= 8.0;
    col = T(0., 0., fc) - col*50.0;
    col -= 0.5;
    col = col*u_boost;
    col = smoothstep(vec4(-0.5, -0.5, -0.5, -0.5), vec4(0.5, 0.5, 0.5, 0.5), col);
	float ra = length(col.xyz)/sqrt(u_divisor);
    col = mix(col, vec4(ra,ra,ra,ra), u_colorStep);
	return col;
}
	
void main()
{
	vec2 fc = gl_FragCoord.xy;
	int ip = int(mod(u_pass.x,4.));
	if (ip == 0)
	{
		vec4 col = BLURH;
		col /= col.w;
		gl_FragColor = col;
		return;
	}
	if (ip == 1)
	{
		vec4 col = BLURV;
		col /= col.w;
		gl_FragColor = col;
		return;
	}
	gl_FragColor = dopass(v_texcoord0, fc);
}
