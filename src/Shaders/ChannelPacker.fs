$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

SAMPLER2D(Sampler0, 0);
SAMPLER2D(Sampler1, 1);
SAMPLER2D(Sampler2, 2);
SAMPLER2D(Sampler3, 3);

uniform vec4 R;
uniform vec4 G;
uniform vec4 B;
uniform vec4 A;

void main()
{
    vec4 lu[4];
	lu[0] = texture2D(Sampler0, v_texcoord0);
	lu[1] = texture2D(Sampler1, v_texcoord0);
	lu[2] = texture2D(Sampler2, v_texcoord0);
	lu[3] = texture2D(Sampler3, v_texcoord0);
		
	int T[4];
	int C[4];
	T[0] = int(mod((R.x/4.),4.));
	C[0] = int(mod(R.x,4.));
	T[1] = int(mod((G.x/4.),4.));
	C[1] = int(mod(G.x,4.));
	T[2] = int(mod((B.x/4.),4.));
	C[2] = int(mod(B.x,4.));
	T[3] = int(mod((A.x/4.),4.));
	C[3] = int(mod(A.x,4.));

	vec4 res = vec4(0.,0.,0.,0.);
	
	for (int component = 0; component < 4; component++)
	{
		for (int t = 0; t < 4; t++)
		{
			for (int c = 0; c < 4; c++)
			{
				res[component] += lu[t][c] * ((T[component] == t && C[component] == c) ? 1. : 0.);
			}
		}
	}

	if (R.x>15.)
		res.x = 1. - res.x;
	if (G.x>15.)
		res.y = 1. - res.y;
	if (B.x>15.)
		res.z = 1. - res.z;
	if (A.x>15.)
		res.w = 1. - res.w;

	gl_FragColor = res;
}
