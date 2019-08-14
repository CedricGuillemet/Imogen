$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

uniform vec4 type;

void main()
{
    vec2 uvin = v_texcoord0 - vec2(0.5,0.5);
	vec2 uv;
	int op = int(type.x);
	if (op == 1)
	{
		uv.x = cos(uvin.x * PI * 2. + PI / 2.) * (1. - (uvin.y + 0.5)) / 2. + 0.5;
		uv.y = sin(uvin.x * PI * 2. + PI / 2.) * (1. - (uvin.y + 0.5)) / 2. + 0.5;
	}
	else
	{
		uv.x = atan2(uvin.y, uvin.x);
		uv.x += PI / 2.;
		if (uv.x < 0.)
			uv.x += PI * 2.;
		uv.x /= PI * 2.;
		uv.y = 1. - length(uvin) * 2.;
	}
	vec4 tex = texture2D(Sampler0, uv);

	gl_FragColor = tex;
}
