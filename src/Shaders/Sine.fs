$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

uniform vec4 translation;
uniform vec4 frequency;
uniform vec4 angle;

void main()
{
	vec2 nuv = v_texcoord0 - vec2(0.5, 0.5) + translation.xy;
	nuv = vec2(cos(angle.x), sin(angle.x)) * (nuv * frequency.x * PI * 2.0);
    float r = cos(nuv.x + nuv.y) * 0.5 + 0.5;
	gl_FragColor = vec4(r, r, r, r);
}
