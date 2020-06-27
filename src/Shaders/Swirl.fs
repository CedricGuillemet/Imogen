$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

SAMPLER2D(Sampler0, 0);

uniform vec4 angles;

void main()
{
    vec2 uv = v_texcoord0 - vec2(0.5, 0.5);
	float len = length(uv) / (SQRT2 * 0.5);
	float angle = mix(angles.x, angles.y, len);
	vec2 nuv = Rotate2D(uv, angle) + vec2(0.5, 0.5);
	vec4 tex = texture2D(Sampler0, nuv);

	gl_FragColor = tex;
}
