$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

vec4 minimum;
vec4 maximum;

void main()
{
	vec4 tex = texture2D(Sampler0, v_texcoord0);
	gl_FragColor = clamp(tex, minimum, maximum);
}
