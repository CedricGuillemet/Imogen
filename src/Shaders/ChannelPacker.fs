$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

int u_chanR;
int u_chanG;
int u_chanB;
int u_chanA;

void main()
{
    vec4 lu[4];
	lu[0]= texture2D(Sampler0, v_texcoord0);
	lu[1]= texture2D(Sampler1, v_texcoord0);
	lu[2]= texture2D(Sampler2, v_texcoord0);
	lu[3]= texture2D(Sampler3, v_texcoord0);
		
	vec4 res = vec4(lu[int(mod((u_chanR/4),4))][int(mod(u_chanR,4))],
		lu[int(mod((u_chanG/4),4))][int(mod(u_chanG,4))],
		lu[int(mod((u_chanB/4),4))][int(mod(u_chanB,4))],
		lu[int(mod((u_chanA/4),4))][int(mod(u_chanA,4))]);
		
	if (u_chanR>15)
		res.x = 1. - res.x;
	if (u_chanG>15)
		res.y = 1. - res.y;
	if (u_chanB>15)
		res.z = 1. - res.z;
	if (u_chanA>15)
		res.w = 1. - res.w;

	gl_FragColor = res;
}
