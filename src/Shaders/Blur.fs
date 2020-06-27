$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

SAMPLER2D(Sampler0, 0);

uniform vec4 type;
uniform vec4 angle;
uniform vec4 strength;

void main()
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

	vec4 col = vec4(0.0, 0.0, 0.0, 0.0);
	int itype = int(type.x);
	if (itype == 0)
	{
		vec2 dir = vec2(cos(angle.x), sin(angle.x));
		for(int i = 0;i<15;i++)
		{
			col += texture2D(Sampler0, v_texcoord0 + dir * strength.x * float(i-7)) * g[i];
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
				col += texture2D(Sampler0, v_texcoord0 + vec2(float(i-7), float(j-7)) * strength.x) * w;
				sum += w;
			}
		}
		col /= sum;
		
	}

	gl_FragColor = col;
}