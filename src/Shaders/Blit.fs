$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"

SAMPLER2D(Sampler0, 0);

void main()
{
	gl_FragColor = texture2D(Sampler0, v_texcoord0);
}
