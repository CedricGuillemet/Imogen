$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

uniform vec4 strength;
uniform vec4 mode;

void main()
{
	vec4 texOffset = texture2D(Sampler1, v_texcoord0);
	vec2 nuv = v_texcoord0;
	int imode = int(mode.x);
	if (imode == 0)
		nuv += (texOffset.xy-0.5) * strength.x;
	else
		nuv += vec2(cos(texOffset.x * PI * 2.0), sin(texOffset.x * PI * 2.0)) * strength.x;
	vec4 tex = texture2D(Sampler0, nuv);

	gl_FragColor = tex;
}
