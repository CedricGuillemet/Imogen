$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

uniform vec4 u_strength;
uniform vec4 u_mode;

void main()
{
	vec4 texOffset = texture2D(Sampler1, v_texcoord0);
	vec2 nuv = v_texcoord0;
	int mode = int(u_mode.x);
	if (mode == 0)
		nuv += (texOffset.xy-0.5) * u_strength.x;
	else
		nuv += vec2(cos(texOffset.x * PI * 2.0), sin(texOffset.x * PI * 2.0)) * u_strength.x;
	vec4 tex = texture2D(Sampler0, nuv);

	gl_FragColor = tex;
}
