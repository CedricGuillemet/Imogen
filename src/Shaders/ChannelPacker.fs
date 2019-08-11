$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

uniform vec4 u_chanR;
uniform vec4 u_chanG;
uniform vec4 u_chanB;
uniform vec4 u_chanA;

void main()
{
    vec4 lu[4];
	lu[0]= texture2D(Sampler0, v_texcoord0);
	lu[1]= texture2D(Sampler1, v_texcoord0);
	lu[2]= texture2D(Sampler2, v_texcoord0);
	lu[3]= texture2D(Sampler3, v_texcoord0);
		
	int T[4];
	int C[4];
	T[0] = int(mod((u_chanR.x/4.),4.));
	C[0] = int(mod(u_chanR.x,4.));
	T[1] = int(mod((u_chanG.x/4.),4.));
	C[1] = int(mod(u_chanG.x,4.));
	T[2] = int(mod((u_chanB.x/4.),4.));
	C[2] = int(mod(u_chanB.x,4.));
	T[3] = int(mod((u_chanA.x/4.),4.));
	C[3] = int(mod(u_chanA.x,4.));

	vec4 res = vec4(0.,0.,0.,0.);
	float v = 0.;
	
	for (int c=0;c<4;c++)
	{
		for (int t=0;t<4;t++)
		{
			res[c] = lu[t][c] * ((T[c]==t&&C[t]==c)?1.:0.);
		}
	}
	

	if (u_chanR.x>15.)
		res.x = 1. - res.x;
	if (u_chanG.x>15.)
		res.y = 1. - res.y;
	if (u_chanB.x>15.)
		res.z = 1. - res.z;
	if (u_chanA.x>15.)
		res.w = 1. - res.w;

	gl_FragColor = res;
}
