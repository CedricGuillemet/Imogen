$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

int u_R;
int u_G;
int u_B;
int u_A;

void main()
{
    vec4 lu[4];
	lu[0]= texture2D(Sampler0, v_texcoord0);
	lu[1]= texture2D(Sampler1, v_texcoord0);
	lu[2]= texture2D(Sampler2, v_texcoord0);
	lu[3]= texture2D(Sampler3, v_texcoord0);
		
	vec4 res = vec4(lu[int(mod((u_R/4),4))][int(mod(u_R,4))],
		lu[int(mod((u_G/4),4))][int(mod(u_G,4))],
		lu[int(mod((u_B/4),4))][int(mod(u_B,4))],
		lu[int(mod((u_A/4),4))][int(mod(u_A,4))]);
		
	if (u_R>15)
		res.x = 1. - res.x;
	if (u_G>15)
		res.y = 1. - res.y;
	if (u_B>15)
		res.z = 1. - res.z;
	if (u_A>15)
		res.w = 1. - res.w;

	gl_FragColor = res;
}
