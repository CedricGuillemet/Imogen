$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

vec4 u_color;

void main()
{
	gl_FragColor = u_color;
}
