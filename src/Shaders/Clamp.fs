$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

vec4 u_clampMin;
vec4 u_clampMax;

void main()
{
	vec4 tex = texture2D(Sampler0, v_texcoord0);
	gl_FragColor = clamp(tex, u_clampMin, u_clampMax);
}
