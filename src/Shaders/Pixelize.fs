$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

SAMPLER2D(Sampler0, 0);

uniform vec4 scale;

void main()
{
	vec4 tex = texture2D(Sampler0, floor(v_texcoord0*scale.x)/scale.x);
	gl_FragColor = tex;
}
