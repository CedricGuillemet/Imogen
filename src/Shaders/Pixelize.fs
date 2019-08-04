$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

float u_scale;

void main()
{
	vec4 tex = texture2D(Sampler0, floor(v_texcoord0*u_scale)/u_scale);
	gl_FragColor = tex;
}
