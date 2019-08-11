$input v_texcoord0, v_color0, v_positionWorld, v_normal

#include "bgfx_shader.sh"
#include "CommonFS.shader"
#include "Common.shader"

uniform vec4 u_translation;
uniform vec4 u_freq;
uniform vec4 u_angle;

void main()
{
	vec2 nuv = v_texcoord0 - vec2(0.5, 0.5) + u_translation.xy;
	nuv = vec2(cos(u_angle.x), sin(u_angle.x)) * (nuv * u_freq.x * PI * 2.0);
    float r = cos(nuv.x + nuv.y) * 0.5 + 0.5;
	gl_FragColor = vec4(r, r, r, r);
}
