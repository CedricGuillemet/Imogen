$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

uniform vec4 u_scale;

void main()
{
	vec4 tex = texture2D(Sampler0, floor(v_texcoord0*u_scale.x)/u_scale.x);
	gl_FragColor = tex;
}
