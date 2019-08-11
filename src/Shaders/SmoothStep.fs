$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

uniform vec4 u_low;
uniform vec4 u_high;

void main()
{
	vec4 tex = texture2D(Sampler0, v_texcoord0);
	gl_FragColor = smoothstep(u_low.x, u_high.x, tex);
}
